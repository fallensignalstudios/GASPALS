#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "SFBTService_UpdateTargetState.generated.h"

/**
 * Refreshes derived target state on the blackboard each tick:
 *   - DistanceToTarget (float)  : straight-line distance from AI to TargetActor
 *   - HasLineOfSight  (bool)    : eye trace to target
 *   - TargetIsHostile (bool)    : faction check (also true for the very common
 *                                 case where TargetActor is your live target)
 *   - TargetIsDead    (bool)    : reflects ASFCharacterBase::IsDead()
 *   - LastKnownTargetLocation (vector) : updated whenever LOS is true
 *
 * Also auto-clears TargetActor when:
 *   - target is dead, OR
 *   - target is no longer hostile (e.g. faction relationship changed mid-fight), OR
 *   - distance exceeds LeashDistance (loses interest).
 *
 * Any of the output keys can be left unconfigured; the service only writes
 * keys that are set.
 */
UCLASS()
class SIGNALFORGERPG_API USFBTService_UpdateTargetState : public UBTService
{
	GENERATED_BODY()

public:
	USFBTService_UpdateTargetState();

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Keys")
	struct FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Keys")
	struct FBlackboardKeySelector DistanceKey;

	UPROPERTY(EditAnywhere, Category = "Keys")
	struct FBlackboardKeySelector HasLineOfSightKey;

	UPROPERTY(EditAnywhere, Category = "Keys")
	struct FBlackboardKeySelector IsHostileKey;

	UPROPERTY(EditAnywhere, Category = "Keys")
	struct FBlackboardKeySelector IsDeadKey;

	UPROPERTY(EditAnywhere, Category = "Keys")
	struct FBlackboardKeySelector LastKnownLocationKey;

	/** If > 0, target is cleared when distance exceeds this value. */
	UPROPERTY(EditAnywhere, Category = "Behavior", meta = (ClampMin = "0.0"))
	float LeashDistance = 4000.0f;

	/** Clear TargetActor if target is dead. */
	UPROPERTY(EditAnywhere, Category = "Behavior")
	bool bClearOnDead = true;

	/** Clear TargetActor if it is no longer hostile (e.g. faction now allied). */
	UPROPERTY(EditAnywhere, Category = "Behavior")
	bool bClearOnNonHostile = true;

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
};
