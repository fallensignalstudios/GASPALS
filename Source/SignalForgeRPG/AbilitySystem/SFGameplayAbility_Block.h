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

	/**
	 * Length of the active parry window opened the moment the block input is
	 * pressed. While State.ParryWindow is on the owner, blockable hits resolve
	 * to ESFHitOutcome::PerfectParry instead of Blocked. Jedi: Fallen Order
	 * lands close to 0.20–0.25 here; tighten for harder feel, widen for forgive.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Block|Parry", meta = (ClampMin = "0.0"))
	float ParryWindowSeconds = 0.2f;

	/**
	 * After the parry window closes (whether it landed a parry or not), block
	 * input is still accepted as a normal hold-block, but no fresh parry can be
	 * opened until State.ParryCooldown clears. Stops mash-to-parry.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Block|Parry", meta = (ClampMin = "0.0"))
	float ParryCooldownSeconds = 0.6f;

private:
	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitInputRelease> WaitInputReleaseTask = nullptr;

	FActiveGameplayEffectHandle BlockingEffectHandle;

	/** Timer that removes State.ParryWindow once ParryWindowSeconds elapses. */
	FTimerHandle ParryWindowTimerHandle;

	/** Timer that removes State.ParryCooldown once ParryCooldownSeconds elapses. */
	FTimerHandle ParryCooldownTimerHandle;

	/** Opens the parry window (gated on cooldown). Safe to call from ActivateAbility. */
	void OpenParryWindow();

	/** Internal: clears State.ParryWindow and starts the cooldown. */
	void CloseParryWindow();
};