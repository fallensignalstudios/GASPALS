#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SFGameplayAbility.h"
#include "SFGameplayAbility_Sprint.generated.h"

class UGameplayEffect;
class ASFCharacterBase;

UCLASS()
class SIGNALFORGERPG_API USFGameplayAbility_Sprint : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGameplayAbility_Sprint();

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

	virtual void InputReleased(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sprint", meta = (ClampMin = "0.01"))
	float StaminaDrainInterval = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sprint")
	TSubclassOf<UGameplayEffect> SprintStaminaDrainEffect;

private:
	void ApplySprintDrain();
	bool HasEnoughStamina(const FGameplayAbilityActorInfo* ActorInfo = nullptr) const;
	bool TryGetSprintCharacter(const FGameplayAbilityActorInfo* ActorInfo, ASFCharacterBase*& OutCharacter) const;
	void StartSprintDrainTimer(ASFCharacterBase* Character);
	void StopSprintDrainTimer(ASFCharacterBase* Character);
	void EndSprintAbility(bool bReplicateEndAbility, bool bWasCancelled);

private:
	FTimerHandle StaminaDrainTimerHandle;
	FGameplayAbilitySpecHandle CachedHandle;
	const FGameplayAbilityActorInfo* CachedActorInfo = nullptr;
	FGameplayAbilityActivationInfo CachedActivationInfo;
	bool bSprintActive = false;
};