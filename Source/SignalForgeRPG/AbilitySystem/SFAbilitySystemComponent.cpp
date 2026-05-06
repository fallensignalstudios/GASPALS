// SFAbilitySystemComponent.cpp

#include "AbilitySystem/SFAbilitySystemComponent.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/SFGameplayAbility.h"
#include "UI/SFAbilitySlotUIData.h"

USFAbilitySystemComponent::USFAbilitySystemComponent()
{
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
}

void USFAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{

	UE_LOG(LogTemp, Warning, TEXT("ASC::AbilityInputTagPressed: %s"), *InputTag.ToString());

	if (!InputTag.IsValid())
	{
		return;
	}

	TArray<FGameplayAbilitySpecHandle> MatchingHandles;
	GatherAbilitySpecsWithInputTag(InputTag, MatchingHandles);

	for (const FGameplayAbilitySpecHandle& Handle : MatchingHandles)
	{
		InputPressedSpecHandles.AddUnique(Handle);
		InputHeldSpecHandles.AddUnique(Handle);
	}
}

void USFAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	TArray<FGameplayAbilitySpecHandle> MatchingHandles;
	GatherAbilitySpecsWithInputTag(InputTag, MatchingHandles);

	for (const FGameplayAbilitySpecHandle& Handle : MatchingHandles)
	{
		InputReleasedSpecHandles.AddUnique(Handle);
		InputHeldSpecHandles.Remove(Handle);
	}
}

void USFAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
	if (bGamePaused)
	{
		return;
	}

	if (IsInputBlocked())
	{
		ClearAbilityInput();
		return;
	}

	TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;
	AbilitiesToActivate.Reserve(InputHeldSpecHandles.Num() + InputPressedSpecHandles.Num());

	// Held input → WhileInputActive abilities
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputHeldSpecHandles)
	{
		FGameplayAbilitySpec* AbilitySpec = nullptr;
		if (!TryGetSpec(SpecHandle, AbilitySpec) || !AbilitySpec->Ability)
		{
			continue;
		}

		if (AbilitySpec->IsActive())
		{
			continue;
		}

		const USFGameplayAbility* SFAbility = Cast<USFGameplayAbility>(AbilitySpec->Ability);
		if (!ensureMsgf(SFAbility != nullptr,
			TEXT("Granted ability on spec [%s] is not a USFGameplayAbility."),
			*SpecHandle.ToString()))
		{
			continue;
		}

		if (SFAbility->GetActivationPolicy() == ESFAbilityActivationPolicy::WhileInputActive)
		{
			AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
		}
	}

	// Pressed input → OnInputTriggered abilities, or forwarded to active ones
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputPressedSpecHandles)
	{
		FGameplayAbilitySpec* AbilitySpec = nullptr;
		if (!TryGetSpec(SpecHandle, AbilitySpec) || !AbilitySpec->Ability)
		{
			continue;
		}

		AbilitySpec->InputPressed = true;

		if (AbilitySpec->IsActive())
		{
			AbilitySpecInputPressed(*AbilitySpec);
			continue;
		}

		const USFGameplayAbility* SFAbility = Cast<USFGameplayAbility>(AbilitySpec->Ability);
		if (!ensureMsgf(SFAbility != nullptr,
			TEXT("Granted ability on spec [%s] is not a USFGameplayAbility."),
			*SpecHandle.ToString()))
		{
			continue;
		}

		if (SFAbility->GetActivationPolicy() == ESFAbilityActivationPolicy::OnInputTriggered)
		{
			AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
		}
	}

	for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : AbilitiesToActivate)
	{
		TryActivateAbility(AbilitySpecHandle);
	}

	// Released input
	for (const FGameplayAbilitySpecHandle& SpecHandle : InputReleasedSpecHandles)
	{
		FGameplayAbilitySpec* AbilitySpec = nullptr;
		if (!TryGetSpec(SpecHandle, AbilitySpec) || !AbilitySpec->Ability)
		{
			continue;
		}

		AbilitySpec->InputPressed = false;

		if (AbilitySpec->IsActive())
		{
			AbilitySpecInputReleased(*AbilitySpec);
		}
	}

	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

void USFAbilitySystemComponent::ClearAbilityInput()
{
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
}

bool USFAbilitySystemComponent::IsAbilityInputPressed(const FGameplayTag& InputTag) const
{
	if (!InputTag.IsValid())
	{
		return false;
	}

	bool bAnyPressed = false;

	FScopedAbilityListLock ActiveScopeLock(*const_cast<USFAbilitySystemComponent*>(this));
	for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.Ability && AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			bAnyPressed |= AbilitySpec.InputPressed;
		}
	}

	return bAnyPressed;
}

bool USFAbilitySystemComponent::HasAbilityWithInputTag(const FGameplayTag& InputTag) const
{
	if (!InputTag.IsValid())
	{
		return false;
	}

	FScopedAbilityListLock ActiveScopeLock(*const_cast<USFAbilitySystemComponent*>(this));
	for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.Ability && AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			return true;
		}
	}

	return false;
}

void USFAbilitySystemComponent::GetAbilityBarSlotData(TArray<FSFAbilitySlotUIData>& OutSlots) const
{
	OutSlots.Reset();

	FScopedAbilityListLock ActiveScopeLock(*const_cast<USFAbilitySystemComponent*>(this));
	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		const USFGameplayAbility* AbilityCDO = nullptr;

		if (Spec.Ability)
		{
			AbilityCDO = Cast<USFGameplayAbility>(Spec.Ability->GetClass()->GetDefaultObject());
		}

		if (!AbilityCDO || !AbilityCDO->ShouldShowInAbilityBar())
		{
			continue;
		}

		const FGameplayTag InputTag = AbilityCDO->GetInputTag();
		if (!InputTag.IsValid())
		{
			continue;
		}

		FSFAbilitySlotUIData Data;
		Data.InputTag = InputTag;
		Data.Icon = AbilityCDO->GetAbilityIcon();
		Data.bHasAbility = true;

		if (AbilityCDO->HasCooldownTag())
		{
			const FGameplayTag CooldownTag = AbilityCDO->GetCooldownTag();
			Data.CooldownTag = CooldownTag;
			Data.bIsReady = !HasMatchingGameplayTag(CooldownTag);
		}
		else
		{
			Data.bIsReady = true;
		}

		OutSlots.Add(Data);
	}
}

void USFAbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputPressed(Spec);

	if (!Spec.IsActive())
	{
		return;
	}

	const UGameplayAbility* AbilityInstance = Spec.GetPrimaryInstance();

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const FPredictionKey PredictionKey =
		AbilityInstance
		? AbilityInstance->GetCurrentActivationInfo().GetActivationPredictionKey()
		: Spec.ActivationInfo.GetActivationPredictionKey();
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, PredictionKey);
}

void USFAbilitySystemComponent::AbilitySpecInputReleased(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputReleased(Spec);

	if (!Spec.IsActive())
	{
		return;
	}

	const UGameplayAbility* AbilityInstance = Spec.GetPrimaryInstance();

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const FPredictionKey PredictionKey =
		AbilityInstance
		? AbilityInstance->GetCurrentActivationInfo().GetActivationPredictionKey()
		: Spec.ActivationInfo.GetActivationPredictionKey();
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, PredictionKey);
}

void USFAbilitySystemComponent::OnGiveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	Super::OnGiveAbility(AbilitySpec);
	AbilitiesChangedDelegate.Broadcast();
}

void USFAbilitySystemComponent::OnRemoveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	Super::OnRemoveAbility(AbilitySpec);
	AbilitiesChangedDelegate.Broadcast();
}

void USFAbilitySystemComponent::GatherAbilitySpecsWithInputTag(
	const FGameplayTag& InputTag,
	TArray<FGameplayAbilitySpecHandle>& OutSpecHandles) const
{
	OutSpecHandles.Reset();

	if (!InputTag.IsValid())
	{
		return;
	}

	FScopedAbilityListLock ActiveScopeLock(*const_cast<USFAbilitySystemComponent*>(this));
	for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (AbilitySpec.Ability && AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			OutSpecHandles.Add(AbilitySpec.Handle);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("GatherAbilitySpecsWithInputTag: [%s] found %d match(es)"),
		*InputTag.ToString(), OutSpecHandles.Num());
}

bool USFAbilitySystemComponent::TryGetSpec(const FGameplayAbilitySpecHandle& SpecHandle, FGameplayAbilitySpec*& OutSpec) const
{
	OutSpec = const_cast<FGameplayAbilitySpec*>(FindAbilitySpecFromHandle(SpecHandle));
	return OutSpec != nullptr;
}

bool USFAbilitySystemComponent::IsInputBlocked() const
{
	// Hook this into stun / UI focus tags when you’re ready.
	return false;
}