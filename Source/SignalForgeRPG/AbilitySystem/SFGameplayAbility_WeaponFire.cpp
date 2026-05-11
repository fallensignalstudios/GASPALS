#include "AbilitySystem/SFGameplayAbility_WeaponFire.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraShakeBase.h"
#include "Characters/SFCharacterBase.h"
#include "Characters/SFEnemyCharacter.h"
#include "Combat/SFProjectileBase.h"
#include "Combat/SFWeaponActor.h"
#include "Combat/SFWeaponData.h"
#include "Combat/SFWeaponInstanceTypes.h"
#include "Components/SFEquipmentComponent.h"
#include "Core/SignalForgeGameplayTags.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameplayCueManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

USFGameplayAbility_WeaponFire::USFGameplayAbility_WeaponFire()
{
	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();

	InputTag = Tags.Input_PrimaryFire;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(Tags.Ability_Weapon_PrimaryFire);
	SetAssetTags(AssetTags);

	// Cancel any other weapon-fire ability instance so we never double-fire.
	FGameplayTagContainer CancelTags;
	CancelTags.AddTag(Tags.Ability_Weapon_PrimaryFire);
	CancelAbilitiesWithTag = CancelTags;

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

bool USFGameplayAbility_WeaponFire::CanActivateAbility(
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

	// Allow activation even when out of ammo so we can play the dry-fire feedback,
	// but only when an actual weapon is equipped.
	return Equipment->GetCurrentWeaponData() != nullptr;
}

void USFGameplayAbility_WeaponFire::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CachedHandle = Handle;
	CachedActorInfo = ActorInfo;
	CachedActivationInfo = ActivationInfo;
	BurstShotsRemaining = 0;
	bIsCycling = false;

	HandleTriggerPull();
}

void USFGameplayAbility_WeaponFire::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BurstTimerHandle);
		World->GetTimerManager().ClearTimer(CycleTimerHandle);
	}

	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
		if (Tags.State_Weapon_Firing.IsValid())
		{
			ActorInfo->AbilitySystemComponent->RemoveLooseGameplayTag(Tags.State_Weapon_Firing);
		}
	}

	CachedHandle = FGameplayAbilitySpecHandle();
	CachedActorInfo = nullptr;
	CachedActivationInfo = FGameplayAbilityActivationInfo();
	BurstShotsRemaining = 0;
	bIsCycling = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USFGameplayAbility_WeaponFire::FinishAbility(bool bWasCancelled)
{
	if (IsActive())
	{
		EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, bWasCancelled);
	}
}

void USFGameplayAbility_WeaponFire::HandleTriggerPull()
{
	if (!CachedActorInfo || !CachedActorInfo->AvatarActor.IsValid())
	{
		FinishAbility(true);
		return;
	}

	ASFCharacterBase* Character = Cast<ASFCharacterBase>(CachedActorInfo->AvatarActor.Get());
	USFEquipmentComponent* Equipment = nullptr;
	USFWeaponData* WeaponData = nullptr;
	ASFWeaponActor* WeaponActor = nullptr;
	FSFRangedWeaponConfig Config;

	if (!Character || !ResolveRangedContext(Character, Equipment, WeaponData, WeaponActor, Config))
	{
		FinishAbility(true);
		return;
	}

	// Empty-clip handling
	if (WeaponData->AmmoConfig.ClipSize > 0)
	{
		const FSFWeaponInstanceData CurrentInstance = Equipment->GetCurrentWeaponInstance();
		if (CurrentInstance.AmmoInClip < FMath::Max(1, WeaponData->AmmoConfig.AmmoPerShot))
		{
			HandleEmptyClick(Character, Config);
			FinishAbility(false);
			return;
		}
	}

	switch (Config.FireMode)
	{
	case ESFWeaponFireMode::Burst:
		BurstShotsRemaining = FMath::Max(1, Config.BurstCount);
		FireOneShot();
		break;

	case ESFWeaponFireMode::FullAuto:
	case ESFWeaponFireMode::Single:
	case ESFWeaponFireMode::Charge:
	case ESFWeaponFireMode::Beam:
	default:
		BurstShotsRemaining = 0;
		FireOneShot();
		break;
	}
}

void USFGameplayAbility_WeaponFire::FireOneShot()
{
	if (!CachedActorInfo || !CachedActorInfo->AvatarActor.IsValid())
	{
		FinishAbility(true);
		return;
	}

	ASFCharacterBase* Character = Cast<ASFCharacterBase>(CachedActorInfo->AvatarActor.Get());
	USFEquipmentComponent* Equipment = nullptr;
	USFWeaponData* WeaponData = nullptr;
	ASFWeaponActor* WeaponActor = nullptr;
	FSFRangedWeaponConfig Config;

	if (!Character || !ResolveRangedContext(Character, Equipment, WeaponData, WeaponActor, Config))
	{
		FinishAbility(true);
		return;
	}

	// Ammo gating per shot (burst can otherwise outrun the magazine).
	if (WeaponData->AmmoConfig.ClipSize > 0)
	{
		if (!TryConsumeAmmo(Equipment, Config))
		{
			HandleEmptyClick(Character, Config);
			FinishAbility(false);
			return;
		}
	}

	// Aim source: prefer eye viewpoint (camera-aligned crosshair) for hitscan & projectile direction.
	FVector EyeLocation;
	FRotator EyeRotation;
	Character->GetActorEyesViewPoint(EyeLocation, EyeRotation);

	// Muzzle location: weapon actor socket if available.
	FVector MuzzleLocation = EyeLocation;
	FRotator MuzzleRotation = EyeRotation;
	if (WeaponActor && WeaponActor->HasValidMuzzleSocket())
	{
		MuzzleLocation = WeaponActor->GetMuzzleLocation();
		MuzzleRotation = EyeRotation;
	}

	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
	if (UAbilitySystemComponent* ASC = CachedActorInfo->AbilitySystemComponent.Get())
	{
		if (Tags.State_Weapon_Firing.IsValid())
		{
			ASC->AddLooseGameplayTag(Tags.State_Weapon_Firing, 1);
		}
	}

	const int32 Pellets = FMath::Max(1, Config.PelletsPerShot);
	const float SpreadHalfAngle = IsAimingDownSights(Character) ? Config.AdsSpreadDegrees : Config.HipfireSpreadDegrees;

	for (int32 Pellet = 0; Pellet < Pellets; ++Pellet)
	{
		const FRotator ShotRotation = ApplyConeSpread(EyeRotation, SpreadHalfAngle);

		if (Config.bHitscan)
		{
			FireHitscanPellet(Character, WeaponActor, Config, MuzzleLocation, ShotRotation);
		}
		else
		{
			FireProjectilePellet(Character, WeaponActor, Config, MuzzleLocation, ShotRotation);
		}
	}

	// Compute a "tracer end" point along the unspread aim for the muzzle/tracer cue.
	const FVector TracerEnd = EyeLocation + EyeRotation.Vector() * FMath::Max(500.0f, Config.HitscanMaxRange);
	DispatchMuzzleAndTracerCues(Character, WeaponActor, Config, MuzzleLocation, MuzzleRotation, TracerEnd);
	ApplyRecoilAndShake(Character, Config);

	// Optional fire montage (animation only, not gating the ability lifecycle).
	UAnimMontage* FireMontage = Config.FireMontageOverride;
	if (!FireMontage)
	{
		if (USFEquipmentComponent* Equip = Character->GetEquipmentComponent())
		{
			FireMontage = Equip->GetCurrentLightAttackMontage();
		}
	}
	if (FireMontage)
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			if (UAnimInstance* Anim = Mesh->GetAnimInstance())
			{
				Anim->Montage_Play(FireMontage, 1.0f);
			}
		}
	}

	// Schedule the next shot for burst / full-auto, or finish for single shot.
	UWorld* World = GetWorld();
	const float CycleTime = FMath::Max(0.02f, Config.GetCycleTime());

	if (BurstShotsRemaining > 0)
	{
		--BurstShotsRemaining;
		if (BurstShotsRemaining > 0 && World)
		{
			const float Interval = Config.BurstIntervalSeconds > 0.0f ? Config.BurstIntervalSeconds : CycleTime;
			World->GetTimerManager().SetTimer(
				BurstTimerHandle,
				FTimerDelegate::CreateUObject(this, &USFGameplayAbility_WeaponFire::FireOneShot),
				Interval,
				false);
			return;
		}
	}

	// Hold the State_Weapon_Firing tag for one cycle so animations / state machines see it.
	if (World)
	{
		bIsCycling = true;
		World->GetTimerManager().SetTimer(
			CycleTimerHandle,
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				if (CachedActorInfo && CachedActorInfo->AbilitySystemComponent.IsValid())
				{
					const FSignalForgeGameplayTags& T = FSignalForgeGameplayTags::Get();
					if (T.State_Weapon_Firing.IsValid())
					{
						CachedActorInfo->AbilitySystemComponent->RemoveLooseGameplayTag(T.State_Weapon_Firing);
					}
				}
				FinishAbility(false);
			}),
			CycleTime,
			false);
		return;
	}

	FinishAbility(false);
}

void USFGameplayAbility_WeaponFire::FireHitscanPellet(
	ASFCharacterBase* Character,
	ASFWeaponActor* WeaponActor,
	const FSFRangedWeaponConfig& Config,
	const FVector& MuzzleLocation,
	const FRotator& AimRotation)
{
	if (!Character)
	{
		return;
	}

	UWorld* World = Character->GetWorld();
	if (!World)
	{
		return;
	}

	// Trace from the eye viewpoint along the (already spread) aim rotation,
	// not the muzzle, so the bullet goes where the crosshair points.
	FVector EyeLocation;
	FRotator EyeRotation;
	Character->GetActorEyesViewPoint(EyeLocation, EyeRotation);

	const FVector TraceStart = EyeLocation;
	const FVector TraceEnd = TraceStart + AimRotation.Vector() * FMath::Max(500.0f, Config.HitscanMaxRange);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(SFWeaponFireHitscan), true, Character);
	Params.bReturnPhysicalMaterial = true;
	if (WeaponActor)
	{
		Params.AddIgnoredActor(WeaponActor);
	}

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, TraceChannel, Params);

	if (bHit)
	{
		ApplyHitscanDamage(Character, Config, Hit, MuzzleLocation);
	}
}

void USFGameplayAbility_WeaponFire::FireProjectilePellet(
	ASFCharacterBase* Character,
	ASFWeaponActor* WeaponActor,
	const FSFRangedWeaponConfig& Config,
	const FVector& MuzzleLocation,
	const FRotator& AimRotation)
{
	if (!Character)
	{
		return;
	}

	TSubclassOf<ASFProjectileBase> ProjectileClass = Config.ProjectileClass
		? Config.ProjectileClass
		: DefaultProjectileClass;
	if (!ProjectileClass)
	{
		return;
	}

	UWorld* World = Character->GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (ASFProjectileBase* Projectile = World->SpawnActor<ASFProjectileBase>(
			ProjectileClass, MuzzleLocation, AimRotation, SpawnParams))
	{
		Projectile->SetSourceActor(Character);
		Projectile->SetBaseDamage(Config.BaseDamage);
		Projectile->SetDamageFalloff(Config.FalloffStartDistance, Config.FalloffEndDistance, Config.MinFalloffMultiplier);

		TSubclassOf<UGameplayEffect> EffectClass = Config.DamageEffectClass
			? Config.DamageEffectClass
			: DefaultDamageEffect;
		Projectile->SetDamageEffect(EffectClass);
	}
}

void USFGameplayAbility_WeaponFire::ApplyHitscanDamage(
	ASFCharacterBase* Character,
	const FSFRangedWeaponConfig& Config,
	const FHitResult& Hit,
	const FVector& MuzzleLocation)
{
	AActor* TargetActor = Hit.GetActor();
	if (!TargetActor)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = Character->GetAbilitySystemComponent();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);

	const TSubclassOf<UGameplayEffect> EffectClass = Config.DamageEffectClass
		? Config.DamageEffectClass
		: DefaultDamageEffect;

	const bool bHeadshot = HeadshotBones.Contains(Hit.BoneName);
	const float Distance = FVector::Dist(MuzzleLocation, Hit.ImpactPoint);

	// Linear falloff (curve hook available on the config struct for designers to extend).
	float FalloffMult = 1.0f;
	if (Config.FalloffEndDistance > Config.FalloffStartDistance)
	{
		if (Distance >= Config.FalloffEndDistance)
		{
			FalloffMult = Config.MinFalloffMultiplier;
		}
		else if (Distance > Config.FalloffStartDistance)
		{
			const float Alpha = (Distance - Config.FalloffStartDistance) /
				FMath::Max(1.0f, (Config.FalloffEndDistance - Config.FalloffStartDistance));
			FalloffMult = FMath::Lerp(1.0f, Config.MinFalloffMultiplier, Alpha);
		}
	}

	const float HeadshotMult = bHeadshot ? Config.HeadshotMultiplier : 1.0f;
	const float DamageBase = Config.BaseDamage > 0.0f ? Config.BaseDamage : DefaultDamage;
	const float FinalDamage = DamageBase * FalloffMult * HeadshotMult;

	if (SourceASC && TargetASC && EffectClass)
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

	// Last-damaging-character attribution for enemies.
	if (ASFEnemyCharacter* HitEnemy = Cast<ASFEnemyCharacter>(TargetActor))
	{
		HitEnemy->SetLastDamagingCharacter(Character);
	}

	// Hit cue on target ASC so the cinematic combat layer reacts (sparks, blood, etc.).
	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
	if (TargetASC && Tags.Cue_Hit_Impact.IsValid())
	{
		FGameplayCueParameters CueParams;
		CueParams.Instigator = Character;
		CueParams.EffectCauser = Character;
		CueParams.Location = Hit.ImpactPoint;
		CueParams.Normal = Hit.ImpactNormal;
		CueParams.PhysicalMaterial = Hit.PhysMaterial;
		CueParams.RawMagnitude = FinalDamage;
		TargetASC->ExecuteGameplayCue(Tags.Cue_Hit_Impact, CueParams);
	}
}

void USFGameplayAbility_WeaponFire::DispatchMuzzleAndTracerCues(
	ASFCharacterBase* Character,
	ASFWeaponActor* WeaponActor,
	const FSFRangedWeaponConfig& Config,
	const FVector& MuzzleLocation,
	const FRotator& MuzzleRotation,
	const FVector& EndLocation)
{
	if (!Character)
	{
		return;
	}

	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();

	auto FireCue = [&](const FGameplayTag& Tag, const FVector& Location, const FVector& Normal)
	{
		if (!Tag.IsValid())
		{
			return;
		}

		FGameplayCueParameters Params;
		Params.Instigator = Character;
		Params.EffectCauser = WeaponActor ? Cast<AActor>(WeaponActor) : Cast<AActor>(Character);
		Params.Location = Location;
		Params.Normal = Normal;
		Params.RawMagnitude = 1.0f;

		if (ASC)
		{
			ASC->ExecuteGameplayCue(Tag, Params);
		}
		else if (UGameplayCueManager* CueMgr = UAbilitySystemGlobals::Get().GetGameplayCueManager())
		{
			CueMgr->HandleGameplayCue(Character, Tag, EGameplayCueEvent::Executed, Params);
		}
	};

	const FGameplayTag MuzzleTag = Config.MuzzleFlashCueOverride.IsValid() ? Config.MuzzleFlashCueOverride : Tags.Cue_Weapon_MuzzleFlash;
	const FGameplayTag TracerTag = Config.TracerCueOverride.IsValid() ? Config.TracerCueOverride : Tags.Cue_Weapon_Tracer;
	const FGameplayTag ShellTag = Config.ShellEjectCueOverride.IsValid() ? Config.ShellEjectCueOverride : Tags.Cue_Weapon_ShellEject;

	FireCue(MuzzleTag, MuzzleLocation, MuzzleRotation.Vector());
	FireCue(TracerTag, MuzzleLocation, (EndLocation - MuzzleLocation).GetSafeNormal());
	FireCue(ShellTag, MuzzleLocation, MuzzleRotation.Vector());
}

void USFGameplayAbility_WeaponFire::ApplyRecoilAndShake(
	ASFCharacterBase* Character,
	const FSFRangedWeaponConfig& Config)
{
	if (!Character)
	{
		return;
	}

	if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
	{
		if (Config.VerticalRecoil > 0.0f)
		{
			// Negative pitch input pushes the camera up (matches default UE pitch convention).
			PC->AddPitchInput(-Config.VerticalRecoil);
		}
		if (Config.HorizontalRecoil > 0.0f)
		{
			const float Sign = FMath::FRandRange(-1.0f, 1.0f) >= 0.0f ? 1.0f : -1.0f;
			PC->AddYawInput(Config.HorizontalRecoil * Sign);
		}
	}

	TSubclassOf<UCameraShakeBase> ShakeClass = Config.FireCameraShake ? Config.FireCameraShake : DefaultCameraShake;
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

bool USFGameplayAbility_WeaponFire::IsAimingDownSights(ASFCharacterBase* Character) const
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

FRotator USFGameplayAbility_WeaponFire::ApplyConeSpread(const FRotator& BaseRotation, float HalfAngleDegrees) const
{
	if (HalfAngleDegrees <= 0.0f)
	{
		return BaseRotation;
	}

	const FVector BaseDir = BaseRotation.Vector();
	const FVector Spread = FMath::VRandCone(BaseDir, FMath::DegreesToRadians(HalfAngleDegrees));
	return Spread.Rotation();
}

bool USFGameplayAbility_WeaponFire::ResolveRangedContext(
	ASFCharacterBase* Character,
	USFEquipmentComponent*& OutEquipment,
	USFWeaponData*& OutWeaponData,
	ASFWeaponActor*& OutWeaponActor,
	FSFRangedWeaponConfig& OutConfig) const
{
	OutEquipment = nullptr;
	OutWeaponData = nullptr;
	OutWeaponActor = nullptr;

	if (!Character)
	{
		return false;
	}

	OutEquipment = Character->GetEquipmentComponent();
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
	OutConfig = OutWeaponData->RangedConfig;
	return true;
}

bool USFGameplayAbility_WeaponFire::TryConsumeAmmo(
	USFEquipmentComponent* Equipment,
	const FSFRangedWeaponConfig& Config) const
{
	if (!Equipment)
	{
		return true;
	}

	USFWeaponData* WeaponData = Equipment->GetCurrentWeaponData();
	if (!WeaponData || WeaponData->AmmoConfig.ClipSize <= 0)
	{
		return true; // infinite-ammo weapon (e.g. melee, energy beam without clip)
	}

	const int32 Cost = FMath::Max(1, WeaponData->AmmoConfig.AmmoPerShot);

	FSFWeaponInstanceData CurrentInstance = Equipment->GetCurrentWeaponInstance();
	if (CurrentInstance.AmmoInClip < Cost)
	{
		return false;
	}

	CurrentInstance.AmmoInClip -= Cost;

	// In-place update — does not re-equip or re-grant abilities.
	Equipment->UpdateActiveWeaponInstance(CurrentInstance);
	return true;
}

void USFGameplayAbility_WeaponFire::HandleEmptyClick(
	ASFCharacterBase* Character,
	const FSFRangedWeaponConfig& Config)
{
	if (!Character)
	{
		return;
	}

	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();

	if (UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent())
	{
		if (Tags.Cue_Weapon_Empty.IsValid())
		{
			FGameplayCueParameters Params;
			Params.Instigator = Character;
			Params.Location = Character->GetActorLocation();
			ASC->ExecuteGameplayCue(Tags.Cue_Weapon_Empty, Params);
		}
	}

	if (Config.EmptyClickSound)
	{
		UGameplayStatics::PlaySoundAtLocation(Character, Config.EmptyClickSound, Character->GetActorLocation());
	}

	if (Config.EmptyClickMontage)
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			if (UAnimInstance* Anim = Mesh->GetAnimInstance())
			{
				Anim->Montage_Play(Config.EmptyClickMontage, 1.0f);
			}
		}
	}
}
