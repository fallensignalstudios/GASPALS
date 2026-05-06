#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "BTTask_AttackCurrentTarget.generated.h"

/**
 * UBTTask_AttackCurrentTarget
 *
 * Activates the companion's basic attack ability against FocusTarget.
 *
 * Differences from UBTTask_AttackTarget (the enemy version):
 *   - Reads FocusTarget from the blackboard (so combat target service +
 *     order branch can drive it).
 *   - Sets controller focus to the target so weapon/abilities aim correctly.
 *   - Looks up the ability by tag on the controlled companion's ASC.
 *
 * Default AbilityTag is Ability.Attack.Light (set in ctor); designers may
 * swap per BT instance.
 */
UCLASS()
class SIGNALFORGERPG_API UBTTask_AttackCurrentTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_AttackCurrentTarget();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName FocusTargetKey = TEXT("FocusTarget");

	UPROPERTY(EditAnywhere, Category = "Attack")
	FGameplayTag AbilityTag;
};
