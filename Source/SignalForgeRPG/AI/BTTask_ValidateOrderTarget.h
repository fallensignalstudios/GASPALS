#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ValidateOrderTarget.generated.h"

/**
 * UBTTask_ValidateOrderTarget
 *
 * Succeeds iff OrderTargetActor is a non-null, non-pending-kill actor and
 * (optionally) is not flagged dead via ASFCharacterBase::IsDead().
 *
 * Use as a guard at the start of AttackTarget / FocusFire / UseAbility
 * order branches so we early-out and clear the order if the target died.
 */
UCLASS()
class SIGNALFORGERPG_API UBTTask_ValidateOrderTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ValidateOrderTarget();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName OrderTargetActorKey = TEXT("OrderTargetActor");

	/** If true, fail when the target's ASFCharacterBase::IsDead() returns true. */
	UPROPERTY(EditAnywhere, Category = "Validation")
	bool bRejectDead = true;
};
