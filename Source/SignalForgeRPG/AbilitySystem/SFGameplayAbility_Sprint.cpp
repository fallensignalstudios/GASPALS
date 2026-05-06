#include "AbilitySystem/SFGameplayAbility_Sprint.h"

#include "AbilitySystemComponent.h"
#include "Characters/SFCharacterBase.h"
#include "Components/SFStatRegenComponent.h"
#include "Core/SFAttributeSetBase.h"
#include "Core/SignalForgeGameplayTags.h"
#include "Engine/World.h"
#include "TimerManager.h"

USFGameplayAbility_Sprint::USFGameplayAbility_Sprint()
{
	const FSignalForgeGameplayTags& GameplayTags = FSignalForgeGameplayTags::Get();

	InputTag = GameplayTags.Input_Ability_Sprint;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(GameplayTags.Ability_Movement_Sprint);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(GameplayTags.State_Movement_Sprinting);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void USFGameplayAbility_Sprint::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// Already running — don't re-initialize
	if (bSprintActive)
	{
		return;
	}

	ASFCharacterBase* Character = nullptr;
	if (!TryGetSprintCharacter(ActorInfo, Character))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!HasEnoughStamina())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CachedHandle = Handle;
	CachedActorInfo = ActorInfo;
	CachedActivationInfo = ActivationInfo;
	bSprintActive = true;

	Character->StartSprint();

	if (USFStatRegenComponent* RegenComponent = Character->GetStatRegenComponent())
	{
		RegenComponent->NotifySprintStarted();
	}

	StartSprintDrainTimer(Character);
}

void USFGameplayAbility_Sprint::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (ASFCharacterBase* Character = nullptr; TryGetSprintCharacter(ActorInfo, Character))
	{
		Character->StopSprint();
		StopSprintDrainTimer(Character);

		if (USFStatRegenComponent* RegenComponent = Character->GetStatRegenComponent())
		{
			RegenComponent->NotifySprintStopped();
		}
	}

	CachedHandle = FGameplayAbilitySpecHandle();
	CachedActorInfo = nullptr;
	CachedActivationInfo = FGameplayAbilityActivationInfo();
	bSprintActive = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USFGameplayAbility_Sprint::InputReleased(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);

	if (IsActive())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void USFGameplayAbility_Sprint::ApplySprintDrain()
{
	if (!bSprintActive || !IsActive())
	{
		return;
	}

	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();

	ASFCharacterBase* Character = nullptr;
	if (!TryGetSprintCharacter(ActorInfo, Character) || !AbilitySystemComponent)
	{
		EndSprintAbility(true, true);
		return;
	}

	if (!HasEnoughStamina())
	{
		EndSprintAbility(true, false);
		return;
	}

	if (SprintStaminaDrainEffect)
	{
		FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
		EffectContext.AddSourceObject(this);

		FGameplayEffectSpecHandle SpecHandle =
			AbilitySystemComponent->MakeOutgoingSpec(SprintStaminaDrainEffect, 1.0f, EffectContext);

		if (SpecHandle.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

			if (USFStatRegenComponent* RegenComponent = Character->GetStatRegenComponent())
			{
				RegenComponent->NotifyStaminaSpent();
			}
		}
	}

	if (!HasEnoughStamina())
	{
		EndSprintAbility(true, false);
	}
}

bool USFGameplayAbility_Sprint::CanActivateAbility(
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
	if (!TryGetSprintCharacter(ActorInfo, Character))
	{
		return false;
	}

	return HasEnoughStamina(ActorInfo); // pass ActorInfo explicitly
}

bool USFGameplayAbility_Sprint::TryGetSprintCharacter(
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

void USFGameplayAbility_Sprint::StartSprintDrainTimer(ASFCharacterBase* Character)
{
	if (!Character)
	{
		return;
	}

	if (UWorld* World = Character->GetWorld())
	{
		World->GetTimerManager().SetTimer(
			StaminaDrainTimerHandle,
			this,
			&USFGameplayAbility_Sprint::ApplySprintDrain,
			StaminaDrainInterval,
			true);
	}
}

void USFGameplayAbility_Sprint::StopSprintDrainTimer(ASFCharacterBase* Character)
{
	if (!Character)
	{
		return;
	}

	if (UWorld* World = Character->GetWorld())
	{
		World->GetTimerManager().ClearTimer(StaminaDrainTimerHandle);
	}
}

void USFGameplayAbility_Sprint::EndSprintAbility(bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsActive())
	{
		EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, bReplicateEndAbility, bWasCancelled);
	}
}

bool USFGameplayAbility_Sprint::HasEnoughStamina(const FGameplayAbilityActorInfo* InActorInfo) const
{
	const FGameplayAbilityActorInfo* ActorInfo = InActorInfo ? InActorInfo : GetCurrentActorInfo();
	if (!ActorInfo)
	{
		return false;
	}

	const ASFCharacterBase* Character = Cast<ASFCharacterBase>(ActorInfo->AvatarActor.Get());
	if (!Character)
	{
		return false;
	}

	const USFAttributeSetBase* Attributes = Character->GetAttributeSet();
	if (!Attributes)
	{
		return false;
	}

	return Attributes->GetStamina() > 0.0f;
}