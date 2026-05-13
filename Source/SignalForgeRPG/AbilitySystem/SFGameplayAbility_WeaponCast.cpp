#include "AbilitySystem/SFGameplayAbility_WeaponCast.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraComponent.h"
#include "Characters/SFCharacterBase.h"
#include "Combat/SFProjectileBase.h"
#include "Combat/SFWeaponActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SFEquipmentComponent.h"
#include "Core/SignalForgeGameplayTags.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogSFWeaponCast, Log, All);

USFGameplayAbility_WeaponCast::USFGameplayAbility_WeaponCast()
{
	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();

	InputTag = Tags.Input_PrimaryFire;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(Tags.Ability_Weapon_Cast);
	AssetTags.AddTag(Tags.Ability_Weapon_PrimaryFire);
	SetAssetTags(AssetTags);

	// Block reload / ADS / other primary fires while casting -- mirrors WeaponMelee / WeaponFire.
	ActivationBlockedTags.AddTag(Tags.State_Weapon_Reloading);
	ActivationBlockedTags.AddTag(Tags.State_Weapon_Switching);
	ActivationBlockedTags.AddTag(Tags.State_Weapon_Casting);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

bool USFGameplayAbility_WeaponCast::CanActivateAbility(
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

	ASFCharacterBase* Character = nullptr;
	USFEquipmentComponent* Equipment = nullptr;
	USFWeaponData* WeaponData = nullptr;
	if (!ResolveContext(Character, Equipment, WeaponData))
	{
		return false;
	}

	return ValidateCasterConfig(WeaponData->CasterConfig);
}

void USFGameplayAbility_WeaponCast::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	CachedHandle = Handle;
	CachedActorInfo = ActorInfo;
	CachedActivationInfo = ActivationInfo;
	bReleaseFired = false;
	bIsCharging = false;
	bAppliedCastingTag = false;
	bAppliedChargingTag = false;
	ChargeStartSeconds = 0.0;
	ActiveStepIndex = INDEX_NONE;
	ActiveReleaseSection = NAME_None;
	ActiveProjectileOverride = nullptr;
	ActiveStepScaleMul = 1.0f;

	ASFCharacterBase* Character = nullptr;
	USFEquipmentComponent* Equipment = nullptr;
	USFWeaponData* WeaponData = nullptr;
	if (!ResolveContext(Character, Equipment, WeaponData) || !ValidateCasterConfig(WeaponData->CasterConfig))
	{
		UE_LOG(LogSFWeaponCast, Warning, TEXT("WeaponCast: ResolveContext or ValidateCasterConfig failed -- ending."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CachedCharacter = Character;
	CachedEquipment = Equipment;

	const FSFCasterWeaponConfig& Config = WeaponData->CasterConfig;

	// Commit cooldown immediately so input spam doesn't queue duplicate casts. Mana is committed
	// at release time so cancelled / interrupted casts cost nothing.
	if (!CommitAbilityCooldown(Handle, ActorInfo, ActivationInfo, /*ForceCooldown=*/false))
	{
		UE_LOG(LogSFWeaponCast, Warning, TEXT("WeaponCast: CommitAbilityCooldown failed."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Resolve which combo step we're playing. Falls back to single-montage mode when CastComboSteps
	// is empty. Commit advance + LastCastWorldTime immediately so spam during cancel-windows still
	// advances the chain (same pattern WeaponMelee uses for swings).
	UAnimMontage* StepMontage = nullptr;
	FName ChargeSection = Config.ChargeLoopSectionName;
	FName ReleaseSection = Config.ReleaseSectionName;
	TSubclassOf<ASFProjectileBase> StepProjectileOverride = nullptr;
	float StepScaleMul = 1.0f;
	ActiveStepIndex = ResolveAndCommitComboStep(Config, Equipment, StepMontage, ChargeSection, ReleaseSection, StepProjectileOverride, StepScaleMul);
	ActiveReleaseSection = ReleaseSection;
	ActiveProjectileOverride = StepProjectileOverride;
	ActiveStepScaleMul = StepScaleMul;

	if (!StepMontage)
	{
		UE_LOG(LogSFWeaponCast, Warning, TEXT("WeaponCast: no montage resolved (combo empty and CastMontage null?). Ending."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
	AddStateTag(Tags.State_Weapon_Casting, bAppliedCastingTag);

	const bool bStartCharging = Config.bSupportsCharge;
	if (!StartMontage(Config, Character, bStartCharging, StepMontage, ChargeSection))
	{
		UE_LOG(LogSFWeaponCast, Warning, TEXT("WeaponCast: StartMontage failed."));
		FinishAbility(true);
		return;
	}

	BindReleaseEvent();

	if (bStartCharging)
	{
		bIsCharging = true;
		ChargeStartSeconds = Character->GetWorld()->GetTimeSeconds();
		AddStateTag(Tags.State_Weapon_Charging, bAppliedChargingTag);
		BindInputRelease();
		StartChargeVFX(Character, Equipment, Config);

		// Auto-release after MaxChargeSeconds so the player can't hold forever.
		if (Config.MaxChargeSeconds > 0.0f)
		{
			Character->GetWorldTimerManager().SetTimer(
				ChargeTimeoutHandle,
				FTimerDelegate::CreateUObject(this, &USFGameplayAbility_WeaponCast::OnChargeTimeoutFired),
				Config.MaxChargeSeconds,
				/*bLoop=*/false);
		}
	}
}

void USFGameplayAbility_WeaponCast::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	StopChargeVFX();

	if (ASFCharacterBase* Character = CachedCharacter.Get())
	{
		Character->GetWorldTimerManager().ClearTimer(ChargeTimeoutHandle);
	}

	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
	RemoveStateTag(Tags.State_Weapon_Charging, bAppliedChargingTag);
	RemoveStateTag(Tags.State_Weapon_Casting, bAppliedCastingTag);

	MontageTask = nullptr;
	ReleaseEventTask = nullptr;
	InputReleaseTask = nullptr;
	CachedCharacter.Reset();
	CachedEquipment.Reset();
	CachedMontage.Reset();
	CachedActorInfo = nullptr;
	ActiveStepIndex = INDEX_NONE;
	ActiveReleaseSection = NAME_None;
	ActiveProjectileOverride = nullptr;
	ActiveStepScaleMul = 1.0f;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

bool USFGameplayAbility_WeaponCast::ResolveContext(
	ASFCharacterBase*& OutCharacter,
	USFEquipmentComponent*& OutEquipment,
	USFWeaponData*& OutWeaponData) const
{
	OutCharacter = nullptr;
	OutEquipment = nullptr;
	OutWeaponData = nullptr;

	const FGameplayAbilityActorInfo* ActorInfo = CurrentActorInfo;
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		return false;
	}

	OutCharacter = Cast<ASFCharacterBase>(ActorInfo->AvatarActor.Get());
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
	return OutWeaponData != nullptr;
}

bool USFGameplayAbility_WeaponCast::ValidateCasterConfig(const FSFCasterWeaponConfig& Config) const
{
	if (!Config.ProjectileClass)
	{
		return false;
	}
	// A caster weapon is valid if it has at least one montage source: a combo or a legacy single montage.
	if (Config.CastComboSteps.Num() > 0)
	{
		for (const FSFCasterMontageStep& Step : Config.CastComboSteps)
		{
			if (!Step.Montage) { return false; }
		}
		return true;
	}
	return Config.CastMontage != nullptr;
}

bool USFGameplayAbility_WeaponCast::StartMontage(
	const FSFCasterWeaponConfig& Config,
	ASFCharacterBase* Character,
	bool bStartCharging,
	UAnimMontage* MontageToPlay,
	FName ChargeSection)
{
	if (!MontageToPlay || !Character)
	{
		return false;
	}

	const FName StartSection = bStartCharging ? ChargeSection : NAME_None;

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		MontageToPlay,
		/*Rate=*/1.0f,
		StartSection);

	if (!MontageTask)
	{
		return false;
	}

	MontageTask->OnCompleted.AddDynamic(this, &USFGameplayAbility_WeaponCast::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &USFGameplayAbility_WeaponCast::OnMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &USFGameplayAbility_WeaponCast::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &USFGameplayAbility_WeaponCast::OnMontageCancelled);
	MontageTask->ReadyForActivation();

	CachedMontage = MontageToPlay;
	return true;
}

int32 USFGameplayAbility_WeaponCast::ResolveAndCommitComboStep(
	const FSFCasterWeaponConfig& Config,
	USFEquipmentComponent* Equipment,
	UAnimMontage*& OutMontage,
	FName& OutChargeSection,
	FName& OutReleaseSection,
	TSubclassOf<ASFProjectileBase>& OutProjectileOverride,
	float& OutScaleMul) const
{
	OutMontage = nullptr;
	OutChargeSection = Config.ChargeLoopSectionName;
	OutReleaseSection = Config.ReleaseSectionName;
	OutProjectileOverride = nullptr;
	OutScaleMul = 1.0f;

	// Single-montage mode (legacy) -- no combo array.
	if (Config.CastComboSteps.Num() == 0)
	{
		OutMontage = Config.CastMontage;
		return INDEX_NONE;
	}

	if (!Equipment)
	{
		OutMontage = Config.CastComboSteps[0].Montage;
		return 0;
	}

	UWorld* World = Equipment->GetWorld();
	const float Now = World ? World->GetTimeSeconds() : 0.0f;

	FSFWeaponInstanceData Instance = Equipment->GetCurrentWeaponInstance();
	int32 StoredStep = Instance.CasterComboStep;

	// Idle-reset: if the player has been silent longer than ComboResetSeconds, restart from 0.
	if (Config.CastComboResetSeconds > 0.0f && Instance.LastCastWorldTime > 0.0f
		&& (Now - Instance.LastCastWorldTime) > Config.CastComboResetSeconds)
	{
		StoredStep = 0;
	}

	const int32 NumSteps = Config.CastComboSteps.Num();
	const int32 ChosenStep = ((StoredStep % NumSteps) + NumSteps) % NumSteps;
	const FSFCasterMontageStep& Step = Config.CastComboSteps[ChosenStep];

	OutMontage = Step.Montage;
	if (Step.ChargeLoopSectionOverride != NAME_None)
	{
		OutChargeSection = Step.ChargeLoopSectionOverride;
	}
	if (Step.ReleaseSectionOverride != NAME_None)
	{
		OutReleaseSection = Step.ReleaseSectionOverride;
	}
	OutProjectileOverride = Step.ProjectileOverride;
	OutScaleMul = Step.ProjectileScaleMul;

	// Commit advance + LastCastWorldTime NOW so a follow-up press lands on the next step even
	// if this activation is later interrupted (mirrors WeaponMelee combo commit timing).
	Instance.CasterComboStep = (ChosenStep + 1) % NumSteps;
	Instance.LastCastWorldTime = Now;
	Equipment->UpdateActiveWeaponInstance(Instance);

	UE_LOG(LogSFWeaponCast, Verbose, TEXT("WeaponCast: combo step %d/%d montage=%s"),
		ChosenStep, NumSteps, OutMontage ? *OutMontage->GetName() : TEXT("<null>"));

	return ChosenStep;
}

void USFGameplayAbility_WeaponCast::BindReleaseEvent()
{
	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();

	ReleaseEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		Tags.Event_Cast_Release,
		/*OptionalExternalTarget=*/nullptr,
		/*OnlyTriggerOnce=*/true,
		/*OnlyMatchExact=*/false);

	if (ReleaseEventTask)
	{
		ReleaseEventTask->EventReceived.AddDynamic(this, &USFGameplayAbility_WeaponCast::OnReleaseEventReceived);
		ReleaseEventTask->ReadyForActivation();
	}
}

void USFGameplayAbility_WeaponCast::BindInputRelease()
{
	InputReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, /*bTestAlreadyReleased=*/false);
	if (InputReleaseTask)
	{
		InputReleaseTask->OnRelease.AddDynamic(this, &USFGameplayAbility_WeaponCast::OnInputReleased);
		InputReleaseTask->ReadyForActivation();
	}
}

int32 USFGameplayAbility_WeaponCast::ComputeChargeTierIndex(
	const FSFCasterWeaponConfig& Config,
	float HeldSeconds) const
{
	if (Config.ChargeTiers.Num() == 0)
	{
		return INDEX_NONE;
	}

	// ChargeTiers is expected ascending in HoldSeconds. Walk from highest down so the highest
	// satisfied tier wins.
	for (int32 i = Config.ChargeTiers.Num() - 1; i >= 0; --i)
	{
		if (HeldSeconds + KINDA_SMALL_NUMBER >= Config.ChargeTiers[i].HoldSeconds)
		{
			return i;
		}
	}
	return 0;
}

float USFGameplayAbility_WeaponCast::GetChargeSeconds(const FSFCasterWeaponConfig& Config) const
{
	if (!bIsCharging || ChargeStartSeconds <= 0.0)
	{
		return 0.0f;
	}
	const ASFCharacterBase* Character = CachedCharacter.Get();
	if (!Character)
	{
		return 0.0f;
	}
	const double Held = Character->GetWorld()->GetTimeSeconds() - ChargeStartSeconds;
	const float MaxClamp = Config.MaxChargeSeconds > 0.0f ? Config.MaxChargeSeconds : 9999.0f;
	return FMath::Clamp(static_cast<float>(Held), 0.0f, MaxClamp);
}

void USFGameplayAbility_WeaponCast::TransitionToRelease(const FSFCasterWeaponConfig& Config, FName ReleaseSection)
{
	ASFCharacterBase* Character = CachedCharacter.Get();
	UAnimMontage* Montage = CachedMontage.Get();
	if (!Character || !Montage)
	{
		return;
	}

	USkeletalMeshComponent* Mesh = Character->GetMesh();
	UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return;
	}

	if (ReleaseSection != NAME_None)
	{
		AnimInstance->Montage_JumpToSection(ReleaseSection, Montage);
	}

	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
	RemoveStateTag(Tags.State_Weapon_Charging, bAppliedChargingTag);
	bIsCharging = false;
	StopChargeVFX();

	if (Character)
	{
		Character->GetWorldTimerManager().ClearTimer(ChargeTimeoutHandle);
	}
}

// -----------------------------------------------------------------------------
// Spawn + cost
// -----------------------------------------------------------------------------

FTransform USFGameplayAbility_WeaponCast::ResolveSpawnTransform(
	const ASFCharacterBase* Character,
	USFEquipmentComponent* Equipment,
	const FSFCasterWeaponConfig& Config) const
{
	FTransform Result = FTransform::Identity;
	if (!Character)
	{
		return Result;
	}

	if (Config.bSpawnFromWeaponActor && Equipment)
	{
		if (ASFWeaponActor* WeaponActor = Equipment->GetEquippedWeaponActor())
		{
			if (USkeletalMeshComponent* WeaponSkel = WeaponActor->GetSkeletalMeshComponent())
			{
				if (WeaponSkel->DoesSocketExist(Config.SpawnSocketName))
				{
					return WeaponSkel->GetSocketTransform(Config.SpawnSocketName, RTS_World);
				}
			}
			// Static mesh fallback.
			if (UStaticMeshComponent* WeaponStatic = WeaponActor->GetStaticMeshComponent())
			{
				if (WeaponStatic->DoesSocketExist(Config.SpawnSocketName))
				{
					return WeaponStatic->GetSocketTransform(Config.SpawnSocketName, RTS_World);
				}
			}
		}
	}

	// Fall back to character mesh socket.
	if (USkeletalMeshComponent* CharMesh = Character->GetMesh())
	{
		if (CharMesh->DoesSocketExist(Config.SpawnSocketName))
		{
			return CharMesh->GetSocketTransform(Config.SpawnSocketName, RTS_World);
		}
	}

	// Last resort: actor location + forward offset.
	FVector Eye;
	FRotator EyeRot;
	Character->GetActorEyesViewPoint(Eye, EyeRot);
	Result.SetLocation(Eye + EyeRot.Vector() * 50.0f);
	Result.SetRotation(EyeRot.Quaternion());
	return Result;
}

FVector USFGameplayAbility_WeaponCast::ResolveAimDirection(const ASFCharacterBase* Character) const
{
	if (!Character)
	{
		return FVector::ForwardVector;
	}

	FVector Eye;
	FRotator EyeRot;
	Character->GetActorEyesViewPoint(Eye, EyeRot);
	return EyeRot.Vector();
}

void USFGameplayAbility_WeaponCast::SpawnProjectile(
	ASFCharacterBase* Character,
	USFEquipmentComponent* Equipment,
	const USFWeaponData* WeaponData,
	const FSFCasterWeaponConfig& Config,
	int32 TierIndex,
	TSubclassOf<ASFProjectileBase> ProjectileOverride,
	float StepScaleMul)
{
	const TSubclassOf<ASFProjectileBase> ResolvedProjectileClass = ProjectileOverride ? ProjectileOverride : Config.ProjectileClass;
	if (!Character || !ResolvedProjectileClass)
	{
		return;
	}

	UWorld* World = Character->GetWorld();
	if (!World)
	{
		return;
	}

	const FTransform SpawnTransform = ResolveSpawnTransform(Character, Equipment, Config);
	const FVector SpawnLocation = SpawnTransform.GetLocation();
	const FVector AimDirection = ResolveAimDirection(Character);
	const FRotator SpawnRotation = AimDirection.Rotation();

	// Tier multipliers.
	float DamageMul = 1.0f;
	float SpeedMul = 1.0f;
	float ProjectileScale = 1.0f;
	if (Config.ChargeTiers.IsValidIndex(TierIndex))
	{
		const FSFCasterChargeTier& Tier = Config.ChargeTiers[TierIndex];
		DamageMul = Tier.DamageMultiplier;
		SpeedMul = Tier.SpeedMultiplier;
		ProjectileScale = Tier.ProjectileScale;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// SpawnActorDeferred so we can mutate InitialSpeed / scale before BeginPlay runs.
	ASFProjectileBase* Projectile = World->SpawnActorDeferred<ASFProjectileBase>(
		ResolvedProjectileClass,
		FTransform(SpawnRotation, SpawnLocation),
		Character,
		Character,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

	if (!Projectile)
	{
		UE_LOG(LogSFWeaponCast, Warning, TEXT("WeaponCast: SpawnActorDeferred failed for projectile."));
		return;
	}

	Projectile->SetSourceActor(Character);

	// Speed scaling pre-BeginPlay: the projectile reads its InitialSpeed in BeginPlay (which
	// runs at FinishSpawning), so we must call this before FinishSpawning below.
	if (SpeedMul != 1.0f)
	{
		Projectile->SetSpeedMultiplier(SpeedMul);
	}

	// Apply uniform scale at spawn (projectile mesh gets bigger at higher charge tiers). Charge-tier
	// scale and per-step combo scale stack multiplicatively.
	const float FinalScale = ProjectileScale * StepScaleMul;
	FTransform FinishTransform(SpawnRotation, SpawnLocation);
	if (!FMath::IsNearlyEqual(FinalScale, 1.0f))
	{
		FinishTransform.SetScale3D(FVector(FinalScale));
	}

	Projectile->FinishSpawning(FinishTransform);

	// Damage multiplier is currently sourced from the projectile's DA. TODO: route through
	// USFCombatComponent::ProcessProjectileHit so damage modifiers / resistances apply uniformly
	// across melee, ranged, beam, and caster. For now the multiplier is informational only --
	// designers can scale damage by authoring per-projectile DA variants.
	(void)WeaponData;
	(void)DamageMul;

	// Release cue at the spawn point along the aim vector.
	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
	const FGameplayTag ReleaseCue = Config.CastReleaseCueOverride.IsValid()
		? Config.CastReleaseCueOverride
		: Tags.Cue_Weapon_CastRelease;

	if (ReleaseCue.IsValid())
	{
		FGameplayCueParameters CueParams;
		CueParams.Location = SpawnLocation;
		CueParams.Normal = AimDirection;
		CueParams.SourceObject = Equipment ? Cast<UObject>(Equipment->GetEquippedWeaponActor()) : nullptr;
		CueParams.Instigator = Character;

		if (UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent())
		{
			ASC->ExecuteGameplayCue(ReleaseCue, CueParams);
		}
	}

	// Optional camera shake.
	if (Config.CastCameraShake)
	{
		if (APlayerController* PC = Character->GetController<APlayerController>())
		{
			PC->ClientStartCameraShake(Config.CastCameraShake, 1.0f);
		}
	}

	UE_LOG(LogSFWeaponCast, Verbose,
		TEXT("WeaponCast: spawned '%s' step=%d tier=%d speedMul=%.2f scale=%.2f at %s"),
		*Projectile->GetName(), ActiveStepIndex, TierIndex, SpeedMul, FinalScale, *SpawnLocation.ToCompactString());
}

void USFGameplayAbility_WeaponCast::ApplyManaCost(
	ASFCharacterBase* Character,
	const FSFCasterWeaponConfig& Config,
	int32 TierIndex)
{
	if (!Character || !Config.ManaCostEffect)
	{
		return;
	}

	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	float TotalCost = Config.ManaCost;
	if (Config.ChargeTiers.IsValidIndex(TierIndex))
	{
		TotalCost += Config.ChargeTiers[TierIndex].ExtraManaCost;
	}

	if (TotalCost <= 0.0f)
	{
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(Character);

	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Config.ManaCostEffect, GetAbilityLevel(), Context);
	if (Spec.IsValid())
	{
		const FGameplayTag SetByCallerTag = Config.ManaCostSetByCallerTag.IsValid()
			? Config.ManaCostSetByCallerTag
			: FGameplayTag::RequestGameplayTag(FName("Data.ManaCost"), /*ErrorIfNotFound=*/false);

		if (SetByCallerTag.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(SetByCallerTag, TotalCost);
		}
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
	}
}

// -----------------------------------------------------------------------------
// VFX
// -----------------------------------------------------------------------------

void USFGameplayAbility_WeaponCast::StartChargeVFX(
	ASFCharacterBase* Character,
	USFEquipmentComponent* Equipment,
	const FSFCasterWeaponConfig& Config)
{
	if (!Config.ChargeLoopVFX || !Character)
	{
		return;
	}

	USceneComponent* AttachTo = nullptr;
	FName AttachSocket = Config.SpawnSocketName;

	if (Config.bSpawnFromWeaponActor && Equipment)
	{
		if (ASFWeaponActor* WeaponActor = Equipment->GetEquippedWeaponActor())
		{
			if (USkeletalMeshComponent* WeaponSkel = WeaponActor->GetSkeletalMeshComponent())
			{
				if (WeaponSkel->DoesSocketExist(AttachSocket))
				{
					AttachTo = WeaponSkel;
				}
			}
			if (!AttachTo)
			{
				if (UStaticMeshComponent* WeaponStatic = WeaponActor->GetStaticMeshComponent())
				{
					if (WeaponStatic->DoesSocketExist(AttachSocket))
					{
						AttachTo = WeaponStatic;
					}
				}
			}
		}
	}

	if (!AttachTo)
	{
		if (USkeletalMeshComponent* CharMesh = Character->GetMesh())
		{
			AttachTo = CharMesh;
			if (!CharMesh->DoesSocketExist(AttachSocket))
			{
				AttachSocket = NAME_None;
			}
		}
	}

	if (!AttachTo)
	{
		return;
	}

	UNiagaraComponent* Spawned = UNiagaraFunctionLibrary::SpawnSystemAttached(
		Config.ChargeLoopVFX,
		AttachTo,
		AttachSocket,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::SnapToTarget,
		/*bAutoDestroy=*/false);

	ActiveChargeVFX = Spawned;
}

void USFGameplayAbility_WeaponCast::StopChargeVFX()
{
	if (UNiagaraComponent* VFX = ActiveChargeVFX.Get())
	{
		VFX->Deactivate();
		VFX->DestroyComponent();
	}
	ActiveChargeVFX.Reset();
}

void USFGameplayAbility_WeaponCast::AddStateTag(const FGameplayTag& Tag, bool& bFlag)
{
	if (bFlag || !Tag.IsValid())
	{
		return;
	}
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(Tag);
		bFlag = true;
	}
}

void USFGameplayAbility_WeaponCast::RemoveStateTag(const FGameplayTag& Tag, bool& bFlag)
{
	if (!bFlag || !Tag.IsValid())
	{
		return;
	}
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(Tag);
	}
	bFlag = false;
}

// -----------------------------------------------------------------------------
// Task callbacks
// -----------------------------------------------------------------------------

void USFGameplayAbility_WeaponCast::OnMontageCompleted()
{
	FinishAbility(false);
}

void USFGameplayAbility_WeaponCast::OnMontageBlendOut()
{
	// Treat blend-out the same as completion (the release notify has already fired by the time
	// the montage blends out).
	FinishAbility(false);
}

void USFGameplayAbility_WeaponCast::OnMontageInterrupted()
{
	FinishAbility(true);
}

void USFGameplayAbility_WeaponCast::OnMontageCancelled()
{
	FinishAbility(true);
}

void USFGameplayAbility_WeaponCast::OnReleaseEventReceived(FGameplayEventData Payload)
{
	(void)Payload;

	if (bReleaseFired)
	{
		return;
	}
	bReleaseFired = true;

	ASFCharacterBase* Character = CachedCharacter.Get();
	USFEquipmentComponent* Equipment = CachedEquipment.Get();
	if (!Character || !Equipment)
	{
		return;
	}

	USFWeaponData* WeaponData = Equipment->GetCurrentWeaponData();
	if (!WeaponData)
	{
		return;
	}

	const FSFCasterWeaponConfig& Config = WeaponData->CasterConfig;
	const float Held = GetChargeSeconds(Config);
	const int32 TierIndex = Config.bSupportsCharge ? ComputeChargeTierIndex(Config, Held) : INDEX_NONE;

	// Commit the mana cost on release so cancelled casts cost nothing.
	ApplyManaCost(Character, Config, TierIndex);

	SpawnProjectile(Character, Equipment, WeaponData, Config, TierIndex, ActiveProjectileOverride, ActiveStepScaleMul);

	// Charging is over (release notify can fire from a non-charge cast too -- just a no-op here).
	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
	RemoveStateTag(Tags.State_Weapon_Charging, bAppliedChargingTag);
	bIsCharging = false;
	StopChargeVFX();
}

void USFGameplayAbility_WeaponCast::OnInputReleased(float TimeHeld)
{
	(void)TimeHeld;

	if (!bIsCharging)
	{
		return;
	}

	USFEquipmentComponent* Equipment = CachedEquipment.Get();
	if (!Equipment)
	{
		return;
	}
	USFWeaponData* WeaponData = Equipment->GetCurrentWeaponData();
	if (!WeaponData)
	{
		return;
	}

	TransitionToRelease(WeaponData->CasterConfig, ActiveReleaseSection);
}

void USFGameplayAbility_WeaponCast::OnChargeTimeoutFired()
{
	if (!bIsCharging)
	{
		return;
	}

	USFEquipmentComponent* Equipment = CachedEquipment.Get();
	if (!Equipment)
	{
		return;
	}
	USFWeaponData* WeaponData = Equipment->GetCurrentWeaponData();
	if (!WeaponData)
	{
		return;
	}

	TransitionToRelease(WeaponData->CasterConfig, ActiveReleaseSection);
}

void USFGameplayAbility_WeaponCast::FinishAbility(bool bWasCancelled)
{
	EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, /*bReplicateEndAbility=*/true, bWasCancelled);
}
