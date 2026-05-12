#include "AbilitySystem/SFGameplayAbility_WeaponMelee.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Characters/SFCharacterBase.h"
#include "Combat/SFCombatComponent.h"
#include "Combat/SFHitTypes.h"
#include "Combat/SFWeaponActor.h"
#include "Combat/SFWeaponData.h"
#include "Combat/SFWeaponInstanceTypes.h"
#include "Components/SFEquipmentComponent.h"
#include "Core/SignalForgeGameplayTags.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameplayCueManager.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogSFWeaponMelee, Log, All);

USFGameplayAbility_WeaponMelee::USFGameplayAbility_WeaponMelee()
{
	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();

	// Default to the light lane; the heavy BP subclass overrides Lane + InputTag in its
	// constructor / class defaults. Most projects ship one C++ class and two BP subclasses.
	InputTag = Tags.Input_PrimaryFire;

	FGameplayTagContainer AssetTags;
	// Carry both lane tags so the appropriate one is matched regardless of which BP subclass
	// (light or heavy) is granted. Combined with CancelAbilitiesWithTag below this also lets
	// the light ability cancel a heavy mid-windup and vice-versa.
	AssetTags.AddTag(Tags.Ability_Weapon_MeleeLight);
	AssetTags.AddTag(Tags.Ability_Weapon_MeleeHeavy);
	// Carry the PrimaryFire ability tag too so existing reload / ADS blocked-tag rules
	// that already mention PrimaryFire automatically cover us.
	AssetTags.AddTag(Tags.Ability_Weapon_PrimaryFire);
	SetAssetTags(AssetTags);

	// Cancel sibling fire / melee activations on the same hand so a swing can interrupt
	// e.g. a held beam, and a queued combo press during cancel-window cancels the prior swing.
	CancelAbilitiesWithTag.AddTag(Tags.Ability_Weapon_PrimaryFire);
	CancelAbilitiesWithTag.AddTag(Tags.Ability_Weapon_MeleeLight);
	CancelAbilitiesWithTag.AddTag(Tags.Ability_Weapon_MeleeHeavy);

	// Don't start a new swing while switching weapons. MeleeSwinging is allowed mid-cancel-window
	// because we explicitly cancel ourselves via CancelAbilitiesWithTag above.
	ActivationBlockedTags.AddTag(Tags.State_Weapon_Switching);

	// StaminaCostSetByCallerTag is left invalid by default -- designer points it at whatever
	// SetByCaller tag their stamina-cost GE expects (project has no canonical Data.StaminaCost
	// tag yet). If left invalid, ApplyStaminaCost just commits the GE without magnitude override.

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

bool USFGameplayAbility_WeaponMelee::CanActivateAbility(
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

	// Need at least one montage in the lane we represent. Otherwise this weapon doesn't
	// actually want melee on this input -- e.g. a rifle DA accidentally pointed PrimaryFire
	// at WeaponMelee. Failing CanActivate keeps WeaponFire / WeaponBeam free to handle it.
	const FSFMeleeWeaponConfig& Cfg = WeaponData->MeleeConfig;
	const TArray<TObjectPtr<UAnimMontage>>& LaneMontages =
		(this->Lane == ESFMeleeLane::Heavy) ? Cfg.HeavyComboMontages : Cfg.LightComboMontages;

	return LaneMontages.Num() > 0;
}

void USFGameplayAbility_WeaponMelee::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	CachedHandle = Handle;
	CachedActorInfo = ActorInfo;
	CachedActivationInfo = ActivationInfo;
	bHitLandedThisSwing = false;
	bAppliedSwingingTag = false;
	bSwingingOffhand = false;
	CurrentMontage.Reset();

	ASFCharacterBase* Character = nullptr;
	USFEquipmentComponent* Equipment = nullptr;
	USFWeaponData* WeaponData = nullptr;
	USFCombatComponent* Combat = nullptr;

	if (!ResolveContext(Character, Equipment, WeaponData, Combat))
	{
		UE_LOG(LogSFWeaponMelee, Warning, TEXT("WeaponMelee: missing context, ending."));
		FinishAbility(true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		FinishAbility(true);
		return;
	}

	const FSFMeleeWeaponConfig& Cfg = WeaponData->MeleeConfig;

	// Resolve current combo step from the persisted weapon instance, with timeout-based reset.
	FSFWeaponInstanceData Instance = Equipment->GetCurrentWeaponInstance();
	const float Now = (Character->GetWorld() != nullptr) ? Character->GetWorld()->GetTimeSeconds() : 0.0f;
	int32 StoredStep = (Lane == ESFMeleeLane::Heavy) ? Instance.MeleeHeavyComboStep : Instance.MeleeLightComboStep;

	if (Cfg.ComboResetSeconds > 0.0f && Instance.LastMeleeAttackWorldTime > 0.0f
		&& (Now - Instance.LastMeleeAttackWorldTime) > Cfg.ComboResetSeconds)
	{
		StoredStep = 0;
	}

	ChosenComboStep = StoredStep;
	UAnimMontage* Montage = PickMontageForStep(Cfg, ChosenComboStep);
	if (!Montage)
	{
		UE_LOG(LogSFWeaponMelee, Warning, TEXT("WeaponMelee: lane %d step %d has no montage."),
			(int32)Lane, ChosenComboStep);
		FinishAbility(true);
		return;
	}
	CurrentMontage = Montage;

	// Apply stamina cost (best effort; failure does not block the swing).
	ApplyStaminaCost(Character, WeaponData);

	// Pick which weapon actor "owns" this swing (for paired weapons we alternate hands).
	ChooseSwingingWeaponActor(Equipment, WeaponData);

	// Apply the swinging state tag so reload / ADS / movement systems can react.
	if (ActorInfo->AbilitySystemComponent.IsValid())
	{
		const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
		ActorInfo->AbilitySystemComponent->AddLooseGameplayTag(Tags.State_Weapon_MeleeSwinging);
		bAppliedSwingingTag = true;
	}

	// Subscribe to CombatComponent so we know when a hit actually lands. The AnimNotifyState
	// drives BeginAttackWindow / EndAttackWindow + HandleLightAttackHit / HandleHeavyAttackHit
	// inside the montage itself; we just listen for the result.
	CachedCharacter = Character;
	CachedEquipment = Equipment;
	CachedCombat = Combat;
	BindHitDelegate(Combat);

	// Play the montage.
	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			Montage);

	if (!MontageTask)
	{
		UE_LOG(LogSFWeaponMelee, Warning, TEXT("WeaponMelee: failed to create montage task."));
		FinishAbility(true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &USFGameplayAbility_WeaponMelee::OnMontageEnded);
	MontageTask->OnBlendOut.AddDynamic(this, &USFGameplayAbility_WeaponMelee::OnMontageEnded);
	MontageTask->OnInterrupted.AddDynamic(this, &USFGameplayAbility_WeaponMelee::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &USFGameplayAbility_WeaponMelee::OnMontageCancelled);
	MontageTask->ReadyForActivation();

	// Commit the combo step + attack time NOW so a follow-up swing (during cancel-window)
	// sees the advanced index even if this swing is interrupted before montage end.
	CommitComboStepToInstance(Equipment, Lane, ChosenComboStep);

	UE_LOG(LogSFWeaponMelee, Verbose,
		TEXT("WeaponMelee: lane=%d step=%d montage=%s offhand=%d"),
		(int32)Lane, ChosenComboStep, *Montage->GetName(), bSwingingOffhand ? 1 : 0);
}

void USFGameplayAbility_WeaponMelee::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	UnbindHitDelegate();

	// Make sure the attack window is closed -- if the montage was interrupted before its
	// AnimNotifyState_MeleeWindow End event fired, the CombatComponent would otherwise be
	// stuck mid-window and ignore future swings.
	if (USFCombatComponent* Combat = CachedCombat.Get())
	{
		Combat->EndAttackWindow();
	}

	// Whiff cue if the swing closed without landing anything.
	if (!bHitLandedThisSwing)
	{
		DispatchWhiffCueIfApplicable(CachedCharacter.Get());
	}

	// Remove the swinging tag. Guard against double-removal (mirrors the WeaponBeam
	// double-remove bug we fixed earlier this session).
	if (bAppliedSwingingTag && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
		ActorInfo->AbilitySystemComponent->RemoveLooseGameplayTag(Tags.State_Weapon_MeleeSwinging);
		// Also clear cancel window if some notify left it sticky.
		ActorInfo->AbilitySystemComponent->RemoveLooseGameplayTag(Tags.State_Weapon_MeleeCancelWindow);
		bAppliedSwingingTag = false;
	}

	CachedCharacter.Reset();
	CachedEquipment.Reset();
	CachedCombat.Reset();
	CurrentMontage.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool USFGameplayAbility_WeaponMelee::ResolveContext(
	ASFCharacterBase*& OutCharacter,
	USFEquipmentComponent*& OutEquipment,
	USFWeaponData*& OutWeaponData,
	USFCombatComponent*& OutCombat) const
{
	OutCharacter = nullptr;
	OutEquipment = nullptr;
	OutWeaponData = nullptr;
	OutCombat = nullptr;

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

	OutCombat = OutCharacter->GetCombatComponent();
	return OutCombat != nullptr;
}

UAnimMontage* USFGameplayAbility_WeaponMelee::PickMontageForStep(
	const FSFMeleeWeaponConfig& MeleeConfig, int32 Step) const
{
	const TArray<TObjectPtr<UAnimMontage>>& LaneMontages =
		(this->Lane == ESFMeleeLane::Heavy) ? MeleeConfig.HeavyComboMontages : MeleeConfig.LightComboMontages;

	if (LaneMontages.Num() == 0)
	{
		return nullptr;
	}

	const int32 Clamped = FMath::Clamp(Step, 0, LaneMontages.Num() - 1);
	return LaneMontages[Clamped].Get();
}

void USFGameplayAbility_WeaponMelee::ApplyStaminaCost(ASFCharacterBase* Character, const USFWeaponData* WeaponData)
{
	if (!Character || !WeaponData || !StaminaCostEffect)
	{
		// Designer didn't set a stamina-cost GE -- that's fine, weapon swings free.
		return;
	}

	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	const float Magnitude = (Lane == ESFMeleeLane::Heavy)
		? WeaponData->CombatTuning.HeavyAttackStaminaCost
		: WeaponData->CombatTuning.LightAttackStaminaCost;

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(Character);

	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(StaminaCostEffect, 1.0f, Context);
	if (!Spec.IsValid())
	{
		return;
	}

	if (StaminaCostSetByCallerTag.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(StaminaCostSetByCallerTag, Magnitude);
	}

	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

ASFWeaponActor* USFGameplayAbility_WeaponMelee::ChooseSwingingWeaponActor(
	USFEquipmentComponent* Equipment, USFWeaponData* WeaponData)
{
	if (!Equipment || !WeaponData)
	{
		return nullptr;
	}

	// Default: mainhand actor.
	ASFWeaponActor* MainActor = Equipment->GetEquippedWeaponActor();

	if (!WeaponData->bIsPairedWeapon)
	{
		bSwingingOffhand = false;
		return MainActor;
	}

	// Paired weapon: alternate hands using LastSwingHandIndex (1 = mainhand was last, swing offhand;
	// 0 = offhand was last, swing mainhand). Default on a fresh instance is 1 so the first swing
	// is mainhand.
	FSFWeaponInstanceData Instance = Equipment->GetCurrentWeaponInstance();
	const bool bWantOffhand = (Instance.LastSwingHandIndex == 0);

	// NOTE: an offhand weapon-actor slot does not yet exist in EquippedWeaponActors -- that lands
	// in the next TODO step (offhand spawn). For now ChooseSwingingWeaponActor records intent so
	// the cue gets the correct SourceObject as soon as the offhand actor is wired up. The actual
	// trace still happens off the mainhand mesh via USFCombatComponent::GetWeaponTracePoints.
	bSwingingOffhand = bWantOffhand;

	// TODO(paired-trace): once Equipment exposes an offhand slot, look it up here and return that
	// actor when bWantOffhand. Until then both hands' cues attach to the mainhand actor.
	return MainActor;
}

void USFGameplayAbility_WeaponMelee::BindHitDelegate(USFCombatComponent* Combat)
{
	if (!Combat)
	{
		return;
	}

	// FScriptMulticastDelegate::AddDynamic returns void; cleanup is by RemoveDynamic(this, &Fn).
	Combat->OnHitDelivered.AddDynamic(this, &USFGameplayAbility_WeaponMelee::OnHitDelivered);
}

void USFGameplayAbility_WeaponMelee::UnbindHitDelegate()
{
	if (USFCombatComponent* Combat = CachedCombat.Get())
	{
		Combat->OnHitDelivered.RemoveDynamic(this, &USFGameplayAbility_WeaponMelee::OnHitDelivered);
	}
}

void USFGameplayAbility_WeaponMelee::OnHitDelivered(const FSFHitData& HitData, const FSFResolvedHit& ResolvedHit)
{
	bHitLandedThisSwing = true;

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

	const FSFMeleeWeaponConfig& Cfg = WeaponData->MeleeConfig;
	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();

	// Dispatch the hit cue. CombatComponent already runs its own cinematic feedback pipeline
	// (camera shake / slowmo) via TriggerHitFeedback; we add the weapon-specific Niagara/SFX
	// cue here so each weapon can ship its own impact VFX.
	const FGameplayTag CueTag = Cfg.HitCueOverride.IsValid() ? Cfg.HitCueOverride : Tags.Cue_Weapon_MeleeHit;

	if (UAbilitySystemComponent* ASC = (CachedActorInfo ? CachedActorInfo->AbilitySystemComponent.Get() : nullptr))
	{
		FGameplayCueParameters Params;
		Params.Instigator = Character;
		Params.EffectCauser = Character;
		Params.Location = HitData.HitResult.ImpactPoint;
		Params.Normal = HitData.HitResult.ImpactNormal;
		Params.RawMagnitude = 1.0f;
		// Mirror the beam pattern: pass the weapon actor through SourceObject so cue notify
		// Blueprints can attach VFX directly to the weapon's muzzle / blade socket. For paired
		// weapons swinging offhand, dispatch with the offhand actor so VFX spawns on the right
		// hand.
		ASFWeaponActor* WeaponActor = bSwingingOffhand
			? Equipment->GetEquippedOffhandWeaponActor()
			: Equipment->GetEquippedWeaponActor();
		if (!WeaponActor)
		{
			WeaponActor = Equipment->GetEquippedWeaponActor(); // fallback to mainhand
		}
		if (WeaponActor)
		{
			Params.SourceObject = WeaponActor;
		}
		ASC->ExecuteGameplayCue(CueTag, Params);
	}

	// NOTE: USFCombatComponent::TriggerHitFeedback (called internally during hit resolution)
	// already drives hitstop via its per-attack-type feedback profile. We intentionally don't
	// re-apply hitstop here -- MeleeConfig.HitstopSeconds / HitstopTimeDilation are kept as
	// weapon-data hints so designers can author per-weapon feedback profiles on CombatComponent.

	// Camera shake (instigator only).
	if (Cfg.HitCameraShake)
	{
		if (APawn* AsPawn = Cast<APawn>(Character))
		{
			if (APlayerController* PC = Cast<APlayerController>(AsPawn->GetController()))
			{
				PC->ClientStartCameraShake(Cfg.HitCameraShake, 1.0f);
			}
		}
	}

	// Apply hit-reaction GE to the victim (poise / stagger / knockback).
	if (Cfg.HitReactionEffect && HitData.TargetActor)
	{
		UAbilitySystemComponent* TargetASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitData.TargetActor);
		UAbilitySystemComponent* SourceASC = Character->GetAbilitySystemComponent();
		if (TargetASC && SourceASC)
		{
			FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
			Context.AddSourceObject(Character);
			Context.AddInstigator(Character, Character);

			FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(Cfg.HitReactionEffect, 1.0f, Context);
			if (Spec.IsValid())
			{
				SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
			}
		}
	}
}

void USFGameplayAbility_WeaponMelee::OnMontageEnded()
{
	FinishAbility(false);
}

void USFGameplayAbility_WeaponMelee::OnMontageCancelled()
{
	FinishAbility(true);
}

void USFGameplayAbility_WeaponMelee::DispatchWhiffCueIfApplicable(ASFCharacterBase* Character)
{
	if (!Character || !CachedActorInfo)
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

	const FSFMeleeWeaponConfig& Cfg = WeaponData->MeleeConfig;
	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
	const FGameplayTag CueTag = Cfg.WhiffCueOverride.IsValid() ? Cfg.WhiffCueOverride : Tags.Cue_Weapon_MeleeWhiff;
	if (!CueTag.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* ASC = CachedActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		return;
	}

	FGameplayCueParameters Params;
	Params.Instigator = Character;
	Params.EffectCauser = Character;
	ASFWeaponActor* WeaponActor = bSwingingOffhand
		? Equipment->GetEquippedOffhandWeaponActor()
		: Equipment->GetEquippedWeaponActor();
	if (!WeaponActor)
	{
		WeaponActor = Equipment->GetEquippedWeaponActor(); // fallback to mainhand
	}
	if (WeaponActor)
	{
		Params.SourceObject = WeaponActor;
		// Best-effort whiff location: weapon muzzle if available, else actor location.
		Params.Location = WeaponActor->GetActorLocation();
	}
	else
	{
		Params.Location = Character->GetActorLocation();
	}
	Params.RawMagnitude = 1.0f;

	ASC->ExecuteGameplayCue(CueTag, Params);
}

void USFGameplayAbility_WeaponMelee::CommitComboStepToInstance(
	USFEquipmentComponent* Equipment, ESFMeleeLane SwingLane, int32 Step)
{
	if (!Equipment)
	{
		return;
	}

	USFWeaponData* WeaponData = Equipment->GetCurrentWeaponData();
	if (!WeaponData)
	{
		return;
	}

	FSFWeaponInstanceData Instance = Equipment->GetCurrentWeaponInstance();
	const FSFMeleeWeaponConfig& Cfg = WeaponData->MeleeConfig;

	const TArray<TObjectPtr<UAnimMontage>>& LaneMontages =
		(SwingLane == ESFMeleeLane::Heavy) ? Cfg.HeavyComboMontages : Cfg.LightComboMontages;

	const int32 LaneLen = FMath::Max(1, LaneMontages.Num());
	const int32 NextStep = (Step + 1) % LaneLen;

	if (SwingLane == ESFMeleeLane::Heavy)
	{
		Instance.MeleeHeavyComboStep = NextStep;
	}
	else
	{
		Instance.MeleeLightComboStep = NextStep;
	}

	if (UWorld* World = Equipment->GetWorld())
	{
		Instance.LastMeleeAttackWorldTime = World->GetTimeSeconds();
	}

	// Flip the swing-hand index so paired weapons alternate next time. 1 = mainhand just swung,
	// 0 = offhand just swung.
	if (WeaponData->bIsPairedWeapon)
	{
		Instance.LastSwingHandIndex = bSwingingOffhand ? 0 : 1;
	}

	Equipment->UpdateActiveWeaponInstance(Instance);
}

void USFGameplayAbility_WeaponMelee::FinishAbility(bool bWasCancelled)
{
	if (!IsActive())
	{
		return;
	}

	EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, /*bReplicateEndAbility=*/true, bWasCancelled);
}
