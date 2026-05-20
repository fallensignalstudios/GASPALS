#include "AbilitySystem/SFGameplayAbility_Heal.h"

#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Characters/SFCharacterBase.h"
#include "Core/SFAttributeSetBase.h"
#include "Core/SignalForgeGameplayTags.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"

USFGameplayAbility_Heal::USFGameplayAbility_Heal()
{
	const FSignalForgeGameplayTags& GameplayTags = FSignalForgeGameplayTags::Get();

	InputTag = GameplayTags.Input_Ability_4;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(GameplayTags.Ability_EchoBased_Heal);
	SetAssetTags(AssetTags);

	// Prevent stacking heals on top of each other; the activation owned tag is
	// removed automatically when the ability ends.
	ActivationOwnedTags.AddTag(GameplayTags.State_Healing);

	// Block re-activating heal while one is already running.
	ActivationBlockedTags.AddTag(GameplayTags.State_Healing);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool USFGameplayAbility_Heal::CanActivateAbility(
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
	if (!TryGetHealCharacter(ActorInfo, Character))
	{
		return false;
	}

	if (!HealEffect)
	{
		return false;
	}

	// Don't allow casting the heal if already at full health — avoids wasting Echo.
	if (IsHealthAlreadyFull(Character))
	{
		return false;
	}

	return true;
}

void USFGameplayAbility_Heal::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ASFCharacterBase* Character = nullptr;
	if (!TryGetHealCharacter(ActorInfo, Character))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// CommitAbility runs the GameplayAbility's Cost GE (Echo) and Cooldown GE
	// configured in the editor. If the player can't pay, this returns false and
	// the ability ends without applying a heal.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CachedHandle = Handle;
	CachedActorInfo = ActorInfo;
	CachedActivationInfo = ActivationInfo;
	bHasHandledMontageExit = false;

	// Apply the heal effect immediately so the player feels feedback before
	// the animation completes. Cost was already paid by CommitAbility.
	if (!ApplyHealEffect(Character))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Drive presentation from the base class hooks (icon already exposed,
	// montage / VFX / SFX configured on the GA defaults).
	SpawnAbilityVFX();
	PlayAbilitySFX();

	if (ActivationMontage && !bEndImmediately)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this,
				NAME_None,
				ActivationMontage);

		if (!MontageTask)
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
			return;
		}

		MontageTask->OnCompleted.AddDynamic(this, &USFGameplayAbility_Heal::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &USFGameplayAbility_Heal::OnMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &USFGameplayAbility_Heal::OnMontageCancelled);
		MontageTask->OnBlendOut.AddDynamic(this, &USFGameplayAbility_Heal::OnMontageBlendOut);
		MontageTask->ReadyForActivation();

		return;
	}

	// No montage configured (or bEndImmediately): end right away.
	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}

void USFGameplayAbility_Heal::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	ClearCachedActivationState();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool USFGameplayAbility_Heal::TryGetHealCharacter(
	const FGameplayAbilityActorInfo* ActorInfo,
	ASFCharacterBase*& OutCharacter) const
{
	OutCharacter = nullptr;

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		return false;
	}

	OutCharacter = Cast<ASFCharacterBase>(ActorInfo->AvatarActor.Get());
	return OutCharacter != nullptr;
}

bool USFGameplayAbility_Heal::IsHealthAlreadyFull(const ASFCharacterBase* Character) const
{
	if (!Character)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	const float CurrentHealth = ASC->GetNumericAttribute(USFAttributeSetBase::GetHealthAttribute());
	const float MaxHealth = ASC->GetNumericAttribute(USFAttributeSetBase::GetMaxHealthAttribute());

	// Small epsilon so a tiny rounding gap doesn't block the heal.
	return CurrentHealth >= (MaxHealth - 0.5f);
}

bool USFGameplayAbility_Heal::ApplyHealEffect(ASFCharacterBase* Character)
{
	if (!Character || !HealEffect)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	ContextHandle.AddSourceObject(this);
	ContextHandle.AddInstigator(Character, Character);

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(HealEffect, GetAbilityLevel(), ContextHandle);
	if (!SpecHandle.IsValid())
	{
		return false;
	}

	if (bUseSetByCaller)
	{
		// Allows authoring a single HealEffect GE that reads the magnitude from
		// SetByCaller, letting designers tune HealAmount per-ability instance.
		SpecHandle.Data->SetSetByCallerMagnitude(FName("Data.HealAmount"), HealAmount);
	}

	const FActiveGameplayEffectHandle ActiveHandle =
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	return ActiveHandle.WasSuccessfullyApplied();
}

void USFGameplayAbility_Heal::HandleMontageFinished(bool bWasCancelled)
{
	if (bHasHandledMontageExit || !IsActive())
	{
		return;
	}

	bHasHandledMontageExit = true;
	EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, /*bReplicateEndAbility=*/true, bWasCancelled);
}

void USFGameplayAbility_Heal::ClearCachedActivationState()
{
	CachedHandle = FGameplayAbilitySpecHandle();
	CachedActorInfo = nullptr;
	CachedActivationInfo = FGameplayAbilityActivationInfo();
	bHasHandledMontageExit = false;
}

void USFGameplayAbility_Heal::OnMontageCompleted()
{
	HandleMontageFinished(false);
}

void USFGameplayAbility_Heal::OnMontageInterrupted()
{
	HandleMontageFinished(true);
}

void USFGameplayAbility_Heal::OnMontageCancelled()
{
	HandleMontageFinished(true);
}

void USFGameplayAbility_Heal::OnMontageBlendOut()
{
	HandleMontageFinished(false);
}
