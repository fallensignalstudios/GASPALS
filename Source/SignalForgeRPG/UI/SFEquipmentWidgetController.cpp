#include "UI/SFEquipmentWidgetController.h"

#include "Components/SFEquipmentComponent.h"
#include "Combat/SFWeaponData.h"
#include "Inventory/SFItemDefinition.h"

void USFEquipmentWidgetController::Initialize(USFEquipmentComponent* InEquipmentComponent)
{
	if (EquipmentComponent == InEquipmentComponent)
	{
		RefreshEquipmentDisplayData();
		return;
	}

	UnbindFromSources();
	EquipmentComponent = InEquipmentComponent;
	BindToSources();
	RefreshEquipmentDisplayData();
}

void USFEquipmentWidgetController::Deinitialize()
{
	UnbindFromSources();
	EquipmentComponent = nullptr;
	CurrentDisplayEntries.Reset();
	OnEquipmentDisplayDataUpdated.Broadcast();
}

void USFEquipmentWidgetController::BeginDestroy()
{
	UnbindFromSources();
	Super::BeginDestroy();
}

void USFEquipmentWidgetController::BindToSources()
{
	if (!EquipmentComponent)
	{
		return;
	}

	EquipmentComponent->OnEquipmentUpdated.RemoveDynamic(this, &USFEquipmentWidgetController::HandleEquipmentUpdated);
	EquipmentComponent->OnEquipmentUpdated.AddDynamic(this, &USFEquipmentWidgetController::HandleEquipmentUpdated);
}

void USFEquipmentWidgetController::UnbindFromSources()
{
	if (!EquipmentComponent)
	{
		return;
	}

	EquipmentComponent->OnEquipmentUpdated.RemoveDynamic(this, &USFEquipmentWidgetController::HandleEquipmentUpdated);
}

void USFEquipmentWidgetController::RefreshEquipmentDisplayData()
{
	RebuildDisplayEntries();
	OnEquipmentDisplayDataUpdated.Broadcast();
}

void USFEquipmentWidgetController::HandleEquipmentUpdated()
{
	RefreshEquipmentDisplayData();
}

bool USFEquipmentWidgetController::GetDisplayEntryForSlot(ESFEquipmentSlot Slot, FSFEquipmentDisplayEntry& OutEntry) const
{
	for (const FSFEquipmentDisplayEntry& Entry : CurrentDisplayEntries)
	{
		if (Entry.EquipmentSlot == Slot)
		{
			OutEntry = Entry;
			return true;
		}
	}

	return false;
}

void USFEquipmentWidgetController::RebuildDisplayEntries()
{
	CurrentDisplayEntries.Reset();
	CurrentDisplayEntries.Reserve(8);

	AddEntryForSlot(ESFEquipmentSlot::Head, ESFEquipmentSlotType::Head, FText::FromString(TEXT("Head")));
	AddEntryForSlot(ESFEquipmentSlot::Chest, ESFEquipmentSlotType::Chest, FText::FromString(TEXT("Chest")));
	AddEntryForSlot(ESFEquipmentSlot::Arms, ESFEquipmentSlotType::Arms, FText::FromString(TEXT("Arms")));
	AddEntryForSlot(ESFEquipmentSlot::Legs, ESFEquipmentSlotType::Legs, FText::FromString(TEXT("Legs")));
	AddEntryForSlot(ESFEquipmentSlot::Boots, ESFEquipmentSlotType::Boots, FText::FromString(TEXT("Boots")));
	AddEntryForSlot(ESFEquipmentSlot::PrimaryWeapon, ESFEquipmentSlotType::PrimaryWeapon, FText::FromString(TEXT("Primary")));
	AddEntryForSlot(ESFEquipmentSlot::SecondaryWeapon, ESFEquipmentSlotType::SecondaryWeapon, FText::FromString(TEXT("Secondary")));
	AddEntryForSlot(ESFEquipmentSlot::HeavyWeapon, ESFEquipmentSlotType::HeavyWeapon, FText::FromString(TEXT("Heavy")));
}

void USFEquipmentWidgetController::AddEntryForSlot(
	ESFEquipmentSlot Slot,
	ESFEquipmentSlotType DisplaySlotType,
	const FText& SlotDisplayName)
{
	CurrentDisplayEntries.Add(BuildEntryForSlot(Slot, DisplaySlotType, SlotDisplayName));
}

FSFEquipmentDisplayEntry USFEquipmentWidgetController::BuildEntryForSlot(
	ESFEquipmentSlot Slot,
	ESFEquipmentSlotType DisplaySlotType,
	const FText& SlotDisplayName) const
{
	FSFEquipmentDisplayEntry Entry;
	Entry.SlotType = DisplaySlotType;
	Entry.EquipmentSlot = Slot;
	Entry.SlotDisplayName = SlotDisplayName;
	Entry.bIsWeaponSlot =
		Slot == ESFEquipmentSlot::PrimaryWeapon ||
		Slot == ESFEquipmentSlot::SecondaryWeapon ||
		Slot == ESFEquipmentSlot::HeavyWeapon;

	UE_LOG(LogTemp, Warning,
		TEXT("Equip UI: Slot=%d HasItem=%s ItemDef=%s ItemIcon=%s WeaponData=%s WeaponIcon=%s"),
		(int32)Slot,
		Entry.bHasItemEquipped ? TEXT("true") : TEXT("false"),
		*GetNameSafe(Entry.EquippedItemDefinition),
		*GetNameSafe(Entry.EquippedItemIcon),
		*GetNameSafe(Entry.EquippedWeaponData),
		Entry.EquippedWeaponData ? *GetNameSafe(Entry.EquippedWeaponData->Icon) : TEXT("None"));

	if (!EquipmentComponent)
	{
		return Entry;
	}

	const FSFEquipmentSlotEntry SlotEntry = EquipmentComponent->GetEquipmentSlotEntry(Slot);

	Entry.bHasItemEquipped = SlotEntry.bHasItemEquipped;
	Entry.EquippedItemDefinition = SlotEntry.ItemDefinition;
	Entry.EquippedEntryId = SlotEntry.InventoryEntryId;
	Entry.EquippedWeaponInstance = SlotEntry.WeaponInstance;
	Entry.bIsActive = Entry.bIsWeaponSlot && EquipmentComponent->GetCurrentWeaponInstance().InstanceId == SlotEntry.WeaponInstance.InstanceId;

	if (SlotEntry.ItemDefinition)
	{
		Entry.EquippedItemName = SlotEntry.ItemDefinition->DisplayName;
		Entry.EquippedItemIcon = SlotEntry.ItemDefinition->Icon;
	}

	if (Entry.bIsWeaponSlot && SlotEntry.WeaponInstance.IsValid())
	{
		USFWeaponData* WeaponData = SlotEntry.WeaponInstance.WeaponDefinition.IsNull()
			? nullptr
			: SlotEntry.WeaponInstance.WeaponDefinition.LoadSynchronous();

		Entry.EquippedWeaponData = WeaponData;

		if (WeaponData)
		{
			// Prefer weapon data name/icon over generic item definition
			Entry.EquippedItemName = WeaponData->DisplayName;

			if (WeaponData->Icon)
			{
				Entry.EquippedItemIcon = WeaponData->Icon;
			}
		}
	}

	return Entry;
}