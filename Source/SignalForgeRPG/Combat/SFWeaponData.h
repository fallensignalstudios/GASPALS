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