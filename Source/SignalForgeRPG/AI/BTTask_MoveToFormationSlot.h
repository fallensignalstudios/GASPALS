#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_MoveToFormationSlot.generated.h"

/**
 * UBTTask_MoveToFormationSlot
 *
 * Calls AIController::MoveToLocation to the DesiredPosition the position
 * service already wrote, with a configurable acceptance radius. Returns
 * Succeeded immediately if the companion is already inside acceptance.
 *
 * This is a lightweight wrapper — designers can also drop a stock Move To
 * task pointing at DesiredPosition. This version exists so order branches
 * can re-use a single typed leaf without per-instance configuration.
 */
UCLASS()
class SIGNALFORGERPG_API UBTTask_MoveToFormationSlot : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_MoveToFormationSlot();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName DesiredPositionKey = TEXT("DesiredPosition");

	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = "10.0"))
	float AcceptanceRadius = 80.f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	bool bUsePathfinding = true;
};
