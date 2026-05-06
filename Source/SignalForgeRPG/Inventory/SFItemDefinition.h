#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "SFItemTypes.h"
#include "Misc/DataValidation.h"
#include "Combat/SFWeaponData.h"
#include "SFItemDefinition.generated.h"

class UTexture2D;
class UStaticMesh;
class USkeletalMesh;
class UNiagaraSystem;
class USoundBase;
class USFWeaponData;

UENUM(BlueprintType)
enum class ESFItemRarity : uint8
{
	Common,
	Uncommon,
	Rare,
	Epic,
	Legendary
};

UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Identity
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Identity")
	FName ItemId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Identity")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Identity", meta = (MultiLine = true))
	FText Description;

	// Categorization
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Categorization")
	ESFItemType ItemType = ESFItemType::None;

	// High-level category as a GameplayTag, e.g. Item.Weapon.Sword
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Categorization")
	FGameplayTag ItemCategoryTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Categorization")
	ESFItemRarity Rarity = ESFItemRarity::Common;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Categorization")
	FGameplayTagContainer ItemTags;

	// Weapon linkage
	// Static base weapon definition used by equipment/combat.
	// Runtime rolls/perks/tuning live in FSFWeaponInstanceData, not here.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Weapon")
	TSoftObjectPtr<USFWeaponData> WeaponData;

	// UI
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|UI")
	TObjectPtr<UTexture2D> Icon = nullptr;

	// Inventory / stacking
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Inventory")
	bool bStackable = false;

	// If true, each owned copy is its own unique instance and never stacks.
	// Weapons with rolls/perks should generally use this.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Inventory")
	bool bUniqueInstance = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Inventory", meta = (ClampMin = "1"))
	int32 MaxStackSize = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Inventory", meta = (ClampMin = "1"))
	int32 MaxOwnedQuantity = 9999;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Inventory")
	bool bCanDrop = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Inventory")
	bool bCanSell = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Inventory")
	bool bCanDestroy = true;

	// Economy
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Economy", meta = (ClampMin = "0"))
	int32 BuyPrice = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Economy", meta = (ClampMin = "0"))
	int32 SellPrice = 0;

	// Weight / encumbrance driver
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Gameplay", meta = (ClampMin = "0.0"))
	float Weight = 0.0f;

	// Minimum character level required to equip/use
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Gameplay", meta = (ClampMin = "0"))
	int32 RequiredLevel = 0;

	// Durability definition (for items that can break)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Gameplay", meta = (ClampMin = "0"))
	int32 MaxDurability = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Gameplay")
	bool bIsConsumable = false;

	// World visuals
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|World")
	TSoftObjectPtr<UStaticMesh> WorldStaticMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|World")
	TSoftObjectPtr<USkeletalMesh> WorldSkeletalMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|World")
	TSoftObjectPtr<UNiagaraSystem> PickupEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|World")
	TSoftObjectPtr<USoundBase> PickupSound;

	// Optional: pickup actor class, if you spawn a specific pickup actor
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|World")
	TSoftClassPtr<AActor> PickupActorClass;

	// Helpers
	UFUNCTION(BlueprintPure, Category = "Item")
	bool IsWeapon() const
	{
		return ItemType == ESFItemType::Weapon;
	}

	UFUNCTION(BlueprintPure, Category = "Item")
	bool HasWeaponData() const
	{
		return !WeaponData.IsNull();
	}

	UFUNCTION(BlueprintPure, Category = "Item")
	USFWeaponData* GetWeaponData() const;

	UFUNCTION(BlueprintPure, Category = "Item")
	int32 GetEffectiveMaxStackSize() const
	{
		// Weapon items and unique-instance items should never stack.
		if (IsWeapon() || bUniqueInstance)
		{
			return 1;
		}

		return bStackable ? FMath::Max(1, MaxStackSize) : 1;
	}

	UFUNCTION(BlueprintPure, Category = "Item")
	bool CanStack() const
	{
		return GetEffectiveMaxStackSize() > 1;
	}

	UFUNCTION(BlueprintPure, Category = "Item")
	bool IsUniqueInventoryItem() const
	{
		return IsWeapon() || bUniqueInstance;
	}

	UFUNCTION(BlueprintPure, Category = "Item")
	bool HasItemTag(FGameplayTag TagToMatch, bool bExactMatch = false) const
	{
		return bExactMatch ? ItemTags.HasTagExact(TagToMatch) : ItemTags.HasTag(TagToMatch);
	}

	UFUNCTION(BlueprintPure, Category = "Item")
	bool IsDefinitionValid() const
	{
		if (ItemId == NAME_None || DisplayName.IsEmpty())
		{
			return false;
		}

		if (GetEffectiveMaxStackSize() < 1)
		{
			return false;
		}

		if (IsWeapon() && !HasWeaponData())
		{
			return false;
		}

		return true;
	}

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("ItemDefinition"), GetFName());
	}

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override
	{
		EDataValidationResult Result = Super::IsDataValid(Context);
		bool bHasErrors = (Result == EDataValidationResult::Invalid);
		bool bHasWarnings = false;

		if (ItemId == NAME_None)
		{
			Context.AddError(NSLOCTEXT("SFItemDefinition", "ItemIdRequired", "ItemId must be set."));
			bHasErrors = true;
		}

		if (DisplayName.IsEmpty())
		{
			Context.AddError(NSLOCTEXT("SFItemDefinition", "DisplayNameRequired", "DisplayName must not be empty."));
			bHasErrors = true;
		}

		if (ItemType == ESFItemType::None)
		{
			Context.AddWarning(NSLOCTEXT(
				"SFItemDefinition",
				"ItemTypeNone",
				"ItemType is None. Verify this item is intentionally uncategorized."
			));
			bHasWarnings = true;
		}

		if (IsWeapon())
		{
			if (WeaponData.IsNull())
			{
				Context.AddError(NSLOCTEXT(
					"SFItemDefinition",
					"WeaponMissingWeaponData",
					"Weapon items must reference a valid WeaponData asset."
				));
				bHasErrors = true;
			}

			if (bStackable)
			{
				Context.AddWarning(NSLOCTEXT(
					"SFItemDefinition",
					"WeaponMarkedStackable",
					"Weapon items should not be stackable. Runtime logic will treat weapons as non-stackable."
				));
				bHasWarnings = true;
			}

			if (!bUniqueInstance)
			{
				Context.AddWarning(NSLOCTEXT(
					"SFItemDefinition",
					"WeaponNotUniqueInstance",
					"Weapon items usually want bUniqueInstance = true so rolls, perks, and tuning remain per-instance."
				));
				bHasWarnings = true;
			}

			if (MaxStackSize != 1)
			{
				Context.AddWarning(NSLOCTEXT(
					"SFItemDefinition",
					"WeaponMaxStackNotOne",
					"Weapon items should use MaxStackSize = 1."
				));
				bHasWarnings = true;
			}
		}
		else if (!WeaponData.IsNull())
		{
			Context.AddWarning(NSLOCTEXT(
				"SFItemDefinition",
				"NonWeaponHasWeaponData",
				"WeaponData is assigned but ItemType is not Weapon."
			));
			bHasWarnings = true;
		}

		if (bUniqueInstance && bStackable)
		{
			Context.AddWarning(NSLOCTEXT(
				"SFItemDefinition",
				"UniqueAndStackable",
				"Item is marked UniqueInstance and Stackable; it will behave as non-stackable at runtime."
			));
			bHasWarnings = true;
		}

		if (!bStackable && MaxStackSize != 1 && !IsWeapon())
		{
			Context.AddWarning(NSLOCTEXT(
				"SFItemDefinition",
				"NonStackableMaxStack",
				"Non-stackable items should use MaxStackSize = 1. Runtime logic will normalize this."
			));
			bHasWarnings = true;
		}

		if (bStackable && !bUniqueInstance && MaxStackSize < 2)
		{
			Context.AddWarning(NSLOCTEXT(
				"SFItemDefinition",
				"StackableMinStack",
				"Stackable items usually want MaxStackSize >= 2."
			));
			bHasWarnings = true;
		}

		if (MaxOwnedQuantity < 1)
		{
			Context.AddError(NSLOCTEXT(
				"SFItemDefinition",
				"InvalidMaxOwnedQuantity",
				"MaxOwnedQuantity must be at least 1."
			));
			bHasErrors = true;
		}

		if (SellPrice > BuyPrice && BuyPrice > 0)
		{
			Context.AddWarning(NSLOCTEXT(
				"SFItemDefinition",
				"SellGreaterThanBuy",
				"SellPrice is greater than BuyPrice. Verify economy settings."
			));
			bHasWarnings = true;
		}

		if (!WorldStaticMesh.IsNull() && !WorldSkeletalMesh.IsNull())
		{
			Context.AddWarning(NSLOCTEXT(
				"SFItemDefinition",
				"BothMeshesAssigned",
				"Both WorldStaticMesh and WorldSkeletalMesh are assigned. Ensure your presentation code prefers one."
			));
			bHasWarnings = true;
		}

		if (RequiredLevel < 0)
		{
			Context.AddError(NSLOCTEXT(
				"SFItemDefinition",
				"NegativeRequiredLevel",
				"RequiredLevel cannot be negative."
			));
			bHasErrors = true;
		}

		if (MaxDurability < 0)
		{
			Context.AddError(NSLOCTEXT(
				"SFItemDefinition",
				"NegativeDurability",
				"MaxDurability cannot be negative."
			));
			bHasErrors = true;
		}

		if (bHasErrors)
		{
			return EDataValidationResult::Invalid;
		}

		if (bHasWarnings || Result == EDataValidationResult::Valid)
		{
			return bHasWarnings ? EDataValidationResult::Valid : Result;
		}

		return Result;
	}
#endif
};