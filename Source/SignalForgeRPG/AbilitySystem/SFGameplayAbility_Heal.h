#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SFGameplayAbility.h"
#include "SFGameplayAbility_Heal.generated.h"

class UGameplayEffect;
class ASFCharacterBase;

/**
 * Echo-fuelled self heal.
 *
 * Authoring:
 *   - Set Cost Gameplay Effect on the GameplayAbility (drives Echo consumption
 *     via Cost Magnitude on the Echo attribute) so CommitAbility handles it.
 *   - Optionally set Cooldown Gameplay Effect on the GameplayAbility itself
 *     (kept consistent with the rest of the GAS abilities in this project).
 *   - Configure ActivationMontage / ActivationVFX / ActivationSFX on the base
 *     class to drive presentation; this ability will play them on activate.
 *   - Set HealEffect to a GE that applies the Health gain (Instant duration
 *     with Health += SetByCaller "Data.HealAmount" or a flat modifier).
 *   - HealAmount feeds the SetByCaller on the heal GE when bUseSetByCaller is
 *     true. Otherwise the GE applies its baked Health modifier as-is.
 */
UCLASS()
class SIGNALFORGERPG_API USFGameplayAbility_Heal : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGameplayAbility_Heal();

	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	/** Instant GE that grants Health when applied. Required. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Heal")
	TSubclassOf<UGameplayEffect> HealEffect;

	/** Flat Health amount to grant (passed as SetByCaller if bUseSetByCaller). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Heal", meta = (ClampMin = "0.0"))
	float HealAmount = 35.0f;

	/** When true, HealAmount is passed to HealEffect as SetByCaller "Data.HealAmount". */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Heal")
	bool bUseSetByCaller = true;

	/**
	 * When true, the ability ends as soon as the heal effect is applied. When
	 * false, ends once the activation montage finishes (longer animation, gives
	 * the player a recovery window — useful for high-Echo heavy heals).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Heal")
	bool bEndImmediately = false;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnMontageBlendOut();

private:
	bool TryGetHealCharacter(const FGameplayAbilityActorInfo* ActorInfo, ASFCharacterBase*& OutCharacter) const;
	bool ApplyHealEffect(ASFCharacterBase* Character);
	bool IsHealthAlreadyFull(const ASFCharacterBase* Character) const;
	void HandleMontageFinished(bool bWasCancelled);
	void ClearCachedActivationState();

private:
	FGameplayAbilitySpecHandle CachedHandle;
	const FGameplayAbilityActorInfo* CachedActorInfo = nullptr;
	FGameplayAbilityActivationInfo CachedActivationInfo;
	bool bHasHandledMontageExit = false;
};
