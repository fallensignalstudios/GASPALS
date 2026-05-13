#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "SFBTTask_LookAtLocation.generated.h"

/**
 * Rotates the AI to face a world location read from a BB Vector key (e.g.
 * "LastKnownTargetLocation"). Use this when the AI has lost direct vision of
 * the target but still wants to point its body / weapon at the last place
 * the target was seen.
 *
 * Identical interpolation behavior to FaceTarget, but operates on a static
 * location instead of an actor that might keep moving.
 */
UCLASS()
class SIGNALFORGERPG_API USFBTTask_LookAtLocation : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	USFBTTask_LookAtLocation();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Look", meta = (ClampMin = "1.0"))
	float RotationRate = 360.0f;

	UPROPERTY(EditAnywhere, Category = "Look", meta = (ClampMin = "0.1"))
	float AcceptanceAngleDegrees = 5.0f;

	UPROPERTY(EditAnywhere, Category = "Look", meta = (ClampMin = "0.1"))
	float TimeoutSeconds = 3.0f;
};
