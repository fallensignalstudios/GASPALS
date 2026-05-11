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

	/** Degrees of vertical recoil kicked per shot (controller pitch up). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = "0.0"))
	float VerticalRecoil = 0.6f;

	/** Degrees of horizontal recoil kicked per shot (controller yaw, randomized sign). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = "0.0"))
	float HorizontalRecoil = 0.25f;

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

	// If null, weapon does not require ammo.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo")
	TSubclassOf<USFItemDefinition> RequiredAmmoItemDefinition = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = "0"))
	int32 ClipSize = 0;

	// Optional: per-shot ammo cost for special weapons.
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

		return bHasErrors ? EDataValidationResult::Invalid : Result;
	}
#endif
};