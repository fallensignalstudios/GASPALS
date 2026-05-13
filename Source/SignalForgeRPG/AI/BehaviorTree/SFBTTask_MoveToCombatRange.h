#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "SFBTTask_MoveToCombatRange.generated.h"

/**
 * Drives the AI toward (or away from) the target stored in the configured BB
 * Actor key until current distance falls inside the engagement band
 * [MinDistance, MaxDistance]. The band prevents oscillation that happens when
 * you compare against a single magic number.
 *
 * Distance limits can be auto-derived from the AI's currently-equipped weapon
 * range (RangedConfig.HitscanMaxRange / FalloffEnd / melee TraceLength) by
 * leaving bUseWeaponRange = true. In that case:
 *   - MaxDistance = WeaponRange * MaxRangeMultiplier (default 0.85 -> stay
 *     inside effective range with a safety margin).
 *   - MinDistance = WeaponRange * MinRangeMultiplier (default 0.25 for
 *     ranged, melee uses MeleeMinDistance directly).
 *
 * Re-issues a MoveTo every RepathInterval seconds while the target moves, and
 * succeeds the frame the AI is inside the band.
 *
 * Fails fast if no target or the AI cannot reach the target (path not found).
 */
UCLASS()
class SIGNALFORGERPG_API USFBTTask_MoveToCombatRange : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	USFBTTask_MoveToCombatRange();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FMemory); }
	virtual FString GetStaticDescription() const override;

protected:
	/** Derive Min/Max from the currently-equipped weapon's data. */
	UPROPERTY(EditAnywhere, Category = "Range")
	bool bUseWeaponRange = true;

	/** Explicit max engagement distance (cm). Used when bUseWeaponRange = false. */
	UPROPERTY(EditAnywhere, Category = "Range", meta = (EditCondition = "!bUseWeaponRange", ClampMin = "50.0"))
	float MaxDistance = 1500.0f;

	/** Explicit min engagement distance (cm). Used when bUseWeaponRange = false. */
	UPROPERTY(EditAnywhere, Category = "Range", meta = (EditCondition = "!bUseWeaponRange", ClampMin = "0.0"))
	float MinDistance = 400.0f;

	/** Multiplier applied to weapon range to get Max. Used when bUseWeaponRange = true. */
	UPROPERTY(EditAnywhere, Category = "Range", meta = (EditCondition = "bUseWeaponRange", ClampMin = "0.1", ClampMax = "1.0"))
	float MaxRangeMultiplier = 0.85f;

	/** Multiplier applied to weapon range to get Min. Used when bUseWeaponRange = true. */
	UPROPERTY(EditAnywhere, Category = "Range", meta = (EditCondition = "bUseWeaponRange", ClampMin = "0.0", ClampMax = "1.0"))
	float MinRangeMultiplier = 0.25f;

	/** Acceptance radius passed to MoveTo. */
	UPROPERTY(EditAnywhere, Category = "Move", meta = (ClampMin = "10.0"))
	float AcceptanceRadius = 60.0f;

	/** Seconds between re-issued MoveTo calls when the target keeps moving. */
	UPROPERTY(EditAnywhere, Category = "Move", meta = (ClampMin = "0.1"))
	float RepathInterval = 0.4f;

	/** If true, the task uses pathfinding (recommended). */
	UPROPERTY(EditAnywhere, Category = "Move")
	bool bUsePathfinding = true;

	struct FMemory
	{
		float TimeSinceRepath = 0.0f;
		FVector LastTargetLoc = FVector::ZeroVector;
		bool bMoveIssued = false;
	};
};
