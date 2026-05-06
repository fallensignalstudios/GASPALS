#pragma once

#include "CoreMinimal.h"
#include "Characters/SFCharacterBase.h"
#include "SFEnemyCharacter.generated.h"

UCLASS()
class SIGNALFORGERPG_API ASFEnemyCharacter : public ASFCharacterBase
{
	GENERATED_BODY()

public:
	ASFEnemyCharacter();

	virtual void HandleDeath() override;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetLastDamagingCharacter(ASFCharacterBase* InCharacter);

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetXPReward() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float DetectionRange = 1200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float AttackRange = 175.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float AttackInterval = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TObjectPtr<class UBehaviorTree> BehaviorTreeAsset;

	UPROPERTY()
	TObjectPtr<ASFCharacterBase> LastDamagingCharacter;

public:
	float GetDetectionRange() const { return DetectionRange; }
	float GetAttackRange() const { return AttackRange; }
	float GetAttackInterval() const { return AttackInterval; }
	UBehaviorTree* GetBehaviorTreeAsset() const { return BehaviorTreeAsset; }
};