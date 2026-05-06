#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateDesiredPosition.generated.h"

/**
 * UBTService_UpdateDesiredPosition
 *
 * Computes a stance/range-aware formation slot the BT can move to:
 *
 *   Tank   : in front of the player, between player and FocusTarget if any.
 *   DPS    : flank the FocusTarget (lateral offset from player->target line),
 *            or trail off-shoulder of the player when no target.
 *   Healer : hug the player's near side, slightly behind.
 *
 * EngagementRange scales the spacing:
 *   StayClose  -> 250 uu
 *   Skirmish   -> 600 uu
 *   LongRange  -> 1100 uu
 *
 * Outputs:
 *   DesiredPosition       (Vector) — world location for Move To
 *   DesiredCombatDistance (Float)  — preferred range from FocusTarget
 */
UCLASS()
class SIGNALFORGERPG_API UBTService_UpdateDesiredPosition : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateDesiredPosition();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName PlayerActorKey = TEXT("PlayerActor");

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName FocusTargetKey = TEXT("FocusTarget");

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName DesiredPositionKey = TEXT("DesiredPosition");

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName DesiredCombatDistanceKey = TEXT("DesiredCombatDistance");

	UPROPERTY(EditAnywhere, Category = "Spacing", meta = (ClampMin = "50.0"))
	float StayCloseDistance = 250.f;

	UPROPERTY(EditAnywhere, Category = "Spacing", meta = (ClampMin = "100.0"))
	float SkirmishDistance = 600.f;

	UPROPERTY(EditAnywhere, Category = "Spacing", meta = (ClampMin = "200.0"))
	float LongRangeDistance = 1100.f;

	/** Lateral offset (uu) DPS uses when flanking. */
	UPROPERTY(EditAnywhere, Category = "Spacing", meta = (ClampMin = "0.0"))
	float DPSFlankOffset = 350.f;

	/** Distance behind the player a Healer prefers. */
	UPROPERTY(EditAnywhere, Category = "Spacing", meta = (ClampMin = "0.0"))
	float HealerTrailDistance = 200.f;
};
