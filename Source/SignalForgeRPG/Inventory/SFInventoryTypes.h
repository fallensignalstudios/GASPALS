#pragma once

#include "CoreMinimal.h"
#include "Combat/SFWeaponInstanceTypes.h"
#include "GameplayTagContainer.h"
#include "Inventory/SFItemDefinition.h"
#include "SFInventoryTypes.generated.h"

class USFItemDefinition;

USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFInventoryEntry
{
	GENERATED_BODY()

public:
	FSFInventoryEntry()
		: EntryId(FGuid::NewGuid())
	{
	}

	explicit FSFInventoryEntry(USFItemDefinition* InItemDefinition, int32 InQuantity = 1)
		: EntryId(FGuid::NewGuid())
		, ItemDefinition(InItemDefinition)
		, Quantity(FMath::Max(1, InQuantity))
	{
		Normalize();
	}

	FSFInventoryEntry(USFItemDefinition* InItemDefinition, const FSFWeaponInstanceData& InWeaponInstanceData)
		: EntryId(FGuid::NewGuid())
		, ItemDefinition(InItemDefinition)
		, Quantity(1)
		, WeaponInstanceData(InWeaponInstanceData)
	{
		Normalize();
	}

	/** Stable runtime identity for this inventory entry. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FGuid EntryId;

	/** Static item definition backing this runtime entry. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TObjectPtr<USFItemDefinition> ItemDefinition = nullptr;

	/** Stack quantity. Unique-instance items should always normalize to 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (ClampMin = "1"))
	int32 Quantity = 1;

	/** Runtime weapon-specific state for rolled or otherwise unique weapons. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Instance")
	FSFWeaponInstanceData WeaponInstanceData;

	/** General per-entry runtime state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Instance")
	int32 CurrentDurability = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Instance")
	int32 ItemLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Instance")
	bool bFavorite = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Instance")
	bool bEquipped = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Instance")
	FGameplayTagContainer InstanceTags;

	/** Optional timestamp or game-time marker for cooldown / last use logic. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Instance")
	float LastUseTime = -FLT_MAX;

	bool IsValid() const
	{
		if (!EntryId.IsValid() || ItemDefinition == nullptr)
		{
			return false;
		}

		if (!ItemDefinition->IsDefinitionValid())
		{
			return false;
		}

		if (Quantity < 1)
		{
			return false;
		}

		if (IsWeaponEntry() && !WeaponInstanceData.IsValid())
		{
			return false;
		}

		if (IsUniqueEntry() && Quantity != 1)
		{
			return false;
		}

		if (HasDurability())
		{
			if (CurrentDurability < 0 || CurrentDurability > ItemDefinition->MaxDurability)
			{
				return false;
			}
		}
		else if (CurrentDurability != 0)
		{
			return false;
		}

		return true;
	}

	bool HasDefinition() const
	{
		return ItemDefinition != nullptr;
	}

	bool IsWeaponEntry() const
	{
		return ItemDefinition != nullptr && ItemDefinition->IsWeapon();
	}

	bool IsUniqueEntry() const
	{
		return ItemDefinition != nullptr && ItemDefinition->IsUniqueInventoryItem();
	}

	bool IsStackableEntry() const
	{
		return ItemDefinition != nullptr && ItemDefinition->CanStack() && !IsUniqueEntry();
	}

	bool HasWeaponInstanceData() const
	{
		return WeaponInstanceData.IsValid();
	}

	bool HasDurability() const
	{
		return ItemDefinition != nullptr && ItemDefinition->MaxDurability > 0;
	}

	bool IsBroken() const
	{
		return HasDurability() && CurrentDurability <= 0;
	}

	int32 GetMaxStackSize() const
	{
		return ItemDefinition ? ItemDefinition->GetEffectiveMaxStackSize() : 1;
	}

	int32 GetStackSpace() const
	{
		return FMath::Max(0, GetMaxStackSize() - Quantity);
	}

	float GetSingleItemWeight() const
	{
		return ItemDefinition ? ItemDefinition->Weight : 0.0f;
	}

	float GetStackWeight() const
	{
		return GetSingleItemWeight() * Quantity;
	}

	bool CanStackWith(const FSFInventoryEntry& Other) const
	{
		if (!IsStackableEntry() || !Other.IsStackableEntry())
		{
			return false;
		}

		if (ItemDefinition != Other.ItemDefinition)
		{
			return false;
		}

		if (HasWeaponInstanceData() || Other.HasWeaponInstanceData())
		{
			return false;
		}

		if (bEquipped || Other.bEquipped)
		{
			return false;
		}

		if (bFavorite != Other.bFavorite)
		{
			return false;
		}

		if (ItemLevel != Other.ItemLevel)
		{
			return false;
		}

		if (CurrentDurability != Other.CurrentDurability)
		{
			return false;
		}

		if (!(InstanceTags == Other.InstanceTags))
		{
			return false;
		}

		return true;
	}

	void EnsureEntryId()
	{
		if (!EntryId.IsValid())
		{
			EntryId = FGuid::NewGuid();
		}
	}

	void Normalize()
	{
		EnsureEntryId();

		ItemLevel = FMath::Max(1, ItemLevel);
		Quantity = FMath::Max(1, Quantity);

		if (!ItemDefinition)
		{
			Quantity = 1;
			CurrentDurability = 0;
			ItemLevel = 1;
			WeaponInstanceData = FSFWeaponInstanceData();
			InstanceTags.Reset();
			bEquipped = false;
			return;
		}

		if (IsUniqueEntry())
		{
			Quantity = 1;
		}
		else
		{
			Quantity = FMath::Clamp(Quantity, 1, GetMaxStackSize());
		}

		if (HasDurability())
		{
			if (CurrentDurability <= 0)
			{
				CurrentDurability = ItemDefinition->MaxDurability;
			}
			else
			{
				CurrentDurability = FMath::Clamp(CurrentDurability, 0, ItemDefinition->MaxDurability);
			}
		}
		else
		{
			CurrentDurability = 0;
		}

		if (!IsWeaponEntry())
		{
			WeaponInstanceData = FSFWeaponInstanceData();
		}

		if (IsStackableEntry())
		{
			bEquipped = false;
		}
	}
};