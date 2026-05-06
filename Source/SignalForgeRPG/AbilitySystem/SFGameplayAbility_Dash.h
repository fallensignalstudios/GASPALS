#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SFGameplayAbility.h"
#include "SFGameplayAbility_Dash.generated.h"

class UAnimMontage;
class UNiagaraSystem;
class ASFCharacterBase;

UCLASS()
class SIGNALFORGERPG_API USFGameplayAbility_Dash : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGameplayAbility_Dash();

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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash", meta = (ClampMin = "0.0"))
	float DashStrength = 1200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	TObjectPtr<UAnimMontage> DashMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	TObjectPtr<UNiagaraSystem> DashNiagara = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	bool bUseLastMovementInputForDash = true;

private:
	bool TryGetDashCharacter(const FGameplayAbilityActorInfo* ActorInfo, ASFCharacterBase*& OutCharacter) const;
	FVector ResolveDashDirection(const ASFCharacterBase* Character) const;
};