#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SFGameplayAbility.h"
#include "SFGameplayAbility_AttackLight.generated.h"

class UGameplayEffect;
class UAbilityTask_PlayMontageAndWait;
class ASFCharacterBase;
class USFEquipmentComponent;

UCLASS()
class SIGNALFORGERPG_API USFGameplayAbility_AttackLight : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGameplayAbility_AttackLight();

	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack")
	TSubclassOf<UGameplayEffect> DamageEffect;

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

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnMontageBlendOut();

private:
	bool TryGetActivationData(
		const FGameplayAbilityActorInfo* ActorInfo,
		ASFCharacterBase*& OutCharacter,
		USFEquipmentComponent*& OutEquipmentComponent,
		UAnimMontage*& OutAttackMontage) const;

	void HandleMontageFinished(bool bWasCancelled, bool bReplicateEndAbility);
	void ClearCachedActivationState();

private:
	FGameplayAbilitySpecHandle CachedHandle;
	const FGameplayAbilityActorInfo* CachedActorInfo = nullptr;
	FGameplayAbilityActivationInfo CachedActivationInfo;
	bool bHasHandledMontageExit = false;
};