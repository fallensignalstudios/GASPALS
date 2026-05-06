#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_SetHoldAnchor.generated.h"

/**
 * UBTTask_SetHoldAnchor
 *
 * Resolves the HomeAnchorLocation for a HoldPosition order:
 *   - If the active order carries a non-zero TargetLocation, use it.
 *   - Otherwise, use the companion's current world location.
 *
 * The Hold subtree's Move To then references HomeAnchorLocation so the
 * companion returns to anchor between fights without losing its stand.
 */
UCLASS()
class SIGNALFORGERPG_API UBTTask_SetHoldAnchor : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_SetHoldAnchor();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName OrderTargetLocationKey = TEXT("OrderTargetLocation");

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName HomeAnchorLocationKey = TEXT("HomeAnchorLocation");
};
