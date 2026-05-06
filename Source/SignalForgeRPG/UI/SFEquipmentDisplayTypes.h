#pragma once

#include "CoreMinimal.h"
#include "Combat/SFWeaponInstanceTypes.h"
#include "Inventory/SFSlotTypes.h"
#include "SFEquipmentDisplayTypes.generated.h"

class USFItemDefinition;
class USFWeaponData;
class UTexture2D;

UENUM(BlueprintType)
enum class ESFEquipmentSlotType : uint8
{
	Head,
	Chest,
	Arms,
	Legs,
	Boots,
	PrimaryWeapon,
	SecondaryWeapon,
	HeavyWeapon
};

USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFEquipmentDisplayEntry
{
	GENERATED_BODY()

public:
	/** UI-facing slot grouping/type. */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	ESFEquipmentSlotType SlotType = ESFEquipmentSlotType::Head;

	/** Actual equipment slot represented in the equipment component. */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	ESFEquipmentSlot EquipmentSlot = ESFEquipmentSlot::Head;

	/** User-facing slot label. */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	FText SlotDisplayName = FText::GetEmpty();

	/** Whether anything is currently equipped in this slot. */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	bool bHasItemEquipped = false;

	/** Convenience UI flags. */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	bool bIsWeaponSlot = false;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	bool bIsInteractable = true;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	bool bCanAcceptEquipment = true;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	bool bIsActive = false;

	/** Equipped item display data. */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	FText EquippedItemName = FText::GetEmpty();

	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UTexture2D> EquippedItemIcon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<USFItemDefinition> EquippedItemDefinition = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<USFWeaponData> EquippedWeaponData = nullptr;

	/** Stable link back to the inventory entry if this equipped item came from inventory. */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	FGuid EquippedEntryId;

	/** Runtime weapon state if this slot contains a weapon instance. */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	FSFWeaponInstanceData EquippedWeaponInstance;

	bool IsEmpty() const
	{
		return !bHasItemEquipped || EquippedItemDefinition == nullptr;
	}

	bool HasValidEquippedEntry() const
	{
		return EquippedEntryId.IsValid();
	}

	bool HasWeaponInstance() const
	{
		return bIsWeaponSlot && EquippedWeaponInstance.IsValid();
	}
};