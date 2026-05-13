#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "SFBTTask_FleeFromTarget.generated.h"

/**
 * Picks a navigable point directly away from the target stored in the BB Actor
 * key and moves the AI there. Used to disengage when low on health, when a
 * caster wants to reposition, or when the BT detects the AI is too close for
 * its weapon's effective range.
 *
 * Succeeds when the flee point is reached, fails if no navigable retreat
 * point can be resolved within MaxAttempts.
 */
UCLASS()
class SIGNALFORGERPG_API USFBTTask_FleeFromTarget : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	USFBTTask_FleeFromTarget();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FMemory); }
	virtual FString GetStaticDescription() const override;

protected:
	/** Desired distance (cm) the AI should put between itself and the target. */
	UPROPERTY(EditAnywhere, Category = "Flee", meta = (ClampMin = "100.0"))
	float FleeDistance = 1500.0f;

	/** Search radius around the ideal retreat point for navigable fallback. */
	UPROPERTY(EditAnywhere, Category = "Flee", meta = (ClampMin = "50.0"))
	float NavSearchRadius = 400.0f;

	UPROPERTY(EditAnywhere, Category = "Flee", meta = (ClampMin = "1", ClampMax = "16"))
	int32 MaxAttempts = 6;

	UPROPERTY(EditAnywhere, Category = "Flee", meta = (ClampMin = "10.0"))
	float AcceptanceRadius = 80.0f;

	/** Max time we're willing to spend on a single flee before giving up. */
	UPROPERTY(EditAnywhere, Category = "Flee", meta = (ClampMin = "0.1"))
	float TimeoutSeconds = 5.0f;

	struct FMemory
	{
		float Elapsed = 0.0f;
		bool bMoveIssued = false;
	};
};
