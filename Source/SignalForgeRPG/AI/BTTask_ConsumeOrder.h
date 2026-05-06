#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ConsumeOrder.generated.h"

/**
 * UBTTask_ConsumeOrder
 *
 * Marks the current ActiveOrder as "handled" by stamping its sequence into
 * the OrderSequence blackboard key, so re-issuing the same order type still
 * triggers the order branch (sequence will differ). Also asserts the order
 * is valid; fails if Type == None.
 *
 * Use as the first leaf inside the Order subtree.
 */
UCLASS()
class SIGNALFORGERPG_API UBTTask_ConsumeOrder : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ConsumeOrder();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName OrderTypeKey = TEXT("OrderType");

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName OrderSequenceKey = TEXT("OrderSequence");
};
