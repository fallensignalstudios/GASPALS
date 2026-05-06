#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateCombatTarget.generated.h"

class AActor;

/**
 * UBTService_UpdateCombatTarget
 *
 * Picks/refreshes FocusTarget based on aggression + perception.
 *
 *   Passive    : never sets a proactive target (clears unless attacked).
 *   Defensive  : engages threats to player (close to player) or self.
 *   Aggressive : engages closest perceived hostile within MaxEngageRange.
 *
 * If a FocusFire order is active, the service does NOT overwrite the focus —
 * the order branch holds the target.
 */
UCLASS()
class SIGNALFORGERPG_API UBTService_UpdateCombatTarget : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateCombatTarget();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName FocusTargetKey = TEXT("FocusTarget");

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName PlayerActorKey = TEXT("PlayerActor");

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName OrderTypeKey = TEXT("OrderType");

	/** Defensive stance treats hostiles within this radius of the player as threats. */
	UPROPERTY(EditAnywhere, Category = "Targeting", meta = (ClampMin = "100.0"))
	float DefensivePlayerThreatRadius = 1500.f;

	/** Maximum range from the companion at which Aggressive stance will pick a new target. */
	UPROPERTY(EditAnywhere, Category = "Targeting", meta = (ClampMin = "100.0"))
	float AggressiveEngageRange = 2500.f;
};
