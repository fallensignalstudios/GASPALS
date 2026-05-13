#pragma once

#include "CoreMinimal.h"
#include "Characters/SFCharacterBase.h"
#include "SFEnemyCharacter.generated.h"

class UBehaviorTree;

/**
 * ASFEnemyCharacter
 *
 * Thin convenience subclass of ASFCharacterBase. Hostility is no longer
 * implied by this class -- it is determined by the FactionComponent's tag
 * and the faction relationship matrix in USFFactionStatics. This class
 * just bundles a few enemy-flavored defaults:
 *
 *   - Spawns under ASFEnemyAIController.
 *   - Carries a designer-authored BehaviorTreeAsset.
 *   - Carries combat heuristics (DetectionRange / AttackRange / AttackInterval)
 *     consumed by the behavior tree.
 *
 * SetLastDamagingCharacter / GetXPReward live on ASFCharacterBase now, so any
 * character with a hostile faction relationship grants XP on kill.
 */
UCLASS()
class SIGNALFORGERPG_API ASFEnemyCharacter : public ASFCharacterBase
{
	GENERATED_BODY()

public:
	ASFEnemyCharacter();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float DetectionRange = 1200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float AttackRange = 175.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float AttackInterval = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

public:
	float GetDetectionRange() const { return DetectionRange; }
	float GetAttackRange() const { return AttackRange; }
	float GetAttackInterval() const { return AttackInterval; }
	UBehaviorTree* GetBehaviorTreeAsset() const { return BehaviorTreeAsset; }
};
