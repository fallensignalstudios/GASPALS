#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/SFHitTypes.h"          // FSFHitData, FSFResolvedHit, ESFAttackType
#include "GameplayTagContainer.h"
#include "SFHitReactionComponent.generated.h"

class UAnimMontage;
class USkeletalMeshComponent;

/**
 * Per-direction lookup of severity-keyed montages. Wrapped in a USTRUCT so it
 * can be the value type of a TMap (UE doesn't support TMap<Tag, TMap<...>>).
 */
USTRUCT(BlueprintType)
struct FSFHitReactionMontageRow
{
	GENERATED_BODY()

	/** Severity tag -> montage. Leave a key out to fall back to physics only for that pair. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (Categories = "HitReact.Severity"))
	TMap<FGameplayTag, TObjectPtr<UAnimMontage>> BySeverityToMontage;
};

/**
 * USFHitReactionComponent
 *
 * Drives PROCEDURAL hit reactions on any ASFCharacterBase. The reaction is
 * composed of three independently-tunable layers:
 *
 *   1. Physics impulse on the impacted bone (so a sword swing from the left
 *      actually shoves the torso to the right, etc.). Direction is the
 *      attacker -> impact vector projected onto the impact bone; magnitude
 *      scales with damage and the severity tag emitted by the combat system.
 *
 *   2. Physics blend on the bones below the impact bone, ramped in for the
 *      hit's "punch" frame and ramped out over BlendOutTime. This gives the
 *      mesh enough simulation to actually move under the impulse without
 *      ragdolling the whole character.
 *
 *   3. Optional directional anim montage looked up by
 *      HitReact.Direction.* x HitReact.Severity.*. If a montage is
 *      configured for the (direction, severity) pair, it is played on the
 *      character's AnimInstance. Designers can leave the maps empty and
 *      still get a usable physics-only reaction.
 *
 * Lifecycle:
 *   - Auto-added to ASFCharacterBase (see SFCharacterBase.cpp).
 *   - Server authoritative: combat code calls HandleIncomingHit on the
 *     server after every confirmed hit. The component handles its own
 *     reaction locally; designers wanting cosmetic-only replication can
 *     bind to OnHitReactionPlayed and broadcast manually.
 *
 * Intentionally NOT in this component:
 *   - GAS effect application (already handled by USFCombatComponent).
 *   - Damage numbers / floating text (HUD layer).
 *   - Death montages (HealthComponent / death logic handles those).
 *
 * BP extension point:
 *   - Override OnHitReactionStarted to drop your own VFX/SFX/screen-shake
 *     using the resolved direction/severity tags.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FSFOnHitReactionPlayed,
	const FSFHitData&, HitData,
	const FSFResolvedHit&, ResolvedHit,
	FGameplayTag, DirectionTag,
	FGameplayTag, SeverityTag);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), DisplayName = "SF Hit Reaction")
class SIGNALFORGERPG_API USFHitReactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFHitReactionComponent();

	// -------------------------------------------------------------------------
	// Public API
	// -------------------------------------------------------------------------

	/**
	 * Drive a procedural reaction for the given hit. Called by USFCombatComponent
	 * on the target's hit-reaction component immediately after the hit is
	 * resolved and applied via GAS. Safe to call on dead characters -- the
	 * component will skip itself if the owner is dead.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|HitReaction")
	void HandleIncomingHit(const FSFHitData& HitData, const FSFResolvedHit& ResolvedHit);

	/** Designer / BP-driven path: trigger a reaction directly from a direction + severity tag. */
	UFUNCTION(BlueprintCallable, Category = "Combat|HitReaction")
	void TriggerReaction(FGameplayTag DirectionTag, FGameplayTag SeverityTag, FName ImpactBone, FVector ImpulseWorldDirection, float ImpulseScale = 1.0f);

	/** Broadcast every time a hit reaction is processed (after impulse + montage are kicked off). */
	UPROPERTY(BlueprintAssignable, Category = "Combat|HitReaction|Events")
	FSFOnHitReactionPlayed OnHitReactionPlayed;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * Designer/BP override hook. Fires before the impulse + montage are dispatched.
	 * Use this to layer per-NPC reactions (VFX, screen flash, audio cues, voice barks).
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|HitReaction", meta = (DisplayName = "On Hit Reaction Started"))
	void ReceiveHitReactionStarted(const FSFHitData& HitData, const FSFResolvedHit& ResolvedHit, FGameplayTag DirectionTag, FGameplayTag SeverityTag);

	// -------------------------------------------------------------------------
	// Tunables (physics layer)
	// -------------------------------------------------------------------------

	/** Master switch for the physics-impulse layer. Disable to fall back to montages-only. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitReaction|Physics")
	bool bEnablePhysicsImpulse = true;

	/**
	 * Base impulse magnitude (kg*cm/s) applied to a Light/Front hit at the
	 * reference damage. Per-severity multipliers scale this up. Tune for the
	 * weight of your typical character (default assumes ~80kg humanoids).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitReaction|Physics", meta = (ClampMin = "0.0", ClampMax = "100000.0"))
	float BaseImpulseMagnitude = 4000.f;

	/**
	 * Reference damage that maps 1:1 onto BaseImpulseMagnitude. Hits below this
	 * scale down; hits above scale up, clamped by DamageImpulseScaleMin/Max.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitReaction|Physics", meta = (ClampMin = "1.0"))
	float ReferenceDamage = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitReaction|Physics", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float DamageImpulseScaleMin = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitReaction|Physics", meta = (ClampMin = "1.0", ClampMax = "10.0"))
	float DamageImpulseScaleMax = 2.5f;

	/**
	 * Bone-level physics blend weight to set on the impacted bone (and below)
	 * during the reaction. 0 = pure animation, 1 = pure ragdoll. ~0.35 reads
	 * well on humanoid sword hits without losing root motion.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitReaction|Physics", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PhysicsBlendWeight = 0.35f;

	/** How long the physics blend stays at PhysicsBlendWeight before ramping out (seconds). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitReaction|Physics", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float PhysicsHoldTime = 0.10f;

	/** Time to ramp PhysicsBlendWeight back to 0 after the hold (seconds). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitReaction|Physics", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float BlendOutTime = 0.25f;

	/**
	 * Bone names that we will not simulate even if hit (e.g. root / pelvis on
	 * a humanoid -- simulating those usually makes the whole character flop).
	 * The simulation pass walks UP the bone hierarchy until it finds a bone
	 * NOT in this set, then simulates from there.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitReaction|Physics")
	TArray<FName> BonesToNotSimulate;

	/**
	 * Fallback bone used when the incoming hit has no usable BoneName (e.g.
	 * projectile traces that didn't hit a physics asset). Should be a
	 * mid-torso bone like "spine_02".
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitReaction|Physics")
	FName FallbackImpactBone = TEXT("spine_02");

	// -------------------------------------------------------------------------
	// Tunables (montage layer)
	// -------------------------------------------------------------------------

	/** Master switch for the directional-montage layer. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitReaction|Montage")
	bool bEnableDirectionalMontage = true;

	/**
	 * Montages indexed by (direction, severity). Keys are HitReact.Direction.*
	 * tags; the value is the per-severity montage map. Leave any pair empty
	 * to fall back to the physics layer only.
	 *
	 * Recommended authoring: 4 directions x 4 severities = 16 short clips
	 * (10-25 frames each). Most projects ship with the full grid for
	 * heroes and only Light/Heavy for trash mobs.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitReaction|Montage", meta = (Categories = "HitReact.Direction"))
	TMap<FGameplayTag, FSFHitReactionMontageRow> MontagesByDirection;

	/** Base play rate for hit-react montages. Severity scales further. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitReaction|Montage", meta = (ClampMin = "0.1", ClampMax = "4.0"))
	float MontagePlayRate = 1.0f;

	/** If true and a montage is already playing on the AnimInstance, the new one will interrupt. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitReaction|Montage")
	bool bInterruptExistingMontage = true;

	// -------------------------------------------------------------------------
	// Tunables (severity multipliers)
	// -------------------------------------------------------------------------

	/**
	 * Per-severity impulse multiplier. Light=1.0, Heavy=1.6, Stagger=2.4,
	 * Launch=4.0 by default -- tune to taste. Keys are HitReact.Severity.*.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitReaction|Tuning", meta = (Categories = "HitReact.Severity"))
	TMap<FGameplayTag, float> SeverityImpulseMultipliers;

	/** Per-severity montage play-rate multiplier. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat|HitReaction|Tuning", meta = (Categories = "HitReact.Severity"))
	TMap<FGameplayTag, float> SeverityPlayRateMultipliers;

private:
	// -------------------------------------------------------------------------
	// Internals
	// -------------------------------------------------------------------------

	/** Returns the mesh we should drive, or null if owner has no skeletal mesh. */
	USkeletalMeshComponent* GetTargetMesh() const;

	/** True if the owning character is dead -- we skip reactions then so death anims play clean. */
	bool IsOwnerDead() const;

	/** Pick the bone we should simulate from given the raw impact bone. */
	FName ResolveSimulationBone(USkeletalMeshComponent* Mesh, FName ImpactBone) const;

	/** Compute the world-space impulse vector from source -> impact, normalized + scaled. */
	FVector ComputeImpulseVector(const FSFHitData& HitData, USkeletalMeshComponent* Mesh, FName ImpactBone) const;

	/** Look up the impulse multiplier for the severity tag, defaulting to 1.0. */
	float GetSeverityImpulseMultiplier(FGameplayTag SeverityTag) const;

	/** Look up the play-rate multiplier for the severity tag, defaulting to 1.0. */
	float GetSeverityPlayRateMultiplier(FGameplayTag SeverityTag) const;

	/** Look up the montage for a (direction, severity) pair, returning nullptr if not configured. */
	UAnimMontage* GetMontageFor(FGameplayTag DirectionTag, FGameplayTag SeverityTag) const;

	/** Timer-driven: start ramping PhysicsBlendWeight back to 0. */
	UFUNCTION()
	void BeginPhysicsBlendOut();

	/** Timer-driven: finalize blend-out by clearing simulation entirely. */
	UFUNCTION()
	void FinalizePhysicsBlend();

	/** Tracks the bone we're currently simulating from so we can cleanly tear it down. */
	FName ActiveSimulationBone = NAME_None;

	/** Timer handles for the blend-out staircase. */
	FTimerHandle BlendOutStartTimer;
	FTimerHandle BlendOutFinalizeTimer;
};

