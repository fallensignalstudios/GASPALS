#include "AbilitySystem/SFGameplayAbility_WeaponBeam.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraShakeBase.h"
#include "Characters/SFCharacterBase.h"
#include "Characters/SFPlayerCharacter.h"
#include "Combat/SFWeaponActor.h"
#include "Combat/SFWeaponData.h"
#include "Combat/SFWeaponInstanceTypes.h"
#include "Components/SFEquipmentComponent.h"
#include "Core/SignalForgeGameplayTags.h"
#include "Faction/SFFactionStatics.h"
#include "Engine/World.h"
#include "GameplayCueManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

USFGameplayAbility_WeaponBeam::USFGameplayAbility_WeaponBeam()
{
	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();

	InputTag = Tags.Input_PrimaryFire;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(Tags.Ability_Weapon_BeamFire);
	// Also tag with PrimaryFire so anything listening for "primary fire active" sees the beam.
	AssetTags.AddTag(Tags.Ability_Weapon_PrimaryFire);
	SetAssetTags(AssetTags);

	// Cancel the bullet primary-fire if it's somehow active when we ignite (and vice-versa).
	FGameplayTagContainer CancelTags;
	CancelTags.AddTag(Tags.Ability_Weapon_PrimaryFire);
	CancelTags.AddTag(Tags.Ability_Weapon_BeamFire);
	CancelAbilitiesWithTag = CancelTags;

	// Don't allow ignition while a weapon swap is in flight, or while a melee swing is
	// mid-animation.
	FGameplayTagContainer BlockedTags;
	BlockedTags.AddTag(Tags.State_Weapon_Switching);
	BlockedTags.AddTag(Tags.State_Weapon_MeleeSwinging);
	ActivationBlockedTags = BlockedTags;

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

bool USFGameplayAbility_WeaponBeam::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		return false;
	}

	ASFCharacterBase* Character = Cast<ASFCharacterBase>(ActorInfo->AvatarActor.Get());
	if (!Character)
	{
		return false;
	}

	USFEquipmentComponent* Equipment = Character->GetEquipmentComponent();
	if (!Equipment)
	{
		return false;
	}

	USFWeaponData* WeaponData = Equipment->GetCurrentWeaponData();
	if (!WeaponData)
	{
		return false;
	}

	// Only honour the beam ability when the weapon is actually configured for beam fire.
	// Misconfiguration: warn so designers spot it instead of silently no-op'ing.
	if (WeaponData->RangedConfig.FireMode != ESFWeaponFireMode::Beam)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("WeaponBeam::CanActivateAbility -> '%s' is granted WeaponBeam ability but FireMode is %d, not Beam. ")
			TEXT("Set RangedConfig.FireMode = Beam on the weapon data."),
			*WeaponData->GetName(),
			(int32)WeaponData->RangedConfig.FireMode);
		return false;
	}

	return true;
}

void USFGameplayAbility_WeaponBeam::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	UE_LOG(LogTemp, Log, TEXT("WeaponBeam::ActivateAbility -> entered"));

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Warning, TEXT("WeaponBeam::ActivateAbility -> CommitAbility FAILED (cost/cooldown blocked?)"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CachedHandle = Handle;
	CachedActorInfo = ActorInfo;
	CachedActivationInfo = ActivationInfo;
	bTriggerHeld = true;
	bBeamActive = false;
	bAppliedFiringTag = false;

	BeginBeam();
}

void USFGameplayAbility_WeaponBeam::InputReleased(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	bTriggerHeld = false;

	// Trigger released -> shut down the beam, then end the ability so a fresh tap
	// can re-activate it. Leaving the ability active after release was masking
	// subsequent fire attempts and producing the "nothing fires" symptom.
	if (bBeamActive)
	{
		StopBeam(/*bOverheated=*/false);
	}

	FinishAbility(false);

	Super::InputReleased(Handle, ActorInfo, ActivationInfo);
}

void USFGameplayAbility_WeaponBeam::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// If ending while the beam is still on (e.g. cancel from weapon switch), make sure the
	// loop cue and Firing tag don't leak.
	if (bBeamActive)
	{
		StopBeam(/*bOverheated=*/false);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BeamTickTimerHandle);
	}

	// Only remove the Firing tag if we actually added it -- otherwise we trip the
	// "Attempted to remove tag ... not explicitly in the container" warning.
	if (bAppliedFiringTag && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
		if (Tags.State_Weapon_Firing.IsValid())
		{
			ActorInfo->AbilitySystemComponent->RemoveLooseGameplayTag(Tags.State_Weapon_Firing);
		}
		bAppliedFiringTag = false;
	}

	CachedHandle = FGameplayAbilitySpecHandle();
	CachedActorInfo = nullptr;
	CachedActivationInfo = FGameplayAbilityActivationInfo();
	bBeamActive = false;
	bTriggerHeld = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USFGameplayAbility_WeaponBeam::FinishAbility(bool bWasCancelled)
{
	if (IsActive())
	{
		EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, bWasCancelled);
	}
}

void USFGameplayAbility_WeaponBeam::BeginBeam()
{
	ASFCharacterBase* Character = nullptr;
	USFEquipmentComponent* Equipment = nullptr;
	USFWeaponData* WeaponData = nullptr;
	ASFWeaponActor* WeaponActor = nullptr;
	FSFRangedWeaponConfig RangedConfig;
	FSFBeamWeaponConfig BeamConfig;

	if (!ResolveBeamContext(Character, Equipment, WeaponData, WeaponActor, RangedConfig, BeamConfig))
	{
		UE_LOG(LogTemp, Warning, TEXT("WeaponBeam::BeginBeam -> ResolveBeamContext FAILED"));
		FinishAbility(true);
		return;
	}

	// Make sure the instance has a non-sentinel battery value; check overheat lockout.
	EnsureBatteryInitialized(Equipment, BeamConfig);
	FSFWeaponInstanceData Instance = Equipment->GetCurrentWeaponInstance();

	if (Instance.bBeamOverheated)
	{
		const float ResumeThreshold = BeamConfig.OverheatResumeChargeFraction * BeamConfig.BatteryCapacity;
		if (Instance.BeamBatteryCharge < ResumeThreshold)
		{
			// Locked out until battery recovers. Bail without lighting the beam.
			UE_LOG(LogTemp, Warning,
				TEXT("WeaponBeam::BeginBeam -> overheat lockout active (charge=%.1f, resume>=%.1f)"),
				Instance.BeamBatteryCharge, ResumeThreshold);
			FinishAbility(false);
			return;
		}

		// Recovered enough — clear the overheat flag and let the beam start.
		Instance.bBeamOverheated = false;
		Equipment->UpdateActiveWeaponInstance(Instance);
	}

	if (Instance.BeamBatteryCharge <= 0.0f)
	{
		// Nothing left to fire with.
		FinishAbility(false);
		return;
	}

	// Compute tick rate from RPM (unified rate-of-fire knob), clamped to sane bounds.
	const float DesiredHz = RangedConfig.RoundsPerMinute / 60.0f;
	CachedTicksPerSecond = FMath::Clamp(DesiredHz, MinTicksPerSecond, MaxTicksPerSecond);
	const float TickInterval = 1.0f / CachedTicksPerSecond;

	// Mark firing + light the beam.
	if (UAbilitySystemComponent* ASC = CachedActorInfo->AbilitySystemComponent.Get())
	{
		const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
		if (Tags.State_Weapon_Firing.IsValid())
		{
			ASC->AddLooseGameplayTag(Tags.State_Weapon_Firing, 1);
			bAppliedFiringTag = true;
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("WeaponBeam::BeginBeam -> ignited (RPM=%.0f -> %.1f Hz, charge=%.1f / %.1f)"),
		RangedConfig.RoundsPerMinute, CachedTicksPerSecond,
		Instance.BeamBatteryCharge, BeamConfig.BatteryCapacity);

	// BeamStart cue: muzzle location/forward direction so the cue receiver can attach the loop VFX.
	FVector MuzzleLocation = Character->GetActorLocation();
	FRotator MuzzleRotation = Character->GetActorRotation();
	if (WeaponActor && WeaponActor->HasValidMuzzleSocket())
	{
		MuzzleLocation = WeaponActor->GetMuzzleLocation();
		MuzzleRotation = WeaponActor->GetMuzzleRotation();
	}

	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
	const FGameplayTag StartTag = BeamConfig.BeamStartCueOverride.IsValid()
		? BeamConfig.BeamStartCueOverride
		: Tags.Cue_Weapon_BeamStart;
	DispatchBeamCue(StartTag, MuzzleLocation, MuzzleRotation.Vector(), WeaponActor);

	// Optional looping fire montage. Kept local-only (no replication); the GameplayCue handles cosmetic sync.
	if (BeamConfig.BeamLoopMontage)
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			if (UAnimInstance* Anim = Mesh->GetAnimInstance())
			{
				Anim->Montage_Play(BeamConfig.BeamLoopMontage, 1.0f);
			}
		}
	}

	bBeamActive = true;

	// Fire once immediately so the player gets instant feedback, then schedule the loop.
	BeamTick();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			BeamTickTimerHandle,
			FTimerDelegate::CreateUObject(this, &USFGameplayAbility_WeaponBeam::BeamTick),
			TickInterval,
			/*bLoop=*/true);
	}
}

void USFGameplayAbility_WeaponBeam::BeamTick()
{
	if (!bBeamActive)
	{
		return;
	}

	if (!CachedActorInfo || !CachedActorInfo->AvatarActor.IsValid())
	{
		StopBeam(false);
		FinishAbility(true);
		return;
	}

	ASFCharacterBase* Character = nullptr;
	USFEquipmentComponent* Equipment = nullptr;
	USFWeaponData* WeaponData = nullptr;
	ASFWeaponActor* WeaponActor = nullptr;
	FSFRangedWeaponConfig RangedConfig;
	FSFBeamWeaponConfig BeamConfig;

	if (!ResolveBeamContext(Character, Equipment, WeaponData, WeaponActor, RangedConfig, BeamConfig))
	{
		StopBeam(false);
		FinishAbility(true);
		return;
	}

	// Trigger released between ticks -> shut down cleanly. (InputReleased already calls
	// StopBeam, but be defensive: weapon switch cancellation can also drop bTriggerHeld.)
	if (!bTriggerHeld)
	{
		StopBeam(false);
		FinishAbility(false);
		return;
	}

	// Drain battery. If empty, overheat.
	FSFWeaponInstanceData Instance = Equipment->GetCurrentWeaponInstance();
	Instance.BeamBatteryCharge -= BeamConfig.DrainPerTick;
	const bool bWillOverheat = (Instance.BeamBatteryCharge <= 0.0f);
	if (bWillOverheat)
	{
		Instance.BeamBatteryCharge = 0.0f;
		Instance.bBeamOverheated = true;
	}

	if (UWorld* World = GetWorld())
	{
		Instance.LastBeamFireWorldTime = World->GetTimeSeconds();
	}
	Equipment->UpdateActiveWeaponInstance(Instance);

	// Aim source: eye viewpoint, mirroring WeaponFire so the beam goes where the crosshair points.
	FVector EyeLocation;
	FRotator EyeRotation;
	Character->GetActorEyesViewPoint(EyeLocation, EyeRotation);

	// Apply ADS spread if any.
	const float SpreadHalfAngle = IsAimingDownSights(Character) ? RangedConfig.AdsSpreadDegrees : RangedConfig.HipfireSpreadDegrees;
	const FRotator AimRotation = ApplyConeSpread(EyeRotation, SpreadHalfAngle);

	// Muzzle location for cue & damage falloff origin.
	FVector MuzzleLocation = EyeLocation;
	if (WeaponActor && WeaponActor->HasValidMuzzleSocket())
	{
		MuzzleLocation = WeaponActor->GetMuzzleLocation();
	}

	// Hitscan trace from eye to max beam range.
	UWorld* World = Character->GetWorld();
	FVector EndLocation = EyeLocation + AimRotation.Vector() * FMath::Max(100.0f, BeamConfig.BeamRange);
	FVector ImpactNormal = -AimRotation.Vector();
	bool bDidHit = false;

	if (World)
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(SFWeaponBeamTick), true, Character);
		Params.bReturnPhysicalMaterial = true;
		if (WeaponActor)
		{
			Params.AddIgnoredActor(WeaponActor);
		}

		FHitResult Hit;
		bDidHit = World->LineTraceSingleByChannel(Hit, EyeLocation, EndLocation, TraceChannel, Params);
		if (bDidHit)
		{
			EndLocation = Hit.ImpactPoint;
			ImpactNormal = Hit.ImpactNormal;
			ApplyBeamHit(Character, RangedConfig, BeamConfig, Hit, MuzzleLocation);
		}
	}

	// BeamTick cue: cue receiver should snap the ribbon endpoint to Location.
	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
	const FGameplayTag TickTag = BeamConfig.BeamTickCueOverride.IsValid()
		? BeamConfig.BeamTickCueOverride
		: Tags.Cue_Weapon_BeamTick;
	DispatchBeamCue(TickTag, EndLocation, ImpactNormal, WeaponActor);

	// Optional separate impact cue (sparks / decals). Only when we actually hit something.
	if (bDidHit && Tags.Cue_Weapon_BeamImpact.IsValid())
	{
		DispatchBeamCue(Tags.Cue_Weapon_BeamImpact, EndLocation, ImpactNormal, WeaponActor);
	}

	// Recoil: scaled per-tick so DPS-equivalent climb matches bullet weapons of the same RPM.
	ApplyBeamRecoilForTick(Character, RangedConfig, CachedTicksPerSecond);

	// If we just hit zero, stop with overheat. Do this AFTER the tick so the player still gets
	// the last damage + cue from the shot that drained the battery.
	if (bWillOverheat)
	{
		// Overheat cue + sound.
		if (Tags.Cue_Weapon_BeamOverheat.IsValid())
		{
			DispatchBeamCue(Tags.Cue_Weapon_BeamOverheat, MuzzleLocation, AimRotation.Vector(), WeaponActor);
		}
		if (BeamConfig.OverheatSound)
		{
			UGameplayStatics::PlaySoundAtLocation(Character, BeamConfig.OverheatSound, MuzzleLocation);
		}

		StopBeam(/*bOverheated=*/true);
		FinishAbility(false);
	}
}

void USFGameplayAbility_WeaponBeam::StopBeam(bool bOverheated)
{
	if (!bBeamActive)
	{
		return;
	}

	bBeamActive = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BeamTickTimerHandle);
	}

	// Clear the firing tag so animations / state machines stop their "beam-on" branch.
	// Only remove if we actually added it (guards against double-remove via EndAbility safety net).
	if (bAppliedFiringTag && CachedActorInfo && CachedActorInfo->AbilitySystemComponent.IsValid())
	{
		const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
		if (Tags.State_Weapon_Firing.IsValid())
		{
			CachedActorInfo->AbilitySystemComponent->RemoveLooseGameplayTag(Tags.State_Weapon_Firing);
		}
		bAppliedFiringTag = false;
	}

	// BeamStop cue at the muzzle so the ribbon tears down at the right location.
	if (CachedActorInfo && CachedActorInfo->AvatarActor.IsValid())
	{
		if (ASFCharacterBase* Character = Cast<ASFCharacterBase>(CachedActorInfo->AvatarActor.Get()))
		{
			FVector MuzzleLocation = Character->GetActorLocation();
			FRotator MuzzleRotation = Character->GetActorRotation();
			if (USFEquipmentComponent* Equipment = Character->GetEquipmentComponent())
			{
				ASFWeaponActor* WeaponActor = Equipment->GetEquippedWeaponActor();
				if (WeaponActor && WeaponActor->HasValidMuzzleSocket())
				{
					MuzzleLocation = WeaponActor->GetMuzzleLocation();
					MuzzleRotation = WeaponActor->GetMuzzleRotation();
				}

				if (USFWeaponData* WeaponData = Equipment->GetCurrentWeaponData())
				{
					const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
					const FGameplayTag StopTag = WeaponData->BeamConfig.BeamStopCueOverride.IsValid()
						? WeaponData->BeamConfig.BeamStopCueOverride
						: Tags.Cue_Weapon_BeamStop;
					DispatchBeamCue(StopTag, MuzzleLocation, MuzzleRotation.Vector(), WeaponActor);
				}
			}
		}
	}

	(void)bOverheated;  // Reserved for future hooks (UI overheat warning, distinct stop cue, etc.).
}

void USFGameplayAbility_WeaponBeam::ApplyBeamHit(
	ASFCharacterBase* Character,
	const FSFRangedWeaponConfig& RangedConfig,
	const FSFBeamWeaponConfig& BeamConfig,
	const FHitResult& Hit,
	const FVector& MuzzleLocation)
{
	AActor* TargetActor = Hit.GetActor();
	if (!Character || !TargetActor)
	{
		return;
	}

	// Friend-foe gate: skip non-hostile / dead characters unless the weapon opted into
	// friendly fire via FSFRangedWeaponConfig::bAllowFriendlyFire. Beams tick frequently, so
	// rejecting early keeps us from spamming GE specs into allied ASCs.
	if (ASFCharacterBase* HitCharacter = Cast<ASFCharacterBase>(TargetActor))
	{
		if (HitCharacter->IsDead())
		{
			return;
		}
		if (!RangedConfig.bAllowFriendlyFire && !USFFactionStatics::AreHostile(Character, HitCharacter))
		{
			return;
		}
	}

	UAbilitySystemComponent* SourceASC = Character->GetAbilitySystemComponent();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);

	const TSubclassOf<UGameplayEffect> EffectClass = RangedConfig.DamageEffectClass
		? RangedConfig.DamageEffectClass
		: DefaultDamageEffect;

	const bool bHeadshot = BeamConfig.bAllowHeadshots && HeadshotBones.Contains(Hit.BoneName);
	const float HeadshotMult = bHeadshot ? RangedConfig.HeadshotMultiplier : 1.0f;
	const float Distance = FVector::Dist(MuzzleLocation, Hit.ImpactPoint);

	// Reuse the same falloff curve as bullet hits so beam damage feels consistent at long range.
	float FalloffMult = 1.0f;
	if (RangedConfig.FalloffEndDistance > RangedConfig.FalloffStartDistance)
	{
		if (Distance >= RangedConfig.FalloffEndDistance)
		{
			FalloffMult = RangedConfig.MinFalloffMultiplier;
		}
		else if (Distance > RangedConfig.FalloffStartDistance)
		{
			const float Alpha = (Distance - RangedConfig.FalloffStartDistance) /
				FMath::Max(1.0f, (RangedConfig.FalloffEndDistance - RangedConfig.FalloffStartDistance));
			FalloffMult = FMath::Lerp(1.0f, RangedConfig.MinFalloffMultiplier, Alpha);
		}
	}

	const float FinalDamage = BeamConfig.DamagePerTick * FalloffMult * HeadshotMult;

	if (SourceASC && TargetASC && EffectClass && FinalDamage > 0.0f)
	{
		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		Context.AddSourceObject(Character);
		Context.AddHitResult(Hit);

		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, 1.0f, Context);
		if (SpecHandle.IsValid())
		{
			const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
			if (Tags.Data_BaseDamage.IsValid())
			{
				SpecHandle.Data->SetSetByCallerMagnitude(Tags.Data_BaseDamage, FinalDamage);
			}
			if (Tags.Data_FalloffMultiplier.IsValid())
			{
				SpecHandle.Data->SetSetByCallerMagnitude(Tags.Data_FalloffMultiplier, FalloffMult);
			}
			if (Tags.Data_RangedDistance.IsValid())
			{
				SpecHandle.Data->SetSetByCallerMagnitude(Tags.Data_RangedDistance, Distance);
			}
			if (bHeadshot && Tags.Data_IsWeakpointHit.IsValid())
			{
				SpecHandle.Data->SetSetByCallerMagnitude(Tags.Data_IsWeakpointHit, 1.0f);
			}

			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}
	}

	// Last-damaging-character attribution -- generalized to any ASFCharacterBase so XP/credit
	// flows for any faction matchup, not just legacy enemy actors.
	if (ASFCharacterBase* HitCharacter = Cast<ASFCharacterBase>(TargetActor))
	{
		HitCharacter->SetLastDamagingCharacter(Character);
	}
}

void USFGameplayAbility_WeaponBeam::ApplyBeamRecoilForTick(
	ASFCharacterBase* Character,
	const FSFRangedWeaponConfig& RangedConfig,
	float TicksPerSecond)
{
	if (!Character || TicksPerSecond <= 0.0f)
	{
		return;
	}

	// Bullet recoil is specified per-shot. For a beam at TicksPerSecond Hz to match the same
	// "muzzle climb per second" envelope as a bullet weapon at the same RPM, we divide the
	// per-shot kick by ticks-per-second. (Bullet weapons at RPM N fire N/60 shots per second
	// too, so this gives parity at equal RPM.)
	const float PerTickPitchScale = 1.0f / TicksPerSecond;

	const float AdsScale = IsAimingDownSights(Character)
		? FMath::Max(0.0f, RangedConfig.AdsRecoilMultiplier)
		: 1.0f;

	if (ASFPlayerCharacter* PlayerChar = Cast<ASFPlayerCharacter>(Character))
	{
		const float PitchKick = FMath::Max(0.0f, RangedConfig.VerticalRecoil) * AdsScale * PerTickPitchScale;
		float YawKick = 0.0f;
		if (RangedConfig.HorizontalRecoil > 0.0f)
		{
			const float Sign = FMath::FRandRange(-1.0f, 1.0f) >= 0.0f ? 1.0f : -1.0f;
			YawKick = RangedConfig.HorizontalRecoil * AdsScale * PerTickPitchScale * Sign;
		}

		if (PitchKick > 0.0f || !FMath::IsNearlyZero(YawKick))
		{
			PlayerChar->ApplyRecoilKick(
				PitchKick,
				YawKick,
				RangedConfig.RecoilInterpSpeed,
				RangedConfig.RecoilRecoverySpeed,
				RangedConfig.RecoilRecoveryFraction);
		}
	}

	// Optional camera shake — note we only spawn one per tick. If this feels too jittery the
	// designer should leave FireCameraShake null on beam weapons and rely on the recoil kick alone.
	TSubclassOf<UCameraShakeBase> ShakeClass = RangedConfig.FireCameraShake ? RangedConfig.FireCameraShake : DefaultCameraShake;
	if (ShakeClass)
	{
		if (UWorld* World = Character->GetWorld())
		{
			UGameplayStatics::PlayWorldCameraShake(
				World,
				ShakeClass,
				Character->GetActorLocation(),
				0.0f,
				CameraShakeRadius,
				1.0f);
		}
	}
}

void USFGameplayAbility_WeaponBeam::DispatchBeamCue(
	const FGameplayTag& Tag,
	const FVector& Location,
	const FVector& Normal,
	ASFWeaponActor* WeaponActor)
{
	if (!Tag.IsValid() || !CachedActorInfo)
	{
		return;
	}

	AActor* Avatar = CachedActorInfo->AvatarActor.Get();
	if (!Avatar)
	{
		return;
	}

	FGameplayCueParameters Params;
	Params.Instigator = Avatar;
	Params.EffectCauser = Avatar;
	Params.Location = Location;
	Params.Normal = Normal;
	Params.RawMagnitude = 1.0f;

	// Pass the weapon actor through SourceObject so cue notify Blueprints can
	// attach Niagara directly to the weapon's muzzle socket. Read in BP via
	// `Parameters -> SourceObject` and cast to ASFWeaponActor.
	if (WeaponActor)
	{
		Params.SourceObject = WeaponActor;
	}

	if (UAbilitySystemComponent* ASC = CachedActorInfo->AbilitySystemComponent.Get())
	{
		ASC->ExecuteGameplayCue(Tag, Params);
	}
	else if (UGameplayCueManager* CueMgr = UAbilitySystemGlobals::Get().GetGameplayCueManager())
	{
		CueMgr->HandleGameplayCue(Avatar, Tag, EGameplayCueEvent::Executed, Params);
	}
}

void USFGameplayAbility_WeaponBeam::EnsureBatteryInitialized(
	USFEquipmentComponent* Equipment,
	const FSFBeamWeaponConfig& BeamConfig)
{
	if (!Equipment)
	{
		return;
	}

	FSFWeaponInstanceData Instance = Equipment->GetCurrentWeaponInstance();

	// Sentinel: negative charge means "uninitialized" -> fill to capacity.
	if (Instance.BeamBatteryCharge < 0.0f)
	{
		Instance.BeamBatteryCharge = BeamConfig.BatteryCapacity;
		Instance.bBeamOverheated = false;
		Equipment->UpdateActiveWeaponInstance(Instance);
		return;
	}

	// Apply recharge for time elapsed since last fire (continuous regen between ignitions).
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float Now = World->GetTimeSeconds();
	const float Elapsed = Now - Instance.LastBeamFireWorldTime;
	const float Idle = Elapsed - BeamConfig.RechargeDelaySeconds;

	if (Idle > 0.0f && BeamConfig.RechargeUnitsPerSecond > 0.0f && Instance.BeamBatteryCharge < BeamConfig.BatteryCapacity)
	{
		const float Recovered = Idle * BeamConfig.RechargeUnitsPerSecond;
		Instance.BeamBatteryCharge = FMath::Min(BeamConfig.BatteryCapacity, Instance.BeamBatteryCharge + Recovered);
		// Bump the timestamp so the next ignition doesn't re-credit the same idle window.
		Instance.LastBeamFireWorldTime = Now;
		Equipment->UpdateActiveWeaponInstance(Instance);
	}
}

bool USFGameplayAbility_WeaponBeam::ResolveBeamContext(
	ASFCharacterBase*& OutCharacter,
	USFEquipmentComponent*& OutEquipment,
	USFWeaponData*& OutWeaponData,
	ASFWeaponActor*& OutWeaponActor,
	FSFRangedWeaponConfig& OutRangedConfig,
	FSFBeamWeaponConfig& OutBeamConfig) const
{
	OutCharacter = nullptr;
	OutEquipment = nullptr;
	OutWeaponData = nullptr;
	OutWeaponActor = nullptr;

	if (!CachedActorInfo || !CachedActorInfo->AvatarActor.IsValid())
	{
		return false;
	}

	OutCharacter = Cast<ASFCharacterBase>(CachedActorInfo->AvatarActor.Get());
	if (!OutCharacter)
	{
		return false;
	}

	OutEquipment = OutCharacter->GetEquipmentComponent();
	if (!OutEquipment)
	{
		return false;
	}

	OutWeaponData = OutEquipment->GetCurrentWeaponData();
	if (!OutWeaponData)
	{
		return false;
	}

	OutWeaponActor = OutEquipment->GetEquippedWeaponActor();
	OutRangedConfig = OutWeaponData->RangedConfig;
	OutBeamConfig = OutWeaponData->BeamConfig;
	return true;
}

bool USFGameplayAbility_WeaponBeam::IsAimingDownSights(ASFCharacterBase* Character) const
{
	if (!Character)
	{
		return false;
	}

	if (UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent())
	{
		const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
		if (Tags.State_Weapon_ADS.IsValid())
		{
			return ASC->HasMatchingGameplayTag(Tags.State_Weapon_ADS);
		}
	}
	return false;
}

FRotator USFGameplayAbility_WeaponBeam::ApplyConeSpread(const FRotator& BaseRotation, float HalfAngleDegrees) const
{
	if (HalfAngleDegrees <= 0.0f)
	{
		return BaseRotation;
	}

	const FVector BaseDir = BaseRotation.Vector();
	const FVector Spread = FMath::VRandCone(BaseDir, FMath::DegreesToRadians(HalfAngleDegrees));
	return Spread.Rotation();
}
