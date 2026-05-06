#include "Inventory/SFInventoryWidgetController.h"

#include "Components/SFEquipmentComponent.h"
#include "Components/SFInventoryComponent.h"
#include "Combat/SFWeaponData.h"
#include "Inventory/SFInventoryTypes.h"
#include "Inventory/SFItemDefinition.h"
#include "Inventory/SFInventoryFunctionLibrary.h"

void USFInventoryWidgetController::Initialize(
	USFInventoryComponent* InInventoryComponent,
	USFEquipmentComponent* InEquipmentComponent
)
{
	if (InventoryComponent == InInventoryComponent && EquipmentComponent == InEquipmentComponent)
	{
		RefreshInventoryDisplayData();
		return;
	}

	UnbindFromSources();

	InventoryComponent = InInventoryComponent;
	EquipmentComponent = InEquipmentComponent;

	BindToSources();
	RefreshInventoryDisplayData();
}

void USFInventoryWidgetController::Deinitialize()
{
	UnbindFromSources();
	InventoryComponent = nullptr;
	EquipmentComponent = nullptr;
	CurrentDisplayEntries.Reset();
	SelectedIndex = INDEX_NONE;
	SelectedEntryGuid.Invalidate();
}

void USFInventoryWidgetController::BeginDestroy()
{
	UnbindFromSources();
	Super::BeginDestroy();
}

void USFInventoryWidgetController::BindToSources()
{
	if (InventoryComponent && !InventoryComponent->OnInventoryUpdated.IsAlreadyBound(this, &USFInventoryWidgetController::HandleInventoryUpdated))
	{
		InventoryComponent->OnInventoryUpdated.AddDynamic(this, &USFInventoryWidgetController::HandleInventoryUpdated);
	}

	if (EquipmentComponent && !EquipmentComponent->OnEquippedWeaponChanged.IsAlreadyBound(this, &USFInventoryWidgetController::HandleEquippedWeaponChanged))
	{
		EquipmentComponent->OnEquippedWeaponChanged.AddDynamic(this, &USFInventoryWidgetController::HandleEquippedWeaponChanged);
	}
	if (EquipmentComponent &&
		!EquipmentComponent->OnEquipmentUpdated.IsAlreadyBound(this, &USFInventoryWidgetController::HandleInventoryUpdated))
	{
		EquipmentComponent->OnEquipmentUpdated.AddDynamic(this, &USFInventoryWidgetController::HandleInventoryUpdated);
	}
}

void USFInventoryWidgetController::UnbindFromSources()
{
	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryUpdated.RemoveDynamic(this, &USFInventoryWidgetController::HandleInventoryUpdated);
	}

	if (EquipmentComponent)
	{
		EquipmentComponent->OnEquippedWeaponChanged.RemoveDynamic(this, &USFInventoryWidgetController::HandleEquippedWeaponChanged);
	}
	if (EquipmentComponent)
	{
		EquipmentComponent->OnEquipmentUpdated.RemoveDynamic(this, &USFInventoryWidgetController::HandleInventoryUpdated);
		EquipmentComponent->OnEquippedWeaponChanged.RemoveDynamic(this, &USFInventoryWidgetController::HandleEquippedWeaponChanged);
	}
}

void USFInventoryWidgetController::RefreshInventoryDisplayData()
{
	RebuildDisplayEntries();
	ValidateSelection();
	OnInventoryDisplayDataUpdated.Broadcast(CurrentDisplayEntries);
}

ESFInventoryWidgetOpResult USFInventoryWidgetController::EquipItemAtIndex(int32 DisplayIndex)
{
	if (!CurrentDisplayEntries.IsValidIndex(DisplayIndex))
	{
		OnInventoryEquipRequested.Broadcast(ESFInventoryWidgetOpResult::InvalidIndex);
		return ESFInventoryWidgetOpResult::InvalidIndex;
	}

	const ESFInventoryWidgetOpResult Result = EquipDisplayEntry(CurrentDisplayEntries[DisplayIndex]);
	OnInventoryEquipRequested.Broadcast(Result);
	return Result;
}

ESFInventoryWidgetOpResult USFInventoryWidgetController::EquipSelectedItem()
{
	if (!HasValidSelection())
	{
		OnInventoryEquipRequested.Broadcast(ESFInventoryWidgetOpResult::InvalidIndex);
		return ESFInventoryWidgetOpResult::InvalidIndex;
	}

	const ESFInventoryWidgetOpResult Result = EquipDisplayEntry(CurrentDisplayEntries[SelectedIndex]);
	OnInventoryEquipRequested.Broadcast(Result);
	return Result;
}

ESFInventoryWidgetOpResult USFInventoryWidgetController::EquipDisplayEntry(const FSFInventoryDisplayEntry& DisplayEntry)
{
	if (!InventoryComponent)
	{
		return ESFInventoryWidgetOpResult::MissingInventoryComponent;
	}

	if (!EquipmentComponent)
	{
		return ESFInventoryWidgetOpResult::MissingEquipmentComponent;
	}

	if (!DisplayEntry.IsValid())
	{
		return ESFInventoryWidgetOpResult::InvalidDisplayEntry;
	}

	FSFInventoryEntry Entry;
	if (!TryGetInventoryEntryByGuid(DisplayEntry.InventoryEntryGuid, Entry))
	{
		return ESFInventoryWidgetOpResult::InvalidDisplayEntry;
	}

	if (Entry.ItemDefinition != DisplayEntry.ItemDefinition || !Entry.ItemDefinition)
	{
		return ESFInventoryWidgetOpResult::ItemNotFound;
	}

	ESFEquipmentOpResult EquipResult = ESFEquipmentOpResult::EquipFailed;

	if (Entry.ItemDefinition->IsWeapon())
	{
		if (!Entry.WeaponInstanceData.IsValid())
		{
			return ESFInventoryWidgetOpResult::ItemNotFound;
		}

		const ESFEquipmentSlot PreferredSlot =
			EquipmentComponent->GetPreferredSlotForItem(Entry.ItemDefinition);
		EquipResult = EquipmentComponent->EquipItemInstanceToSlot(
			Entry.ItemDefinition,
			Entry.WeaponInstanceData,
			PreferredSlot);
	}
	else
	{
		EquipResult = EquipmentComponent->EquipItemDefinition(Entry.ItemDefinition);
	}

	switch (EquipResult)
	{
	case ESFEquipmentOpResult::Success:
	case ESFEquipmentOpResult::AlreadyEquipped:
		return ESFInventoryWidgetOpResult::Success;

	case ESFEquipmentOpResult::InvalidItem:
	case ESFEquipmentOpResult::UnsupportedItemType:
	case ESFEquipmentOpResult::MissingWeaponData:
		return ESFInventoryWidgetOpResult::ItemNotFound;

	case ESFEquipmentOpResult::InvalidSlot:
	case ESFEquipmentOpResult::EquipFailed:
	default:
		return ESFInventoryWidgetOpResult::EquipFailed;
	}
}

bool USFInventoryWidgetController::SelectIndex(int32 Index)
{
	if (!CurrentDisplayEntries.IsValidIndex(Index))
	{
		return false;
	}

	SelectedIndex = Index;
	SelectedEntryGuid = CurrentDisplayEntries[Index].InventoryEntryGuid;
	OnInventorySelectionChanged.Broadcast(SelectedIndex);
	return true;
}

void USFInventoryWidgetController::ClearSelection()
{
	if (SelectedIndex == INDEX_NONE && !SelectedEntryGuid.IsValid())
	{
		return;
	}

	SelectedIndex = INDEX_NONE;
	SelectedEntryGuid.Invalidate();
	OnInventorySelectionChanged.Broadcast(SelectedIndex);
}

void USFInventoryWidgetController::SetSortMode(ESFInventoryDisplaySortMode InSortMode)
{
	if (SortMode == InSortMode)
	{
		return;
	}

	SortMode = InSortMode;
	RefreshInventoryDisplayData();
}

void USFInventoryWidgetController::SetShowOnlyEquippable(bool bInShowOnlyEquippable)
{
	if (bShowOnlyEquippable == bInShowOnlyEquippable)
	{
		return;
	}

	bShowOnlyEquippable = bInShowOnlyEquippable;
	RefreshInventoryDisplayData();
}

bool USFInventoryWidgetController::HasValidSelection() const
{
	return SelectedEntryGuid.IsValid()
		&& CurrentDisplayEntries.IsValidIndex(SelectedIndex)
		&& CurrentDisplayEntries[SelectedIndex].InventoryEntryGuid == SelectedEntryGuid;
}

void USFInventoryWidgetController::HandleInventoryUpdated()
{
	RefreshInventoryDisplayData();
}

void USFInventoryWidgetController::HandleEquippedWeaponChanged(USFWeaponData* NewWeaponData, FSFWeaponInstanceData NewWeaponInstance)
{
	RefreshInventoryDisplayData();
}

void USFInventoryWidgetController::RebuildDisplayEntries()
{
	CurrentDisplayEntries.Reset();

	if (!InventoryComponent)
	{
		return;
	}

	const TArray<FSFInventoryEntry>& InventoryEntries = InventoryComponent->GetInventoryEntries();
	CurrentDisplayEntries.Reserve(InventoryEntries.Num());

	for (int32 SourceIndex = 0; SourceIndex < InventoryEntries.Num(); ++SourceIndex)
	{
		const FSFInventoryEntry& Entry = InventoryEntries[SourceIndex];
		if (!Entry.ItemDefinition || !PassesDisplayFilters(Entry.ItemDefinition))
		{
			continue;
		}

		const bool bIsEquipped = IsEntryEquipped(Entry);

		if (bHideEquippedEntries && bIsEquipped)
		{
			continue;
		}

		FSFInventoryDisplayEntry DisplayEntry(
			Entry.ItemDefinition,
			Entry.ItemDefinition->DisplayName,
			Entry.Quantity,
			bIsEquipped,
			Entry.IsStackableEntry(),
			Entry.ItemDefinition->ItemId,
			Entry.EntryId,
			SourceIndex,
			Entry.ItemDefinition->Icon
		);

		CurrentDisplayEntries.Add(DisplayEntry);
	}

	ApplySorting();
}

void USFInventoryWidgetController::ApplySorting()
{
	// Let the function library handle the sort logic, including equipped-first if desired.
	const bool bEquippedFirst = true;
	CurrentDisplayEntries = USFInventoryFunctionLibrary::SortDisplayEntries(
		CurrentDisplayEntries,
		SortMode,
		bEquippedFirst);
}

void USFInventoryWidgetController::ValidateSelection()
{
	if (!SelectedEntryGuid.IsValid())
	{
		if (!CurrentDisplayEntries.IsValidIndex(SelectedIndex))
		{
			SelectedIndex = INDEX_NONE;
		}
		return;
	}

	const int32 NewSelectedIndex = USFInventoryFunctionLibrary::GetDisplayIndexByGuid(
		CurrentDisplayEntries,
		SelectedEntryGuid);

	if (NewSelectedIndex == INDEX_NONE)
	{
		SelectedIndex = INDEX_NONE;
		SelectedEntryGuid.Invalidate();
		OnInventorySelectionChanged.Broadcast(SelectedIndex);
		return;
	}

	if (SelectedIndex != NewSelectedIndex)
	{
		SelectedIndex = NewSelectedIndex;
		OnInventorySelectionChanged.Broadcast(SelectedIndex);
	}
}

bool USFInventoryWidgetController::PassesDisplayFilters(const USFItemDefinition* ItemDefinition) const
{
	if (!ItemDefinition)
	{
		return false;
	}

	if (!bShowOnlyEquippable)
	{
		return true;
	}

	return ItemDefinition->ItemType == ESFItemType::Weapon
		|| ItemDefinition->ItemType == ESFItemType::Armor;
}

bool USFInventoryWidgetController::IsEntryEquipped(const FSFInventoryEntry& Entry) const
{
	if (!EquipmentComponent || !Entry.ItemDefinition)
	{
		return false;
	}

	if (Entry.ItemDefinition->IsWeapon())
	{
		const FSFWeaponInstanceData EquippedInstance = EquipmentComponent->GetCurrentWeaponInstance();
		return EquippedInstance.IsValid()
			&& Entry.WeaponInstanceData.IsValid()
			&& EquippedInstance.InstanceId == Entry.WeaponInstanceData.InstanceId;
	}

	const TMap<ESFEquipmentSlot, FSFEquipmentSlotEntry>& EquippedSlots = EquipmentComponent->GetEquippedSlots();
	for (const TPair<ESFEquipmentSlot, FSFEquipmentSlotEntry>& Pair : EquippedSlots)
	{
		if (Pair.Value.ItemDefinition == Entry.ItemDefinition)
		{
			return true;
		}
	}

	return false;
}

bool USFInventoryWidgetController::TryGetInventoryEntryByGuid(const FGuid& EntryGuid, FSFInventoryEntry& OutEntry) const
{
	if (!InventoryComponent || !EntryGuid.IsValid())
	{
		return false;
	}

	const TArray<FSFInventoryEntry>& InventoryEntries = InventoryComponent->GetInventoryEntries();
	for (const FSFInventoryEntry& Entry : InventoryEntries)
	{
		if (Entry.EntryId == EntryGuid)
		{
			OutEntry = Entry;
			return true;
		}
	}

	return false;
}

void USFInventoryWidgetController::CycleSortMode()
{
	ESFInventoryDisplaySortMode NewMode = SortMode;

	switch (SortMode)
	{
	case ESFInventoryDisplaySortMode::Default:
		NewMode = ESFInventoryDisplaySortMode::NameAscending;
		break;
	case ESFInventoryDisplaySortMode::NameAscending:
		NewMode = ESFInventoryDisplaySortMode::NameDescending;
		break;
	case ESFInventoryDisplaySortMode::NameDescending:
		NewMode = ESFInventoryDisplaySortMode::QuantityDescending;
		break;
	case ESFInventoryDisplaySortMode::QuantityDescending:
		NewMode = ESFInventoryDisplaySortMode::QuantityAscending;
		break;
	case ESFInventoryDisplaySortMode::QuantityAscending:
	default:
		NewMode = ESFInventoryDisplaySortMode::Default;
		break;
	}

	SetSortMode(NewMode);
}

void USFInventoryWidgetController::ToggleEquippableFilter()
{
	SetShowOnlyEquippable(!bShowOnlyEquippable);
}

void USFInventoryWidgetController::SetHideEquippedEntries(bool bInHideEquippedEntries)
{
	if (bHideEquippedEntries == bInHideEquippedEntries)
	{
		return;
	}

	bHideEquippedEntries = bInHideEquippedEntries;
	RefreshInventoryDisplayData();
}

int32 USFInventoryWidgetController::GetMaxInventorySlots() const
{
	return InventoryComponent ? InventoryComponent->GetMaxInventorySlots() : 0;
}