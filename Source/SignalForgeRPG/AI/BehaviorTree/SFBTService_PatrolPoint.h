#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "SFBTService_PatrolPoint.generated.h"

/**
 * Picks a random reachable point on the navmesh within PatrolRadius of the
 * AI's HomeLocation (BB Vector key) and writes it to the PatrolPoint BB key.
 *
 * The point is only re-rolled when:
 *   - PatrolPoint is unset / zero, OR
 *   - the AI has gotten within ArrivalDistance of the current PatrolPoint
 *     (i.e. it reached the previous waypoint and is now ready for the next), OR
 *   - SecondsSinceLastRoll exceeds StuckTimeout (recovery: AI is wedged
 *     against geometry and the MoveTo failed silently).
 *
 * Attach this service to the patrol Sequence (peaceful idle branch). The
 * Sequence then just does: Move To PatrolPoint -> Wait -> repeat. The service
 * keeps PatrolPoint fresh in the background.
 */
UCLASS()
class SIGNALFORGERPG_API USFBTService_PatrolPoint : public UBTService
{
	GENERATED_BODY()

public:
	USFBTService_PatrolPoint();

	struct FMemory
	{
		float SecondsSinceLastRoll = 0.0f;
	};

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FMemory); }
	virtual FString GetStaticDescription() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Keys")
	struct FBlackboardKeySelector HomeLocationKey;

	UPROPERTY(EditAnywhere, Category = "Keys")
	struct FBlackboardKeySelector PatrolPointKey;

	/** Radius around HomeLocation in which to sample patrol points (cm). */
	UPROPERTY(EditAnywhere, Category = "Patrol", meta = (ClampMin = "100.0"))
	float PatrolRadius = 1500.0f;

	/** Distance at which the current PatrolPoint counts as "reached". */
	UPROPERTY(EditAnywhere, Category = "Patrol", meta = (ClampMin = "10.0"))
	float ArrivalDistance = 120.0f;

	/** Max attempts to find a navigable point per re-roll. */
	UPROPERTY(EditAnywhere, Category = "Patrol", meta = (ClampMin = "1", ClampMax = "16"))
	int32 MaxAttempts = 6;

	/** Re-roll if no progress has been made for this long (recovery from being stuck). */
	UPROPERTY(EditAnywhere, Category = "Patrol", meta = (ClampMin = "1.0"))
	float StuckTimeout = 8.0f;

	/** If HomeLocation key is unset/zero, fall back to the AI's current location. */
	UPROPERTY(EditAnywhere, Category = "Patrol")
	bool bFallbackToCurrentLocation = true;

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

private:
	/** Shared logic between OnBecomeRelevant and TickNode. Returns true if a new point was written. */
	bool TryRollPatrolPoint(UBehaviorTreeComponent& OwnerComp, FMemory* Mem, bool bForceReroll) const;
};
