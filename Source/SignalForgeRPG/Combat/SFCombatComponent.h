#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/SFHitTypes.h"                // FSFHitData, FSFResolvedHit, ESFAttackType, FSFCinematicFeedback
#include "GameplayTagContainer.h"
#include "SFCombatComponent.generated.h"

class USFEquipmentComponent;
class ASFWeaponActor;
class UGameplayEffect;
class ASFCharacterBase;
class UCameraShakeBase;

/**
 * Delegate broadcast every time a hit is resolved (successful melee impact).
 * Useful for HUD hit markers, audio managers, achievement trackers, and
 * cinematic systems that want to react without polling.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSFOnHitDelivered, const FSFHitData&, HitData, const FSFResolvedHit&, ResolvedHit);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SIGNALFORGERPG_API USFCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFCombatComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY()
	TObjectPtr<ASFCharacterBase> OwnerCharacter;

	/** LIGHT ATTACK TRACE SETTINGS */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Light")
	float LightAttackTraceStartOffset = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Light")
	float LightAttackTraceLength = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Light")
	float LightAttackTraceRadius = 50.0f;

	/** HEAVY ATTACK TRACE SETTINGS */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Heavy")
	float HeavyAttackTraceStartOffset = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Heavy")
	float HeavyAttackTraceLength = 125.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Heavy")
	float HeavyAttackTraceRadius = 70.0f;

	/** Actors already hit during the current attack window */
	UPROPERTY()
	TSet<TObjectPtr<AActor>> HitActorsThisAttack;

	/** Helper to get weapon socket trace points, returns false if not available */
	bool GetWeaponTracePoints(FVector& OutStart, FVector& OutEnd) const;

	/** ===== Cinematic feedback profiles ===== */

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Cinematic")
	FSFCinematicFeedback LightFeedback;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Cinematic")
	FSFCinematicFeedback HeavyFeedback;

	/** Multiplier applied to feedback intensity for critical strikes. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Cinematic", meta = (ClampMin = "1.0", ClampMax = "5.0"))
	float CriticalFeedbackMultiplier = 1.75f;

	/** Camera shake fired on a normal hit. Drop a UCameraShakeBase subclass here. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Cinematic")
	TSubclassOf<UCameraShakeBase> NormalHitCameraShake;

	/** Camera shake fired on a critical / weakpoint / heavy guard-break hit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Cinematic")
	TSubclassOf<UCameraShakeBase> HeavyHitCameraShake;

	/** Camera shake fired on a perfect parry / finisher. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Cinematic")
	TSubclassOf<UCameraShakeBase> CinematicCameraShake;

	/** Radius around the hit location within which players will receive camera shake. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Cinematic", meta = (ClampMin = "100.0"))
	float CameraShakeInnerRadius = 400.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|Cinematic", meta = (ClampMin = "200.0"))
	float CameraShakeOuterRadius = 2000.f;

	/** ===== Combo system ===== */

	/** Maximum time (real seconds) between hits before the combo chain resets. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combo", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float ComboWindowSeconds = 1.25f;

	/** Maximum chain index before wrapping back to 0. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Combo", meta = (ClampMin = "1", ClampMax = "32"))
	int32 MaxComboLength = 4;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Combo")
	int32 CurrentComboIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Combo")
	float LastAttackRealTime = -100.f;

	/** ===== Weakpoint detection ===== */

	/** Bone names that should be treated as weakpoints (e.g. "head", "neck"). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Crit")
	TArray<FName> WeakpointBoneNames;

	/** Bonus damage multiplier applied when a weakpoint is struck. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Crit", meta = (ClampMin = "1.0", ClampMax = "10.0"))
	float WeakpointDamageMultiplier = 2.0f;

	/** Default damage when the target has no resolver. Replaces the old hard-coded 20.f. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Tuning", meta = (ClampMin = "0.0"))
	float LightDefaultDamage = 18.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Tuning", meta = (ClampMin = "0.0"))
	float HeavyDefaultDamage = 32.f;

	/** Hit-stop bookkeeping (so overlapping hits don't compound dilation forever). */
	FTimerHandle HitStopTimerHandle;
	FTimerHandle SlowMoTimerHandle;
	float PreHitStopMeshTimeDilation = 1.f;
	float PreSlowMoGlobalTimeDilation = 1.f;
	TWeakObjectPtr<AActor> PendingHitStopTarget;

public:
	/** Broadcast on the server (and locally on clients running combat) after every confirmed hit. */
	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FSFOnHitDelivered OnHitDelivered;

	/** Called from animation notifies / abilities during light attack window */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void HandleLightAttackHit();

	/** Called from animation notifies / abilities during heavy attack window */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void HandleHeavyAttackHit();

	/** Called at start of an attack's active hit window */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void BeginAttackWindow();

	/** Called at end of an attack's active hit window */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void EndAttackWindow();

	/** Reset the active combo chain (e.g. when the character is staggered / takes a hit). */
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	void ResetCombo();

	UFUNCTION(BlueprintPure, Category = "Combat|Combo")
	int32 GetCurrentComboIndex() const { return CurrentComboIndex; }

	/** Lets BP / abilities push a manual cinematic event (e.g. parry timing window result). */
	UFUNCTION(BlueprintCallable, Category = "Combat|Cinematic")
	void PlayCinematicFeedback(FGameplayTag FeedbackTag, AActor* AtActor, FVector AtLocation);

protected:
	/** Core function to process an attack type (Light/Heavy) */
	void HandleAttackHitInternal(ESFAttackType AttackType);

	/** Multi-hit sweep trace using pawn channel */
	bool PerformAttackSweep(float StartOffset, float TraceLength, float TraceRadius, TArray<FHitResult>& OutHitResults) const;

	/** Build hit data from a trace result */
	FSFHitData BuildHitDataFromResult(AActor* HitActor, const FHitResult& HitResult, ESFAttackType AttackType) const;

	/** Apply resolved hit via GAS, including health/poise/guard changes and tags */
	void ApplyResolvedHitWithGAS(const FSFHitData& HitData, const FSFResolvedHit& ResolvedHit);

	/** Drive VFX/SFX/camera/hit stop based on hit outcome. Now implemented. */
	void TriggerHitFeedback(const FSFHitData& HitData, const FSFResolvedHit& ResolvedHit);

	/** Utility: get base damage effect for a given attack type from the owner */
	TSubclassOf<UGameplayEffect> GetDamageEffectForAttackType(ESFAttackType AttackType) const;

	/** Choose feedback profile for an attack type. */
	const FSFCinematicFeedback& GetFeedbackForAttackType(ESFAttackType AttackType) const;

	/** Returns one of HitReact.Direction.* based on the incoming hit relative to the target. */
	FGameplayTag ComputeDirectionalHitTag(const FSFHitData& HitData) const;

	/** Returns the severity tag (HitReact.Severity.*) appropriate for this hit. */
	FGameplayTag ComputeSeverityHitTag(const FSFHitData& HitData, const FSFResolvedHit& ResolvedHit) const;

	/** True if the hit landed on a registered weakpoint bone (case-insensitive). */
	bool IsWeakpointBone(FName BoneName) const;

	/** Apply a brief time dilation to the target (and optionally source) to sell the impact. */
	void ApplyHitStop(AActor* Target, const FSFCinematicFeedback& Feedback, float IntensityScale);

	/** Drop global time dilation for a cinematic moment (parry / finisher). */
	void ApplySlowMo(const FSFCinematicFeedback& Feedback, float IntensityScale);

	/** Restore the target's time dilation. Timer-driven. */
	UFUNCTION()
	void ClearHitStop();

	/** Restore global world time dilation. Timer-driven. */
	UFUNCTION()
	void ClearSlowMo();

	/** Dispatch a GameplayCue on the target's ASC (or burst it via the world if no ASC). */
	void ExecuteHitCue(AActor* Target, FGameplayTag CueTag, const FSFHitData& HitData, float Magnitude) const;

	/** Tell the local player camera manager to play a shake scaled by intensity. */
	void PlayCameraShakeForHit(const FVector& WorldLocation, const FSFCinematicFeedback& Feedback, float IntensityScale, TSubclassOf<UCameraShakeBase> ShakeOverride) const;
};
