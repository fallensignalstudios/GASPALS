#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateFacing.generated.h"

/**
 * UBTService_UpdateFacing
 *
 * Toggles companion rotation behavior based on combat / order context:
 *
 *   In combat (FocusTarget set or AttackTarget/FocusFire order):
 *     - OrientRotationToMovement = false
 *     - UseControllerDesiredRotation = true
 *     - Controller focuses target so the pawn yaws to face it while strafing
 *
 *   Out of combat (no FocusTarget):
 *     - OrientRotationToMovement = true   (face direction of travel)
 *     - UseControllerDesiredRotation = false
 *     - Controller focus cleared
 *
 * Place on the Selector root next to the other companion services.
 */
UCLASS()
class SIGNALFORGERPG_API UBTService_UpdateFacing : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateFacing();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName FocusTargetKey = TEXT("FocusTarget");

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName OrderTypeKey = TEXT("OrderType");

	/** Yaw rotation rate (deg/sec) used out of combat for orient-to-movement. */
	UPROPERTY(EditAnywhere, Category = "Rotation", meta = (ClampMin = "60.0"))
	float MovementYawRate = 720.f;

	/** Yaw rotation rate (deg/sec) used in combat for face-target. */
	UPROPERTY(EditAnywhere, Category = "Rotation", meta = (ClampMin = "60.0"))
	float CombatYawRate = 540.f;
};
