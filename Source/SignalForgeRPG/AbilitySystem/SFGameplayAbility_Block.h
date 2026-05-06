// Copyright Fallen Signal Studios LLC 2026. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SFGameplayAbility.h"
#include "SFGameplayAbility_Block.generated.h"

class UAbilityTask_WaitInputRelease;
class UGameplayEffect;

UCLASS(Abstract)
class SIGNALFORGERPG_API USFGameplayAbility_Block : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGameplayAbility_Block();

	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	UFUNCTION()
	void OnInputReleased(float TimeHeld);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Block")
	TSubclassOf<UGameplayEffect> BlockingEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Block")
	bool bCancelBlockOnStunOrBreak = true;

private:
	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitInputRelease> WaitInputReleaseTask = nullptr;

	FActiveGameplayEffectHandle BlockingEffectHandle;
};