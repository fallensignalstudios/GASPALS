#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ClearActiveOrder.generated.h"

/**
 * UBTTask_ClearActiveOrder
 *
 * Calls USFCompanionTacticsComponent::ClearOrder() on the controlled
 * companion, which sets Type=None and broadcasts. Place at the end of an
 * order subtree branch that should not persist (e.g. after MoveToLocation
 * succeeds).
 *
 * Persistent orders (HoldPosition, FocusFire) should NOT use this — they
 * are cleared by the player or by a separate decorator (e.g. target dead).
 */
UCLASS()
class SIGNALFORGERPG_API UBTTask_ClearActiveOrder : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ClearActiveOrder();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
