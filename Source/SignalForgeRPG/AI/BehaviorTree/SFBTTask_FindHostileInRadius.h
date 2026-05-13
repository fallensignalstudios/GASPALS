#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "SFBTTask_FindHostileInRadius.generated.h"

/**
 * Scans the world for hostile ASFCharacterBase pawns within Radius of the AI,
 * picks the closest, and writes it to the configured Actor BB key.
 *
 * Use as a fallback when AI perception hasn't caught the target yet (e.g.
 * the AI just spawned and hasn't run a sense tick yet, or the player is
 * crouched / silent so perception alone won't trigger). Also useful for
 * "panic targeting" where a unit instantly knows about close-range threats.
 *
 * Succeeds when a hostile was found and written to the BB key.
 * Fails when no hostile is in range; the BB key is left untouched on failure
 * unless bClearOnFail = true.
 */
UCLASS()
class SIGNALFORGERPG_API USFBTTask_FindHostileInRadius : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	USFBTTask_FindHostileInRadius();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	/** Search radius in centimeters. */
	UPROPERTY(EditAnywhere, Category = "Search", meta = (ClampMin = "100.0"))
	float Radius = 2500.0f;

	/** Require unobstructed line-of-sight to count a candidate. */
	UPROPERTY(EditAnywhere, Category = "Search")
	bool bRequireLineOfSight = false;

	/** Clear the BB key when no hostile is found instead of leaving it alone. */
	UPROPERTY(EditAnywhere, Category = "Search")
	bool bClearOnFail = false;
};
