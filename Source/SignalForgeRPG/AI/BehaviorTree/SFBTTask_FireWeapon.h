#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "GameplayTagContainer.h"
#include "SFBTTask_FireWeapon.generated.h"

UENUM()
enum class ESFBTFireMode : uint8
{
	Tap,
	Hold,
	PressOnly,
};

/**
 * Activate the currently-equipped weapon's primary fire by pressing the
 * Input.PrimaryFire input tag on the AI's ability system. Works uniformly
 * for ranged, beam, melee, and caster weapons because all four primary-fire
 * abilities share the same input tag.
 *
 * Modes:
 *   - Tap: send Pressed immediately followed by Released, then succeed.
 *     Use for single-shot weapons (most enemies).
 *   - Hold: send Pressed, wait HoldDuration seconds, send Released, then succeed.
 *     Use for caster charges or beam sustains.
 *   - Press only: send Pressed and succeed immediately. Pair with the
 *     StopFiring task to release later (full automatic / multi-tick beam).
 *
 * The task auto-fails if the AI has no ability system, no equipped weapon,
 * or the primary fire ability is on cooldown / blocked.
 */
UCLASS()
class SIGNALFORGERPG_API USFBTTask_FireWeapon : public UBTTaskNode
{
	GENERATED_BODY()

public:
	USFBTTask_FireWeapon();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FMemory); }
	virtual FString GetStaticDescription() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Fire")
	ESFBTFireMode FireMode = ESFBTFireMode::Tap;

	/** Seconds to hold input for Hold mode. Ignored in Tap / PressOnly. */
	UPROPERTY(EditAnywhere, Category = "Fire", meta = (ClampMin = "0.0", EditCondition = "FireMode == ESFBTFireMode::Hold"))
	float HoldDuration = 0.5f;

	/**
	 * Optional ADS lead-in. When > 0 the task presses the ADS input tag, waits this many
	 * seconds for the ADS ability to settle (camera FOV, weapon sway), then sends the
	 * primary-fire press. ADS is released when the task completes (or aborts). Set to 0
	 * to fire from the hip immediately.
	 */
	UPROPERTY(EditAnywhere, Category = "Fire|ADS", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float AdsLeadInSeconds = 0.25f;

	enum class EFirePhase : uint8
	{
		AdsRamp,
		Holding,
		Done,
	};

	struct FMemory
	{
		float ElapsedSeconds = 0.0f;
		bool bFirePressed = false;
		bool bAdsPressed = false;
		EFirePhase Phase = EFirePhase::AdsRamp;
	};

private:
	void SendFirePressed(UBehaviorTreeComponent& OwnerComp);
	void SendFireReleased(UBehaviorTreeComponent& OwnerComp);
	void SendAdsPressed(UBehaviorTreeComponent& OwnerComp);
	void SendAdsReleased(UBehaviorTreeComponent& OwnerComp);

	/** Fire-shot transition: presses fire, configures FMemory state based on FireMode, returns the BT result the caller should propagate. */
	EBTNodeResult::Type BeginFiring(UBehaviorTreeComponent& OwnerComp, FMemory& Mem);
};
