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

	struct FMemory
	{
		float ElapsedSeconds = 0.0f;
		bool bPressed = false;
	};

private:
	void SendPressed(UBehaviorTreeComponent& OwnerComp);
	void SendReleased(UBehaviorTreeComponent& OwnerComp);
};
