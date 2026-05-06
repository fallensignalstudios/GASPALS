#pragma once

#include "CoreMinimal.h"
#include "SFInventoryDisplayTypes.generated.h"

class UTexture2D;
class USFItemDefinition;

/**
 * UI-friendly representation of an inventory entry.
 * Long-term safe for sorting/filtering by carrying a stable source reference.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFInventoryDisplayEntry
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<USFItemDefinition> ItemDefinition = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 Quantity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	bool bIsEquipped = false;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	bool bIsStackable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FName ItemId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FGuid InventoryEntryGuid;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 SourceInventoryIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UTexture2D> Icon = nullptr;

public:
	FSFInventoryDisplayEntry()
	{
	}

	FSFInventoryDisplayEntry(
		USFItemDefinition* InItemDefinition,
		const FText& InDisplayName,
		int32 InQuantity,
		bool bInIsEquipped,
		bool bInIsStackable,
		FName InItemId,
		const FGuid& InInventoryEntryGuid,
		int32 InSourceInventoryIndex,
		UTexture2D* InIcon = nullptr
	)
		: ItemDefinition(InItemDefinition)
		, DisplayName(InDisplayName)
		, Quantity(InQuantity)
		, bIsEquipped(bInIsEquipped)
		, bIsStackable(bInIsStackable)
		, ItemId(InItemId)
		, InventoryEntryGuid(InInventoryEntryGuid)
		, SourceInventoryIndex(InSourceInventoryIndex)
		, Icon(InIcon)
	{
	}

	bool IsValid() const
	{
		return ItemDefinition != nullptr
			&& InventoryEntryGuid.IsValid()
			&& SourceInventoryIndex != INDEX_NONE;
	}
};