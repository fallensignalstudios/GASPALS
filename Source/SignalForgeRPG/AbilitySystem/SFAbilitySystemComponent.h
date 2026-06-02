// SFAbilitySystemComponent.h

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "SFAbilitySystemComponent.generated.h"

class USFGameplayAbility;
struct FSFAbilitySlotUIData;

DECLARE_MULTICAST_DELEGATE(FSFOnAbilitiesChanged);

UCLASS()
class SIGNALFORGERPG_API USFAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	USFAbilitySystemComponent();

	// Input routing (tag-based)
	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

	// Called once per frame by owning pawn / controller
	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);
	void ClearAbilityInput();

	bool IsAbilityInputPressed(const FGameplayTag& InputTag) const;
	bool HasAbilityWithInputTag(const FGameplayTag& InputTag) const;

	// Ability-bar data access (UI-facing, read-only)
	void GetAbilityBarSlotData(TArray<FSFAbilitySlotUIData>& OutSlots) const;

	// Ability inventory notifications
	FSFOnAbilitiesChanged& OnAbilitiesChanged() { return AbilitiesChangedDelegate; }

protected:
	// Input hooked into GAS
	virtual void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) override;
	virtual void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) override;

	virtual void OnGiveAbility(FGameplayAbilitySpec& AbilitySpec) override;
	virtual void OnRemoveAbility(FGameplayAbilitySpec& AbilitySpec) override;

private:
	void GatherAbilitySpecsWithInputTag(const FGameplayTag& InputTag, TArray<FGameplayAbilitySpecHandle>& OutSpecHandles) const;
	bool TryGetSpec(const FGameplayAbilitySpecHandle& SpecHandle, FGameplayAbilitySpec*& OutSpec) const;
	bool IsInputBlocked() const;

private:
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;

	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;

	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;

	FSFOnAbilitiesChanged AbilitiesChangedDelegate;
};