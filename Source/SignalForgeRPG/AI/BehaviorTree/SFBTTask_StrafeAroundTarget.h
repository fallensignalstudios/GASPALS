#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "SFBTTask_StrafeAroundTarget.generated.h"

UENUM()
enum class ESFBTStrafeDirection : uint8
{
	Auto,
	Left,
	Right,
};

/**
 * Picks a navigable point lateral to the target stored in the BB Actor key
 * (alternating left / right per call) at roughly the AI's current engagement
 * distance, and moves there. Used by combat trees to make ranged enemies feel
 * alive instead of standing still while firing.
 *
 * Direction:
 *   - Auto: alternate left/right every time the task runs.
 *   - Left / Right: forced direction (useful for paired flanker AI).
 *
 * Succeeds when the strafe point is reached, fails if no nav point can be
 * found within MaxAttempts probes.
 */
UCLASS()
class SIGNALFORGERPG_API USFBTTask_StrafeAroundTarget : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	USFBTTask_StrafeAroundTarget();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FMemory); }
	virtual FString GetStaticDescription() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Strafe")
	ESFBTStrafeDirection Direction = ESFBTStrafeDirection::Auto;

	/** Lateral offset along the strafe axis (cm). */
	UPROPERTY(EditAnywhere, Category = "Strafe", meta = (ClampMin = "50.0"))
	float StrafeDistance = 400.0f;

	/** Search radius around the ideal strafe point when looking for nav-reachable spot. */
	UPROPERTY(EditAnywhere, Category = "Strafe", meta = (ClampMin = "50.0"))
	float NavSearchRadius = 300.0f;

	UPROPERTY(EditAnywhere, Category = "Strafe", meta = (ClampMin = "1", ClampMax = "16"))
	int32 MaxAttempts = 4;

	UPROPERTY(EditAnywhere, Category = "Strafe", meta = (ClampMin = "10.0"))
	float AcceptanceRadius = 60.0f;

	/** Max time we're willing to spend on a single strafe before giving up. */
	UPROPERTY(EditAnywhere, Category = "Strafe", meta = (ClampMin = "0.1"))
	float TimeoutSeconds = 3.0f;

	struct FMemory
	{
		float Elapsed = 0.0f;
		bool bMoveIssued = false;
	};

private:
	/** Toggled per-instance to give Auto its alternating behavior. */
	UPROPERTY(Transient)
	mutable bool bLastWentRight = false;
};
