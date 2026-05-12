#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Animation/SFAnimationTypes.h"
#include "Combat/SFWeaponStatBlock.h"
#include "Combat/SFWeaponActor.h"
#include "Misc/DataValidation.h"
#include "Combat/SFWeaponAnimationSet.h"
#include "SFWeaponData.generated.h"

class UTexture2D;
class UStaticMesh;
class USkeletalMesh;
class UAnimMontage;
class UAnimInstance;
class USFWeaponAnimationSet;
class USFItemDefinition;
class ASFProjectileBase;
class UGameplayAbility;
class UGameplayEffect;
class USoundBase;
class UCameraShakeBase;
class UCurveFloat;
class USFAmmoType;

/** High-level fire mode for ranged weapons. */
UENUM(BlueprintType)
enum class ESFWeaponFireMode : uint8
{
	Single		UMETA(DisplayName = "Single Shot"),
	Burst		UMETA(DisplayName = "Burst Fire"),
	FullAuto	UMETA(DisplayName = "Full Auto"),
	Charge		UMETA(DisplayName = "Charge / Hold"),
	Beam		UMETA(DisplayName = "Continuous Beam")
};

/**
 * Per-weapon ranged tuning. Drives the default ranged fire ability and weapon-granted abilities.
 * Designed so that designers can swap rifle/pistol/shotgun/sniper feel by editing a single struct.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFRangedWeaponConfig
{
	GENERATED_BODY()

	/** Fire mode determines whether holding the trigger keeps firing, fires bursts, etc. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire")
	ESFWeaponFireMode FireMode = ESFWeaponFireMode::Single;

	/** Cyclic rate of fire, used to derive cycle time between shots. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire", meta = (ClampMin = "30.0", ClampMax = "2400.0"))
	float RoundsPerMinute = 480.0f;

	/** Rounds per burst when FireMode == Burst. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire", meta = (ClampMin = "1", ClampMax = "16"))
	int32 BurstCount = 3;

	/** Seconds between shots inside a burst (defaults to 60/RPM if zero). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire", meta = (ClampMin = "0.0"))
	float BurstIntervalSeconds = 0.05f;

	/** If true, use a hitscan trace; otherwise spawn ProjectileClass. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire")
	bool bHitscan = true;

	/** Max trace distance for hitscan weapons. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire|Hitscan", meta = (ClampMin = "100.0", EditCondition = "bHitscan"))
	float HitscanMaxRange = 12000.0f;

	/** Projectile class spawned per shot when bHitscan is false. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire|Projectile", meta = (EditCondition = "!bHitscan"))
	TSubclassOf<ASFProjectileBase> ProjectileClass;

	/** Base damage applied per hit (used by hitscan; projectile uses its own damage effect). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0.0"))
	float BaseDamage = 18.0f;

	/** Damage effect class applied on hit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/** Distance (cm) at which damage starts falling off. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage|Falloff", meta = (ClampMin = "0.0"))
	float FalloffStartDistance = 2500.0f;

	/** Distance (cm) at which damage hits its minimum multiplier. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage|Falloff", meta = (ClampMin = "0.0"))
	float FalloffEndDistance = 7500.0f;

	/** Minimum damage multiplier at >= FalloffEndDistance (e.g. 0.35 = 35%). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage|Falloff", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinFalloffMultiplier = 0.35f;

	/** Optional curve overrides the simple Start/End falloff. X = distance in cm, Y = multiplier [0..1]. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage|Falloff")
	TObjectPtr<UCurveFloat> DamageFalloffCurve = nullptr;

	/** Multiplier when the trace hits a headshot bone (e.g. head/neck). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "1.0"))
	float HeadshotMultiplier = 2.0f;

	/** Pellets per shot: 1 for rifles/pistols, 8+ for shotguns. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spread", meta = (ClampMin = "1", ClampMax = "32"))
	int32 PelletsPerShot = 1;

	/** Cone half-angle in degrees while hipfiring. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spread", meta = (ClampMin = "0.0", ClampMax = "45.0"))
	float HipfireSpreadDegrees = 2.5f;

	/** Cone half-angle in degrees while aiming down sights. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spread", meta = (ClampMin = "0.0", ClampMax = "45.0"))
	float AdsSpreadDegrees = 0.4f;

	/** Camera FOV while in ADS (0 = leave camera FOV alone, useful for non-zoom weapons). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ADS", meta = (ClampMin = "0.0", ClampMax = "170.0"))
	float AdsCameraFOV = 55.0f;

	/** Spring-arm target length while ADS (0 = leave arm length alone). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ADS", meta = (ClampMin = "0.0"))
	float AdsSpringArmLength = 150.0f;

	/** Spring-arm socket-offset Y (right) while ADS, useful for shoulder-aim. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ADS")
	FVector AdsSpringArmSocketOffset = FVector(0.0f, 35.0f, 10.0f);

	/** Seconds to interp into ADS (zoom-in time). Smaller = snappier. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ADS", meta = (ClampMin = "0.01", ClampMax = "1.5"))
	float AdsBlendInSeconds = 0.18f;

	/** Seconds to interp back to hipfire on release. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ADS", meta = (ClampMin = "0.01", ClampMax = "1.5"))
	float AdsBlendOutSeconds = 0.12f;

	/** Movement speed multiplier while ADS (e.g. 0.55 = walk speed reduced 45%). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ADS", meta = (ClampMin = "0.1", ClampMax = "1.5"))
	float AdsMovementSpeedMultiplier = 0.55f;

	/** Recoil multiplier while ADS (e.g. 0.7 = 30% less recoil when aimed). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ADS", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float AdsRecoilMultiplier = 0.7f;

	/** Optional ADS pose montage looped while aimed (e.g. raised-rifle additive). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ADS")
	TObjectPtr<UAnimMontage> AdsPoseMontage = nullptr;

	/** Degrees of vertical recoil kicked per shot (smoothly interpolated upward, not instant). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = "0.0"))
	float VerticalRecoil = 0.6f;

	/** Degrees of horizontal recoil kicked per shot (smoothly interpolated, randomized sign). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = "0.0"))
	float HorizontalRecoil = 0.25f;

	/** How fast the controller eases toward the new recoil target (higher = snappier kick).
	 *  Used as the InterpSpeed in FMath::FInterpTo each tick. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = "1.0", ClampMax = "60.0"))
	float RecoilInterpSpeed = 18.0f;

	/** How fast the controller eases back toward the original aim after the kick. Lower
	 *  values feel "floaty," higher values snap back hard. 0 disables recovery. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = "0.0", ClampMax = "30.0"))
	float RecoilRecoverySpeed = 8.0f;

	/** Fraction of the kick that recovery returns to the player's original aim (1.0 = full
	 *  return, 0.0 = aim drift stays where the kick left it). 0.85 feels right for most rifles. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RecoilRecoveryFraction = 0.85f;

	/** Camera shake when firing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback")
	TSubclassOf<UCameraShakeBase> FireCameraShake;

	/** Optional fire montage override (defaults to weapon ability montage if null). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback")
	TObjectPtr<UAnimMontage> FireMontageOverride = nullptr;

	/** Montage played when trying to fire with empty clip. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback")
	TObjectPtr<UAnimMontage> EmptyClickMontage = nullptr;

	/** Sound when dry-firing on empty clip. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback")
	TObjectPtr<USoundBase> EmptyClickSound = nullptr;

	/** Optional gameplay cue tag override for muzzle flash (else uses default Cue.Weapon.MuzzleFlash). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback|Cues", meta = (Categories = "SignalForge.Cue"))
	FGameplayTag MuzzleFlashCueOverride;

	/** Optional gameplay cue tag override for tracer (else uses default Cue.Weapon.Tracer). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback|Cues", meta = (Categories = "SignalForge.Cue"))
	FGameplayTag TracerCueOverride;

	/** Optional gameplay cue tag override for shell ejection (else uses default Cue.Weapon.ShellEject). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Feedback|Cues", meta = (Categories = "SignalForge.Cue"))
	FGameplayTag ShellEjectCueOverride;

	/** Reload duration in seconds (used by default reload ability if no montage is set). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reload", meta = (ClampMin = "0.1"))
	float ReloadSeconds = 1.8f;

	/** Optional reload montage (default reload ability plays it on the owner). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reload")
	TObjectPtr<UAnimMontage> ReloadMontage = nullptr;

	/** Computes interval (seconds) between shots from RPM. */
	float GetCycleTime() const
	{
		return RoundsPerMinute > 0.0f ? (60.0f / RoundsPerMinute) : 0.1f;
	}
};

/**
 * Tuning for continuous-beam weapons (energy rifles, plasma cutters, trace rifles).
 * The beam fires as a stream of per-tick hitscans at a rate derived from the parent
 * FSFRangedWeaponConfig::RoundsPerMinute (RPM / 60 = ticks per second), so all fire
 * modes share one unified rate-of-fire knob. Damage, recoil, and battery drain are
 * specified per *tick* so designers can dial DPS independent of tick rate by tweaking
 * RPM or the per-tick values.
 *
 * Battery model: a float charge meter on the weapon instance that drains while held
 * and recharges (after a delay) while released. Empty battery overheats the weapon
 * and locks fire until charge climbs back above the resume threshold.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFBeamWeaponConfig
{
	GENERATED_BODY()

	/** Maximum beam length / hitscan trace distance, cm. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Beam", meta = (ClampMin = "100.0"))
	float BeamRange = 8000.0f;

	/** Damage applied per beam tick. Effective DPS = (RPM / 60) * DamagePerTick. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Beam|Damage", meta = (ClampMin = "0.0"))
	float DamagePerTick = 4.0f;

	/** If true, beam ticks roll headshot multiplier the same as bullet hits. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Beam|Damage")
	bool bAllowHeadshots = true;

	/** Total battery capacity (arbitrary units, designer tuning). Drained per tick, recharged over time. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Beam|Battery", meta = (ClampMin = "1.0"))
	float BatteryCapacity = 100.0f;

	/** Charge consumed per beam tick. With default capacity 100 and drain 2 at 1200 RPM (20 Hz) -> 2.5s sustained fire. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Beam|Battery", meta = (ClampMin = "0.0"))
	float DrainPerTick = 2.0f;

	/** Charge regenerated per second while the weapon is idle (after the recharge delay elapses). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Beam|Battery", meta = (ClampMin = "0.0"))
	float RechargeUnitsPerSecond = 40.0f;

	/** Seconds of idle (no fire) before recharge starts. Encourages tap-fire discipline. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Beam|Battery", meta = (ClampMin = "0.0"))
	float RechargeDelaySeconds = 1.0f;

	/**
	 * After overheating (battery hits 0) the weapon stays locked until charge rises above
	 * this fraction of capacity. Prevents stuttery 1-tick-on, 1-tick-off behaviour. 0.25 = wait
	 * for 25% charge before player can fire again.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Beam|Battery", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float OverheatResumeChargeFraction = 0.25f;

	/** Optional looping fire montage (held while beam is active). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Beam|Feedback")
	TObjectPtr<UAnimMontage> BeamLoopMontage = nullptr;

	/**
	 * Cue spawned once when the beam starts (loop VFX/audio handle).
	 * Stopped via Cue.Weapon.BeamStop. The cue receiver is responsible for spawning the
	 * Niagara ribbon and tracking it; param Location = muzzle, Normal = aim direction.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Beam|Feedback|Cues", meta = (Categories = "SignalForge.Cue"))
	FGameplayTag BeamStartCueOverride;

	/** Cue executed each tick to update the beam endpoint VFX (Location = hit/end, Normal = surface or aim). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Beam|Feedback|Cues", meta = (Categories = "SignalForge.Cue"))
	FGameplayTag BeamTickCueOverride;

	/** Cue executed when the beam stops (release / overheat / cancel). Tells the receiver to tear down the loop VFX. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Beam|Feedback|Cues", meta = (Categories = "SignalForge.Cue"))
	FGameplayTag BeamStopCueOverride;

	/** Sound played once when the beam overheats. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Beam|Feedback")
	TObjectPtr<USoundBase> OverheatSound = nullptr;
};

/**
 * Per-weapon melee tuning. Drives USFGameplayAbility_WeaponMelee.
 *
 * Design intent:
 *  - Combo chain authored as ordered montages (light + heavy lanes). Each chain index has its own
 *    damage so designers can ramp damage on the final combo step.
 *  - Hit-detection windows are authored in-montage via AnimNotifyState_SFMeleeWindow; the ability
 *    sweeps capsules between two weapon sockets every tick the window is open.
 *  - A "cancel window" near the end of each montage lets the next input chain smoothly instead
 *    of locking out till the entire montage finishes.
 *  - Hitstop is a brief local time dilation on the instigator (you, not the world) for impact feel.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFMeleeWeaponConfig
{
	GENERATED_BODY()

	/** Ordered light-attack montages. Pressing primary fire steps through this array; combo resets
	 *  to index 0 after ComboResetSeconds of idle (or when interrupted by damage / weapon swap). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee|Combo|Light")
	TArray<TObjectPtr<UAnimMontage>> LightComboMontages;

	/** Per-step damage for the light combo. If the array is shorter than LightComboMontages, the last
	 *  value is used for trailing steps. If empty, falls back to the base damage SetByCaller. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee|Combo|Light")
	TArray<float> LightComboDamages;

	/** Ordered heavy-attack montages (driven by SecondaryFire input). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee|Combo|Heavy")
	TArray<TObjectPtr<UAnimMontage>> HeavyComboMontages;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee|Combo|Heavy")
	TArray<float> HeavyComboDamages;

	/** Hit-detection sockets on the weapon mesh. Sweeps a capsule from start->end every tick the
	 *  AnimNotifyState_SFMeleeWindow is active. For dual-wield, the offhand actor is swept by
	 *  the same socket names on its own mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee|Hit Detection")
	FName TraceStartSocket = TEXT("MeleeTraceStart");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee|Hit Detection")
	FName TraceEndSocket = TEXT("MeleeTraceEnd");

	/** Capsule radius for the sweep. Larger = more forgiving hit detection. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee|Hit Detection", meta = (ClampMin = "0.5"))
	float TraceRadius = 6.0f;

	/** Friendly-fire toggle. False is the standard FPS/coop default. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee|Hit Detection")
	bool bCanFriendlyFire = false;

	/** Seconds of brief local time dilation applied to the instigator when a hit lands, for impact feel. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee|Feel", meta = (ClampMin = "0.0", ClampMax = "0.25"))
	float HitstopSeconds = 0.06f;

	/** Time dilation value during hitstop. Lower = more pronounced freeze. 1.0 disables hitstop. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee|Feel", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HitstopTimeDilation = 0.05f;

	/** GE applied to victims on hit (poise / stagger / knockback). Designer-authored. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee|Feel")
	TSubclassOf<UGameplayEffect> HitReactionEffect;

	/** Optional camera shake on landed hit (instigator camera). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee|Feel")
	TSubclassOf<UCameraShakeBase> HitCameraShake;

	/** Window in seconds after montage play during which queueing the next input chains smoothly.
	 *  Starts when a montage hits its cancel-window notify (see AnimNotifyState_SFMeleeCancel). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee|Combo", meta = (ClampMin = "0.0"))
	float InputBufferSeconds = 0.25f;

	/** Seconds of idle (no attack input, no hit) before combo step resets to 0. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee|Combo", meta = (ClampMin = "0.1"))
	float ComboResetSeconds = 1.0f;

	/** Cue executed when a swing lands on a valid target. Param Location = hit point, Normal = surface. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee|Feedback|Cues", meta = (Categories = "SignalForge.Cue"))
	FGameplayTag HitCueOverride;

	/** Cue executed when a swing's window closes without hitting anything (whiff swoosh). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee|Feedback|Cues", meta = (Categories = "SignalForge.Cue"))
	FGameplayTag WhiffCueOverride;
};

/** Combat tuning very similar to Narrative's AttackDamage + multipliers. */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFWeaponCombatTuning
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.01"))
	float AttacksPerSecond = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	FGameplayTag DamageTypeTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "1.0"))
	float CriticalMultiplier = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.0"))
	float LightAttackStaminaCost = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.0"))
	float HeavyAttackStaminaCost = 20.0f;
};

USTRUCT(BlueprintType)
struct FSFWeaponAttachmentSlotConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attachment")
	FName SocketName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attachment")
	FTransform RelativeOffset = FTransform::Identity;
};

USTRUCT(BlueprintType)
struct FSFWeaponAmmoConfig
{
	GENERATED_BODY()

	/** The pooled ammunition resource this weapon draws from on reload. If null,
	 *  the weapon does not require reserve ammo and reload magically refills
	 *  (useful for energy weapons that regenerate, or testing). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo")
	TObjectPtr<USFAmmoType> AmmoType = nullptr;

	/** Rounds the magazine can hold. 0 = no magazine (e.g. energy weapon with
	 *  built-in regen, melee, or hitscan-from-infinite). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = "0"))
	int32 ClipSize = 0;

	/** Per-shot ammo cost for special weapons (e.g. shotgun shell counts as one
	 *  shot but consumes 8 pellets worth — use 1 here, set Pellets in the
	 *  ranged config instead). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = "0"))
	int32 AmmoPerShot = 1;
};

/**
 * Weapon definition data asset.
 * Mirrors Narrative's WeaponItem responsibilities for visuals + animation,
 * but as a static asset that your inventory/equipment system instantiates.
 */
UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFWeaponData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Identity */

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Identity")
	FName WeaponId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Identity")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Identity", meta = (MultiLine = true))
	FText Description;

	/** High‑level combat mode (e.g. Melee, Ranged, Magic). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Identity")
	ESFCombatMode CombatMode = ESFCombatMode::None;

	/** Primary weapon type tag (e.g. SignalForge.Weapon.Sword). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Identity")
	FGameplayTag WeaponTypeTag;

	/** Additional descriptive tags (e.g. Elemental.Fire, Style.Heavy). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Identity")
	FGameplayTagContainer WeaponTags;

	/** Optional: gameplay tag that selects an entry in USFAnimInstanceBase::TaggedWeaponProfiles. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Animation")
	FGameplayTag AnimProfileTag;

	/** UI / Visuals */

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|UI")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Visual")
	TSubclassOf<ASFWeaponActor> WeaponActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Visual")
	TObjectPtr<UStaticMesh> StaticWeaponMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Visual")
	TObjectPtr<USkeletalMesh> SkeletalWeaponMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Visual")
	FName AttachSocketName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Visual")
	FTransform RelativeAttachTransform = FTransform::Identity;

	/** Core animation profile (idle/attacks/overlay). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Animation|Profile")
	FSFWeaponAnimationProfile AnimationProfile;

	/**
	 * Per‑weapon override/linked anim layer applied while this weapon is wielded.
	 * This is the Narrative‑style "WeaponAnimLayer".
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Animation|Layers")
	TSubclassOf<UAnimInstance> OverlayLinkedAnimLayerClass = nullptr;

	/**
	 * Form‑specific overrides for the overlay anim layer, keyed by gameplay tags
	 * (e.g. SignalForge.Character.Form.Heavy, SignalForge.Character.Form.Light).
	 * Mirrors Narrative's FormSpecificLayers map.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Animation|Layers",
		meta = (ForceInlineRow, Categories = "SignalForge.Character.Form"))
	TMap<FGameplayTag, TSubclassOf<UAnimInstance>> FormSpecificOverlayLayers;

	/**
	 * Data‑driven combo animation set for this weapon.
	 * This is your equivalent to Narrative's AttackComboAnimSet / GetComboAnims.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Animation|Set")
	TObjectPtr<USFWeaponAnimationSet> WeaponAnimationSet = nullptr;

	/** Base stats and caps */

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	FSFWeaponStatBlock BaseStats;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	FSFWeaponStatBlock MinStatCaps;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	FSFWeaponStatBlock MaxStatCaps;

	/** Combat tuning */

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Combat")
	FSFWeaponCombatTuning CombatTuning;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ammo")
	FSFWeaponAmmoConfig AmmoConfig;

	/** Ranged tuning (fire mode, spread, recoil, falloff). Used by the default ranged fire ability. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ranged")
	FSFRangedWeaponConfig RangedConfig;

	/** Beam tuning. Only consulted when RangedConfig.FireMode == Beam and the granted primary
	 *  fire ability is USFGameplayAbility_WeaponBeam. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Beam",
		meta = (EditCondition = "RangedConfig.FireMode == ESFWeaponFireMode::Beam"))
	FSFBeamWeaponConfig BeamConfig;

	/**
	 * Melee tuning. Consulted when the granted primary fire ability is USFGameplayAbility_WeaponMelee
	 * (i.e. swords, axes, batons, energy blades). Ranged config is ignored for melee weapons; melee
	 * has its own animation-driven cadence rather than RPM-based timing.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Melee")
	FSFMeleeWeaponConfig MeleeConfig;

	/**
	 * Two-handed weapons (greatswords, heavy rifles) occupy both hands. While a two-handed weapon
	 * is active, the equipment component refuses to spawn an offhand weapon, and the anim system
	 * uses the two-handed overlay (typically via FormSpecificOverlayLayers keyed off a form tag).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Wielding")
	bool bIsTwoHanded = false;

	/**
	 * Paired weapons (dual pistols, dual swords) spawn two ASFWeaponActor instances on a single
	 * equip. Primary fire alternates between the two muzzles/swing arcs each activation. The
	 * offhand actor is attached to OffhandAttachSocketName on the same character mesh.
	 *
	 * Cannot be combined with bIsTwoHanded -- the validator will flag this.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Wielding|Paired")
	bool bIsPairedWeapon = false;

	/** Actor class for the offhand. If null, falls back to WeaponActorClass (most common case:
	 *  identical pair of pistols). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Wielding|Paired",
		meta = (EditCondition = "bIsPairedWeapon"))
	TSubclassOf<ASFWeaponActor> OffhandWeaponActorClass;

	/** Skeletal mesh used by the offhand weapon actor. If null, the offhand falls back to the
	 *  mainhand SkeletalWeaponMesh (useful for symmetric pairs like matching dual swords).
	 *  Set this for asymmetric pairs (katana + wakizashi, mismatched pistols, etc.). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Wielding|Paired",
		meta = (EditCondition = "bIsPairedWeapon"))
	TObjectPtr<USkeletalMesh> OffhandSkeletalWeaponMesh = nullptr;

	/** Static mesh used by the offhand weapon actor. Same fallback rules as the skeletal slot. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Wielding|Paired",
		meta = (EditCondition = "bIsPairedWeapon"))
	TObjectPtr<UStaticMesh> OffhandStaticWeaponMesh = nullptr;

	/** Relative transform applied to the offhand mesh component. Falls back to the mainhand
	 *  RelativeAttachTransform if left at identity. Use this to mirror the offhand mesh
	 *  (typically a -1 X scale or a 180-degree yaw rotation) so a one-handed sword reads as a
	 *  left-hand sword instead of a clipped right-hand sword. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Wielding|Paired",
		meta = (EditCondition = "bIsPairedWeapon"))
	FTransform OffhandMeshRelativeTransform = FTransform::Identity;

	/** Hand socket for the offhand weapon while active. Defaults to a sensible left-hand grip name. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Wielding|Paired",
		meta = (EditCondition = "bIsPairedWeapon"))
	FName OffhandAttachSocketName = TEXT("hand_l_socket");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Wielding|Paired",
		meta = (EditCondition = "bIsPairedWeapon"))
	FTransform OffhandRelativeAttachTransform = FTransform::Identity;

	/** Holster socket for the offhand weapon when this slot is not the active one. Falls back to
	 *  OffhandAttachSocketName (offhand stays in hand) if unset -- match HolsteredAttachSocketName
	 *  for parity with the primary actor. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Wielding|Paired",
		meta = (EditCondition = "bIsPairedWeapon"))
	FName OffhandHolsteredAttachSocketName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Wielding|Paired",
		meta = (EditCondition = "bIsPairedWeapon"))
	FTransform OffhandHolsteredRelativeAttachTransform = FTransform::Identity;

	/** Primary fire ability granted while this weapon is equipped (e.g. WeaponFire). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Abilities")
	TSubclassOf<UGameplayAbility> PrimaryFireAbility;

	/** Secondary fire / aim-down-sights ability granted while equipped (optional). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Abilities")
	TSubclassOf<UGameplayAbility> SecondaryFireAbility;

	/** Reload ability granted while equipped (optional). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Abilities")
	TSubclassOf<UGameplayAbility> ReloadAbility;

	/** Additional weapon-granted abilities (special fire modes, melee bash, signature attacks). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Abilities")
	TArray<TSubclassOf<UGameplayAbility>> ExtraWeaponAbilities;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Attachments",
		meta = (Categories = "SignalForge.Equipment.Weapon.AttachSlot"))
	TMap<FGameplayTag, FSFWeaponAttachmentSlotConfig> AttachmentSlotConfig;

public:
	/** Simple accessors mirroring your old API */

	UFUNCTION(BlueprintPure, Category = "Weapon|Animation")
	const FSFWeaponAnimationProfile& GetAnimationProfile() const
	{
		return AnimationProfile;
	}

	UFUNCTION(BlueprintPure, Category = "Weapon|Animation")
	ESFOverlayMode GetOverlayMode() const
	{
		return AnimationProfile.OverlayMode;
	}

	UFUNCTION(BlueprintPure, Category = "Weapon|Animation")
	bool UsesUpperBodyOverlay() const
	{
		return AnimationProfile.bUseUpperBodyOverlay;
	}

	UFUNCTION(BlueprintPure, Category = "Weapon|Animation")
	UAnimMontage* GetLightAttackMontage() const
	{
		return AnimationProfile.LightAttackMontage;
	}

	UFUNCTION(BlueprintPure, Category = "Weapon|Animation")
	UAnimMontage* GetHeavyAttackMontage() const
	{
		return AnimationProfile.HeavyAttackMontage;
	}

	UFUNCTION(BlueprintPure, Category = "Weapon|Animation")
	UAnimMontage* GetAbilityMontage() const
	{
		return AnimationProfile.AbilityMontage;
	}

	/** Primary anim layer class (ignoring form‑specific overrides). */
	UFUNCTION(BlueprintPure, Category = "Weapon|Animation|Layers")
	TSubclassOf<UAnimInstance> GetOverlayLinkedAnimLayerClass() const
	{
		return OverlayLinkedAnimLayerClass;
	}

	/**
	 * Returns the best overlay layer for a given form tag, falling back to the base layer.
	 * This is your "per weapon override layer BP + tag‑driven variation".
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Animation|Layers")
	TSubclassOf<UAnimInstance> GetOverlayLayerForForm(const FGameplayTag& FormTag) const
	{
		if (const TSubclassOf<UAnimInstance>* Found = FormSpecificOverlayLayers.Find(FormTag))
		{
			return *Found;
		}
		return OverlayLinkedAnimLayerClass;
	}

	/** Data‑driven combo set access (weapon‑specific). */
	UFUNCTION(BlueprintPure, Category = "Weapon|Animation|Set")
	USFWeaponAnimationSet* GetWeaponAnimationSet() const
	{
		return WeaponAnimationSet;
	}

	/** Tag that anim instance can use to pull a profile from its TMap. */
	UFUNCTION(BlueprintPure, Category = "Weapon|Animation")
	FGameplayTag GetAnimProfileTag() const
	{
		return AnimProfileTag;
	}

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("WeaponDefinition"), GetFName());
	}

	// Holster visuals
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Visual")
	FName HolsteredAttachSocketName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Visual")
	FTransform HolsteredRelativeAttachTransform = FTransform::Identity;

	/**
	 * Time (seconds) it takes to swap *to* this weapon from a different slot, or from holstered
	 * to drawn. While this timer runs the owner has the State.Weapon.Switching tag, which blocks
	 * fire / reload / ADS abilities. Use 0 for instant snap (debug / cheat weapons).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Switching", meta = (ClampMin = "0.0"))
	float SwapTimeSeconds = 0.5f;

	/** Optional draw montage played on the character when this weapon becomes active. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Switching")
	TObjectPtr<UAnimMontage> DrawMontage = nullptr;

	/** Optional holster montage played on the character when this weapon leaves the active slot. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Switching")
	TObjectPtr<UAnimMontage> HolsterMontage = nullptr;

	// Optional crosshair UI
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|UI")
	TSubclassOf<UUserWidget> CrosshairWidgetClass;


#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override
	{
		EDataValidationResult Result = Super::IsDataValid(Context);
		bool bHasErrors = (Result == EDataValidationResult::Invalid);

		if (WeaponId == NAME_None)
		{
			Context.AddError(NSLOCTEXT("SFWeaponData", "WeaponIdRequired", "WeaponId must be set."));
			bHasErrors = true;
		}

		if (DisplayName.IsEmpty())
		{
			Context.AddError(NSLOCTEXT("SFWeaponData", "DisplayNameRequired", "DisplayName must not be empty."));
			bHasErrors = true;
		}

		if (!WeaponTypeTag.IsValid())
		{
			Context.AddWarning(NSLOCTEXT("SFWeaponData", "WeaponTypeTagMissing", "WeaponTypeTag is not set."));
		}

		if (!WeaponActorClass && !StaticWeaponMesh && !SkeletalWeaponMesh)
		{
			Context.AddError(NSLOCTEXT("SFWeaponData", "NoVisualRepresentation", "Weapon has no actor class or mesh assigned."));
			bHasErrors = true;
		}

		if (StaticWeaponMesh && SkeletalWeaponMesh)
		{
			Context.AddWarning(NSLOCTEXT("SFWeaponData", "BothWeaponMeshes", "Both StaticWeaponMesh and SkeletalWeaponMesh are assigned. Ensure runtime code has a clear preference."));
		}

		if (AttachSocketName == NAME_None)
		{
			Context.AddWarning(NSLOCTEXT("SFWeaponData", "AttachSocketMissing", "AttachSocketName is not set."));
		}

		if (CombatTuning.AttacksPerSecond <= 0.0f)
		{
			Context.AddError(NSLOCTEXT("SFWeaponData", "InvalidAttackRate", "AttacksPerSecond must be greater than 0."));
			bHasErrors = true;
		}

		if (bIsTwoHanded && bIsPairedWeapon)
		{
			Context.AddError(NSLOCTEXT("SFWeaponData", "TwoHandedAndPairedMutuallyExclusive", "bIsTwoHanded and bIsPairedWeapon cannot both be true. A two-handed weapon occupies both hands with a single grip; a paired weapon spawns a separate offhand actor. Pick one."));
			bHasErrors = true;
		}

		return bHasErrors ? EDataValidationResult::Invalid : Result;
	}
#endif
};