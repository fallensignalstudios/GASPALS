#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "SFBTTask_MoveToCover.generated.h"

/**
 * Picks a navigable point that breaks line-of-sight to the target stored in
 * the BB Actor key and moves the AI there. Used to seek cover before reloading
 * or while shields regenerate.
 *
 * Algorithm: sample SampleCount candidate points in an annulus around the AI
 * (between MinSearchRadius and MaxSearchRadius), project each to the navmesh,
 * then line-trace from the candidate's eye height to the target's eye height.
 * The first nav-reachable candidate whose trace is BLOCKED (i.e. cover) and
 * whose distance from the target is at least MinDistanceFromTarget wins.
 *
 * Succeeds when the cover point is reached, fails if no covered point can be
 * resolved or the AI can't get there before TimeoutSeconds.
 */
UCLASS()
class SIGNALFORGERPG_API USFBTTask_MoveToCover : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	USFBTTask_MoveToCover();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FMemory); }
	virtual FString GetStaticDescription() const override;

protected:
	/** Inner radius of the search annulus around the AI (cm). */
	UPROPERTY(EditAnywhere, Category = "Cover", meta = (ClampMin = "100.0"))
	float MinSearchRadius = 400.0f;

	/** Outer radius of the search annulus around the AI (cm). */
	UPROPERTY(EditAnywhere, Category = "Cover", meta = (ClampMin = "200.0"))
	float MaxSearchRadius = 1200.0f;

	/** Minimum distance the cover point must be from the target (cm). */
	UPROPERTY(EditAnywhere, Category = "Cover", meta = (ClampMin = "0.0"))
	float MinDistanceFromTarget = 500.0f;

	/** How many random candidate points to evaluate. */
	UPROPERTY(EditAnywhere, Category = "Cover", meta = (ClampMin = "1", ClampMax = "32"))
	int32 SampleCount = 12;

	/** Eye-height offset added to candidate / target for the LOS trace (cm). */
	UPROPERTY(EditAnywhere, Category = "Cover", meta = (ClampMin = "0.0"))
	float EyeHeightOffset = 60.0f;

	UPROPERTY(EditAnywhere, Category = "Cover", meta = (ClampMin = "10.0"))
	float AcceptanceRadius = 80.0f;

	UPROPERTY(EditAnywhere, Category = "Cover", meta = (ClampMin = "0.1"))
	float TimeoutSeconds = 6.0f;

	struct FMemory
	{
		float Elapsed = 0.0f;
		bool bMoveIssued = false;
	};
};
