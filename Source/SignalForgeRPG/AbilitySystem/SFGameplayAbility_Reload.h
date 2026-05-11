#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SFGameplayAbility.h"
#include "SFGameplayAbility_Reload.generated.h"

class USFEquipmentComponent;
class USFWeaponData;

/**
 * Default ranged-weapon reload ability.
 *
 * - Adds State.Weapon.Reloading while running so other systems (animation, fire ability)
 *   can react / be blocked.
 * - Plays FSFRangedWeaponConfig::ReloadMontage if set, otherwise runs a timer of
 *   FSFRangedWeaponConfig::ReloadSeconds.
 * - On completion, refills the equipped weapon's clip up to its ClipSize.
 * - Cancels cleanly if interrupted (e.g. hit react, sprint).
 *
 * Note: the inventory ammo pool consumption is left to the weapon's owning inventory
 * (so a designer can plug in a "shared reserve" model later) — for now this ability
 * just tops up the clip, which matches single-magazine prototype loops.
 */
UCLASS()
class SIGNALFORGERPG_API USFGameplayAbility_Reload : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGameplayAbility_Reload();

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

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	UFUNCTION()
	void OnReloadMontageCompleted();

	UFUNCTION()
	void OnReloadMontageInterrupted();

private:
	void FinishReload(bool bRefillClip);
	bool ResolveContext(USFEquipmentComponent*& OutEquipment, USFWeaponData*& OutWeaponData) const;

	FGameplayAbilitySpecHandle CachedHandle;
	const FGameplayAbilityActorInfo* CachedActorInfo = nullptr;
	FGameplayAbilityActivationInfo CachedActivationInfo;

	FTimerHandle ReloadTimerHandle;
};
