#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "SFBTTask_FaceTarget.generated.h"

/**
 * Rotates the AI's controlled pawn (yaw only) to face the actor stored in the
 * configured BB key. Smoothly interpolates at RotationRate degrees/second and
 * succeeds when the remaining yaw delta is within AcceptanceAngleDegrees.
 *
 * Fails immediately if the BB key is empty or the target is dead.
 *
 * This is intentionally a "controlled" facing task rather than relying on
 * AAIController::SetFocus, so behavior trees can run it in a sub-sequence
 * (e.g. "face, then fire") without leaving a lingering focus that fights
 * other movement nodes.
 */
UCLASS()
class SIGNALFORGERPG_API USFBTTask_FaceTarget : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	USFBTTask_FaceTarget();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;

protected:
	/** Degrees/second of yaw interpolation. */
	UPROPERTY(EditAnywhere, Category = "Face", meta = (ClampMin = "1.0"))
	float RotationRate = 540.0f;

	/** Succeed once remaining yaw delta is <= this many degrees. */
	UPROPERTY(EditAnywhere, Category = "Face", meta = (ClampMin = "0.1"))
	float AcceptanceAngleDegrees = 5.0f;

	/** Hard timeout to avoid getting stuck if target keeps fleeing in a circle. */
	UPROPERTY(EditAnywhere, Category = "Face", meta = (ClampMin = "0.1"))
	float TimeoutSeconds = 3.0f;
};
