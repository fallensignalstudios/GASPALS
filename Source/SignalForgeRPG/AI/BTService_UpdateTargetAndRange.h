#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateTargetAndRange.generated.h"

class UBehaviorTreeComponent;

UCLASS()
class SIGNALFORGERPG_API UBTService_UpdateTargetAndRange : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateTargetAndRange();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName TargetActorKey = TEXT("TargetActor");

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName IsInAttackRangeKey = TEXT("IsInAttackRange");
};