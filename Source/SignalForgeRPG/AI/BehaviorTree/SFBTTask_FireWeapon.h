#pragma once

#include "CoreMinimal.h"
#include "AI/SFCombatStyles.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "GameplayTagContainer.h"
#include "SFBTTask_FireWeapon.generated.h"

UENUM()
enum class ESFBTFireMode : uint8
{
	Tap,
	Hold,
	PressOnly,
	Burst,
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
 *   - PressOnly: send Pressed and succeed immediately. Pair with the
 *     StopFiring task to release later (full automatic / multi-tick beam).
 *   - Burst: send N Pressed/Released pairs separated by BurstInterShotDelay.
 *     Halo-grunt feel; preferred for rifle enemies.
 *
 * Combat-style preset: when CombatStyle != Custom, the named preset overrides
 * all the manually-authored Fire / ADS / Burst fields. Choose a preset for
 * standard enemies; switch to Custom only for one-off bosses or scripted moments.
 *
 * The task auto-fails if the AI has no ability system or no equipped weapon.
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
	/**
	 * Pick a high-level combat style preset. Custom keeps all the explicit
	 * fields below in play; everything else overrides them with named tuning.
	 */
	UPROPERTY(EditAnywhere, Category = "Combat Style")
	ESFCombatStyle CombatStyle = ESFCombatStyle::BurstShooter;

	UPROPERTY(EditAnywhere, Category = "Fire", meta = (EditCondition = "CombatStyle == ESFCombatStyle::Custom"))
	ESFBTFireMode FireMode = ESFBTFireMode::Tap;

	/** Seconds to hold input for Hold mode. Ignored in Tap / PressOnly / Burst. */
	UPROPERTY(EditAnywhere, Category = "Fire", meta = (ClampMin = "0.0", EditCondition = "CombatStyle == ESFCombatStyle::Custom && FireMode == ESFBTFireMode::Hold"))
	float HoldDuration = 0.5f;

	/** Number of shots in a burst. Ignored unless FireMode == Burst. */
	UPROPERTY(EditAnywhere, Category = "Fire|Burst", meta = (ClampMin = "1", EditCondition = "CombatStyle == ESFCombatStyle::Custom && FireMode == ESFBTFireMode::Burst"))
	int32 BurstShotCount = 3;

	/** Seconds between shots in a burst. */
	UPROPERTY(EditAnywhere, Category = "Fire|Burst", meta = (ClampMin = "0.0", EditCondition = "CombatStyle == ESFCombatStyle::Custom && FireMode == ESFBTFireMode::Burst"))
	float BurstInterShotDelay = 0.10f;

	/**
	 * When true the task presses the ADS input tag, waits AdsLeadInSeconds for
	 * the ADS ability to settle (camera FOV, weapon sway), then sends the
	 * primary-fire press. ADS is released when the task completes (or aborts).
	 * When false the task fires from the hip immediately.
	 */
	UPROPERTY(EditAnywhere, Category = "Fire|ADS", meta = (EditCondition = "CombatStyle == ESFCombatStyle::Custom"))
	bool bRequiresAds = true;

	/** ADS settle time before firing. Ignored when bRequiresAds is false. */
	UPROPERTY(EditAnywhere, Category = "Fire|ADS", meta = (ClampMin = "0.0", ClampMax = "2.0", EditCondition = "CombatStyle == ESFCombatStyle::Custom && bRequiresAds"))
	float AdsLeadInSeconds = 0.35f;

	enum class EFirePhase : uint8
	{
		AdsRamp,
		Holding,
		BurstShooting,
		BurstInterShotWait,
		Done,
	};

	struct FMemory
	{
		float ElapsedSeconds = 0.0f;
		int32 BurstShotsFired = 0;
		bool bFirePressed = false;
		bool bAdsPressed = false;
		EFirePhase Phase = EFirePhase::AdsRamp;

		// Resolved tuning for this activation (after style override applied).
		ESFBTFireMode RuntimeFireMode = ESFBTFireMode::Tap;
		float RuntimeHoldDuration = 0.5f;
		int32 RuntimeBurstShotCount = 3;
		float RuntimeBurstInterShotDelay = 0.1f;
		bool bRuntimeRequiresAds = false;
		float RuntimeAdsLeadInSeconds = 0.0f;
	};

private:
	void SendFirePressed(UBehaviorTreeComponent& OwnerComp);
	void SendFireReleased(UBehaviorTreeComponent& OwnerComp);
	void SendAdsPressed(UBehaviorTreeComponent& OwnerComp);
	void SendAdsReleased(UBehaviorTreeComponent& OwnerComp);

	/** Apply CombatStyle preset (or copy explicit fields if Custom) into FMemory's Runtime* fields. */
	void ResolveRuntimeTuning(FMemory& Mem) const;

	/** Fire-shot transition: presses fire, configures FMemory state based on resolved FireMode, returns the BT result to propagate. */
	EBTNodeResult::Type BeginFiring(UBehaviorTreeComponent& OwnerComp, FMemory& Mem);

	/** Single-shot used inside Burst mode: press then release in the same call, advance counter. */
	void FireBurstShot(UBehaviorTreeComponent& OwnerComp, FMemory& Mem);
};
