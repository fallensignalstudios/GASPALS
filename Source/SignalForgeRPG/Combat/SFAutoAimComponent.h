#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SFAutoAimComponent.generated.h"

class AActor;
class ASFCharacterBase;
class APlayerController;

/**
 * USFAutoAimComponent
 *
 * Destiny-style three-layer aim assist for the player character. Lives on the
 * pawn (added to ASFPlayerCharacter) and is queried by ability code:
 *
 *   1. Bullet magnetism: rotate fire direction toward the best hostile target
 *      within MagnetismAngleDeg of the aim line. Capped so it never overrides
 *      a player explicitly aiming elsewhere \u2014 the bend is subtle.
 *      USFGameplayAbility_WeaponFire calls GetMagnetizedAimDirection() per shot.
 *
 *   2. Sticky aim (aim friction): while the reticle passes over a hostile
 *      target inside StickyAngleDeg, reduce the player's look-input
 *      multiplier so tracking feels weighted. SFPlayerCharacter::Look applies
 *      the returned multiplier to its yaw/pitch input.
 *
 *   3. Reticle nudge on ADS engage: USFGameplayAbility_ADS calls
 *      RequestReticleNudge() once on activation. We rotate the player's
 *      control rotation by the small offset to the closest hostile target
 *      inside NudgeAngleDeg \u2014 the satisfying snap of pulling the trigger.
 *
 * Per-weapon tuning lives in FSFRangedWeaponConfig::AutoAim (see SFWeaponData.h).
 * When the equipped weapon supplies a config block, those values override the
 * component's defaults. Pistols/scout rifles should run looser angles, snipers
 * tighter, MGs almost zero.
 *
 * Faction-aware: only acquires targets where USFFactionStatics::AreHostile
 * (self, candidate) is true. Friendlies and neutrals never trigger any layer.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class SIGNALFORGERPG_API USFAutoAimComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFAutoAimComponent();

	//~ Begin UActorComponent
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	//~ End UActorComponent

	// ---- Public API used by abilities & player input ----

	/**
	 * Returns an aim rotation rotated toward the best hostile target inside
	 * MagnetismAngleDeg. If no target qualifies, returns BaseEyeRotation
	 * unchanged. BaseEyeLocation is used as the cone apex.
	 *
	 * `MagnetismStrength` lerps the result: 0.0 = no bend (returns base),
	 * 1.0 = aim is rotated all the way onto target center-mass. Default 1.0;
	 * pass less for hipfire (typically 0.5\u20130.7 of ADS strength).
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|AutoAim")
	FRotator GetMagnetizedAimDirection(const FVector& BaseEyeLocation, const FRotator& BaseEyeRotation, float MagnetismStrength = 1.0f) const;

	/**
	 * Convenience wrapper that picks magnetism strength based on whether the
	 * pawn is ADS (full 1.0) or hipfiring (HipfireMagnetismFraction). Use this
	 * from USFGameplayAbility_WeaponFire so it never needs to know the cvar.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|AutoAim")
	FRotator GetMagnetizedAimDirectionForFireMode(const FVector& BaseEyeLocation, const FRotator& BaseEyeRotation, bool bIsAimingDownSights) const;

	/**
	 * Returns a multiplier in [MinStickyMul, 1.0] to apply to look input
	 * (yaw + pitch). When the reticle is on a hostile target inside the
	 * sticky cone, returns < 1 so tracking feels weighted. 1.0 otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|AutoAim")
	float GetLookInputMultiplier() const;

	/**
	 * Snap the player's control rotation toward the closest hostile target
	 * inside NudgeAngleDeg. Called from USFGameplayAbility_ADS::ActivateAbility.
	 * No-op if no target qualifies.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|AutoAim")
	void RequestReticleNudge();

	/**
	 * Melee variant of the reticle nudge: snaps the player's control rotation
	 * (yaw only, so the camera doesn't pitch up/down on a downhill enemy) toward
	 * the nearest hostile inside a much wider, shorter-range cone tuned for swing
	 * arcs. Called from USFGameplayAbility_WeaponMelee::ActivateAbility so the
	 * swing doesn't whiff just because the player isn't perfectly centered. Uses
	 * a temporary cone widening so it works even when the standard nudge cone is
	 * tight (snipers).
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|AutoAim")
	void RequestMeleeFacingSnap();

	/** Returns the actor currently selected as the auto-aim target this frame (or null). */
	UFUNCTION(BlueprintPure, Category = "Combat|AutoAim")
	AActor* GetCurrentTarget() const { return CachedTarget.Get(); }

protected:
	// ---- Tuning (component defaults; overridden per-weapon via FSFRangedWeaponConfig) ----

	/** Master switch for the whole component. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AutoAim")
	bool bEnabled = true;

	/** Maximum range to consider targets. Past this, nothing is acquired. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AutoAim", meta = (ClampMin = "100.0"))
	float MaxTargetRange = 6000.0f;

	/** Half-angle cone for bullet magnetism (in degrees). Wider = more forgiving. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AutoAim|Magnetism", meta = (ClampMin = "0.0", ClampMax = "20.0"))
	float MagnetismAngleDeg = 3.0f;

	/** Hipfire receives this fraction of magnetism. 1.0 = same as ADS, 0.5 = half, 0 = none. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AutoAim|Magnetism", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HipfireMagnetismFraction = 0.55f;

	/** Half-angle cone for sticky aim (in degrees). Should be > MagnetismAngleDeg \u2014 stickiness kicks in before magnetism. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AutoAim|Sticky", meta = (ClampMin = "0.0", ClampMax = "30.0"))
	float StickyAngleDeg = 6.0f;

	/** Minimum look-input multiplier when the reticle is dead-on a target. 0.5 \u2192 reticle moves at 50% speed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AutoAim|Sticky", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float MinStickyMultiplier = 0.55f;

	/** Half-angle cone for ADS-engage reticle nudge (in degrees). Wider catches more, but feels intrusive past ~8\u00b0. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AutoAim|Nudge", meta = (ClampMin = "0.0", ClampMax = "15.0"))
	float NudgeAngleDeg = 5.0f;

	/** Fraction of the offset the nudge actually applies. 1.0 = full snap, 0.7 = a satisfying tug. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AutoAim|Nudge", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float NudgeStrength = 0.7f;

	// ---- Melee facing-snap tuning ----

	/** Maximum distance to consider a candidate for the melee facing snap. Past this we don't twist toward anyone. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AutoAim|Melee", meta = (ClampMin = "50.0"))
	float MeleeFacingMaxRange = 350.0f;

	/** Half-angle cone (degrees) for the melee facing snap. Should be generous \u2014 a swing covers a wide arc. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AutoAim|Melee", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float MeleeFacingAngleDeg = 60.0f;

	/** Fraction of the offset the melee snap applies. 1.0 = fully face the target, 0.6 = lean toward them. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AutoAim|Melee", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MeleeFacingStrength = 0.85f;

	/** If true, the melee facing snap only rotates yaw (camera doesn't tilt on uneven terrain). Recommended. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AutoAim|Melee")
	bool bMeleeYawOnly = true;

	/** If true, require unobstructed line of sight to the candidate target. Disable for X-ray rifles. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AutoAim|Selection")
	bool bRequireLineOfSight = true;

	/** Visibility trace channel for LOS checks. Defaults to ECC_Visibility. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AutoAim|Selection")
	TEnumAsByte<ECollisionChannel> LineOfSightChannel = ECC_Visibility;

	/** If true, log every acquisition decision via UE_LOG. Use during tuning, off in shipping. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AutoAim|Debug")
	bool bVerboseLogging = false;

private:
	/** Refreshes CachedTarget by scoring all hostile pawns in range. Cheap; runs per tick. */
	void RefreshCachedTarget();

	/**
	 * Scores a candidate target. Lower = better. Returns -1 if disqualified
	 * (out of cone, out of range, blocked, dead, friendly).
	 * `AcceptCone` is the half-angle cone the caller cares about; callers
	 * pass the widest of (sticky, magnetism, nudge) to harvest candidates once.
	 */
	float ScoreCandidate(const AActor* Candidate, const FVector& EyeLoc, const FVector& EyeFwd, float AcceptConeRadians) const;

	/** Returns a representative aim point on the target (center mass or socket). */
	FVector GetTargetAimPoint(const AActor* Target) const;

	/** Pulls per-weapon tuning overrides into the local fields used this frame, if a ranged weapon is equipped. */
	void ResolveWeaponOverrides();

	// ---- Cached per-tick state ----

	TWeakObjectPtr<AActor> CachedTarget;

	/** Half-angle from camera forward to CachedTarget center this tick (radians). Used by sticky aim & nudge. */
	float CachedTargetAngleRad = 0.0f;

	/** Cached pointer to the owning ASFCharacterBase, refreshed in BeginPlay. */
	TWeakObjectPtr<ASFCharacterBase> CachedSelf;

	/** Cached PlayerController for control-rotation reads/writes. */
	TWeakObjectPtr<APlayerController> CachedPC;
};
