#include "Components/SFInventoryComponent.h"
#include "Inventory/SFItemDefinition.h"
#include "Components/SFEquipmentComponent.h"

#define LOCTEXT_NAMESPACE "SFInventory"

DEFINE_LOG_CATEGORY_STATIC(LogSFInventory, Log, All);

USFInventoryComponent::USFInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USFInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyStartingItems();
}

void USFInventoryComponent::ApplyStartingItems()
{
	if (StartingItems.IsEmpty())
	{
		return;
	}

	InventoryEntries.Reserve(FMath::Min(MaxInventorySlots, StartingItems.Num()));

	for (FSFInventoryEntry Entry : StartingItems)
	{
		if (!Entry.ItemDefinition || Entry.Quantity <= 0)
		{
			continue;
		}

		Entry.EnsureEntryId();
		Entry.Normalize();

		if (!Entry.IsValid())
		{
			UE_LOG(LogSFInventory, Warning, TEXT("Invalid starting inventory entry, skipping."));
			continue;
		}

		if (Entry.IsWeaponEntry() && Entry.HasWeaponInstanceData())
		{
			AddItemInstance(Entry.ItemDefinition, Entry.WeaponInstanceData);
		}
		else
		{
			AddItem(Entry.ItemDefinition, Entry.Quantity);
		}
	}

	// NEW: auto-equip armor/weapons from starting items
	if (USFEquipmentComponent* EquipmentComponent = GetOwnerEquipmentComponent())
	{
		for (const FSFInventoryEntry& Entry : InventoryEntries)
		{
			if (!Entry.ItemDefinition)
			{
				continue;
			}

			// Only auto-equip weapons/armor; adjust if you add more types later
			const bool bIsWeapon = Entry.ItemDefinition->IsWeapon();
			const bool bIsArmor = (Entry.ItemDefinition->ItemType == ESFItemType::Armor);

			if (!bIsWeapon && !bIsArmor)
			{
				continue;
			}

			if (!Entry.IsValid())
			{
				continue;
			}

			// Equip by entry id so EquipmentComponent can track the linkage
			const ESFEquipmentOpResult EquipResult =
				EquipmentComponent->EquipFromInventoryEntry(Entry.EntryId);

			UE_LOG(LogSFInventory, Verbose,
				TEXT("Auto-equip starting item %s (EntryId=%s) → Result=%d"),
				*GetNameSafe(Entry.ItemDefinition),
				*Entry.EntryId.ToString(),
				(int32)EquipResult);
		}
	}
}

FSFInventoryAddResult USFInventoryComponent::AddItem(USFItemDefinition* ItemDefinition, int32 Quantity)
{
	if (!ItemDefinition)
	{
		return FSFInventoryAddResult::AddedNone(
			nullptr,
			Quantity,
			LOCTEXT("Add_InvalidItem", "Invalid item."),
			ESFInventoryOpResult::InvalidItem);
	}

	if (Quantity <= 0)
	{
		return FSFInventoryAddResult::AddedNone(
			ItemDefinition,
			Quantity,
			LOCTEXT("Add_InvalidQuantity", "Quantity must be greater than zero."),
			ESFInventoryOpResult::InvalidQuantity);
	}

	if (!CanAcceptItem(ItemDefinition))
	{
		return FSFInventoryAddResult::AddedNone(
			ItemDefinition,
			Quantity,
			LOCTEXT("Add_BlockedByRules", "This item cannot be added because of inventory rules."),
			ESFInventoryOpResult::BlockedByRules);
	}

	FSFInventoryAddResult Result = TryAddItem_Internal(ItemDefinition, Quantity);

	if (Result.AddedQuantity > 0)
	{
		OnItemAdded.Broadcast(Result);

		if (bAutoSortOnChange)
		{
			SortInventory();
		}
		else
		{
			BroadcastInventoryUpdated();
		}
	}

	return Result;
}

FSFInventoryAddResult USFInventoryComponent::AddItemInstance(
	USFItemDefinition* ItemDefinition,
	const FSFWeaponInstanceData& WeaponInstance)
{
	if (!ItemDefinition)
	{
		return FSFInventoryAddResult::AddedNone(
			nullptr,
			1,
			LOCTEXT("AddInstance_InvalidItem", "Invalid item."),
			ESFInventoryOpResult::InvalidItem);
	}

	if (!WeaponInstance.IsValid() || !ItemDefinition->IsWeapon())
	{
		return FSFInventoryAddResult::AddedNone(
			ItemDefinition,
			1,
			LOCTEXT("AddInstance_InvalidInstance", "Invalid weapon instance data."),
			ESFInventoryOpResult::InvalidItem);
	}

	if (!CanAcceptItem(ItemDefinition))
	{
		return FSFInventoryAddResult::AddedNone(
			ItemDefinition,
			1,
			LOCTEXT("AddInstance_Blocked",
				"This item cannot be added because of inventory rules."),
			ESFInventoryOpResult::BlockedByRules);
	}

	if (IsFull())
	{
		return FSFInventoryAddResult::AddedNone(
			ItemDefinition,
			1,
			LOCTEXT("AddInstance_Full",
				"You do not have any inventory slots left for this item."),
			ESFInventoryOpResult::InventoryFull);
	}

	FSFInventoryEntry NewEntry(ItemDefinition, WeaponInstance);
	NewEntry.EnsureEntryId();
	NewEntry.Normalize();

	if (!NewEntry.IsValid())
	{
		return FSFInventoryAddResult::AddedNone(
			ItemDefinition,
			1,
			LOCTEXT("AddInstance_InvalidNormalized", "Weapon instance failed validation."),
			ESFInventoryOpResult::InvalidItem);
	}

	const int32 NewIndex = InventoryEntries.Add(NewEntry);
	OnInventoryEntryAdded.Broadcast(InventoryEntries[NewIndex], NewIndex, 1);

	TArray<int32> AffectedSlots;
	AffectedSlots.Add(NewIndex);
	FSFInventoryAddResult Result =
		FSFInventoryAddResult::AddedAll(ItemDefinition, 1, AffectedSlots);
	OnItemAdded.Broadcast(Result);

	if (bAutoSortOnChange)
	{
		SortInventory();
	}
	else
	{
		BroadcastInventoryUpdated();
	}

	return Result;
}

FSFInventoryAddResult USFInventoryComponent::TryAddItem_Internal(
	USFItemDefinition* ItemDefinition,
	int32 Quantity)
{
	FText ErrorText;
	const int32 SpaceForItem = GetSpaceForItem(ItemDefinition, ErrorText);
	const int32 QuantityToActuallyAdd = FMath::Min(Quantity, SpaceForItem);

	if (QuantityToActuallyAdd <= 0)
	{
		const ESFInventoryOpResult FailureResult =
			IsFull() ? ESFInventoryOpResult::InventoryFull : ESFInventoryOpResult::NoCapacity;

		return FSFInventoryAddResult::AddedNone(ItemDefinition, Quantity, ErrorText, FailureResult);
	}

	int32 RemainingToAdd = QuantityToActuallyAdd;
	TArray<int32> AffectedSlots;
	AffectedSlots.Reserve(QuantityToActuallyAdd);

	RemainingToAdd -= AddQuantityToExistingStacks(ItemDefinition, RemainingToAdd, AffectedSlots);

	if (RemainingToAdd > 0)
	{
		RemainingToAdd -= CreateNewStacksForQuantity(ItemDefinition, RemainingToAdd, AffectedSlots);
	}

	const int32 AddedQuantity = QuantityToActuallyAdd - RemainingToAdd;

	if (AddedQuantity <= 0)
	{
		const ESFInventoryOpResult FailureResult =
			IsFull() ? ESFInventoryOpResult::InventoryFull : ESFInventoryOpResult::NoCapacity;

		return FSFInventoryAddResult::AddedNone(ItemDefinition, Quantity, ErrorText, FailureResult);
	}

	if (AddedQuantity < Quantity && !bAllowPartialAdds)
	{
		RemoveItem(ItemDefinition, AddedQuantity);

		return FSFInventoryAddResult::AddedNone(
			ItemDefinition,
			Quantity,
			ErrorText.IsEmpty()
			? LOCTEXT("Add_NoPartialAdds", "Not enough space to add the full quantity.")
			: ErrorText,
			ESFInventoryOpResult::NoCapacity);
	}

	if (AddedQuantity == Quantity)
	{
		return FSFInventoryAddResult::AddedAll(ItemDefinition, Quantity, AffectedSlots);
	}

	return FSFInventoryAddResult::AddedSome(
		ItemDefinition,
		Quantity,
		AddedQuantity,
		AffectedSlots,
		ErrorText.IsEmpty()
		? LOCTEXT("Add_Partial", "Only part of the requested quantity could be added.")
		: ErrorText);
}

int32 USFInventoryComponent::AddQuantityToExistingStacks(
	USFItemDefinition* ItemDefinition,
	int32 QuantityToAdd,
	TArray<int32>& OutAffectedSlots)
{
	if (!ItemDefinition || QuantityToAdd <= 0 || !CanStackItem(ItemDefinition))
	{
		return 0;
	}

	int32 TotalAdded = 0;
	const int32 MaxStackSize = GetItemMaxStackSize(ItemDefinition);

	for (int32 Index = 0; Index < InventoryEntries.Num() && QuantityToAdd > 0; ++Index)
	{
		FSFInventoryEntry& Entry = InventoryEntries[Index];
		if (Entry.ItemDefinition != ItemDefinition)
		{
			continue;
		}

		if (!Entry.IsStackableEntry())
		{
			continue;
		}

		const int32 StackSpace = FMath::Max(0, MaxStackSize - Entry.Quantity);
		if (StackSpace <= 0)
		{
			continue;
		}

		const int32 AddedToStack = FMath::Min(QuantityToAdd, StackSpace);
		Entry.Quantity += AddedToStack;
		Entry.Normalize();

		QuantityToAdd -= AddedToStack;
		TotalAdded += AddedToStack;
		OutAffectedSlots.AddUnique(Index);

		OnInventoryEntryChanged.Broadcast(Entry, Index, AddedToStack);
	}

	return TotalAdded;
}

int32 USFInventoryComponent::CreateNewStacksForQuantity(
	USFItemDefinition* ItemDefinition,
	int32 QuantityToAdd,
	TArray<int32>& OutAffectedSlots)
{
	if (!ItemDefinition || QuantityToAdd <= 0)
	{
		return 0;
	}

	int32 TotalAdded = 0;
	const bool bStackable = CanStackItem(ItemDefinition);
	const int32 MaxStackSize = GetItemMaxStackSize(ItemDefinition);

	while (QuantityToAdd > 0 && !IsFull())
	{
		const int32 NewStackQuantity = bStackable
			? FMath::Min(QuantityToAdd, MaxStackSize)
			: 1;

		FSFInventoryEntry NewEntry(ItemDefinition, NewStackQuantity);
		NewEntry.EnsureEntryId();
		NewEntry.Normalize();

		if (!NewEntry.IsValid())
		{
			UE_LOG(LogSFInventory, Warning, TEXT("New inventory entry failed validation, skipping."));
			break;
		}

		const int32 NewIndex = InventoryEntries.Add(NewEntry);
		OutAffectedSlots.Add(NewIndex);
		OnInventoryEntryAdded.Broadcast(InventoryEntries[NewIndex], NewIndex, NewStackQuantity);

		QuantityToAdd -= NewStackQuantity;
		TotalAdded += NewStackQuantity;
	}

	return TotalAdded;
}

FSFInventoryRemoveResult USFInventoryComponent::RemoveItem(
	USFItemDefinition* ItemDefinition,
	int32 Quantity)
{
	FSFInventoryRemoveResult Result;
	Result.ItemDefinition = ItemDefinition;
	Result.RequestedQuantity = Quantity;
	Result.RemainingRequestedQuantity = Quantity;

	if (!ItemDefinition)
	{
		Result.Result = ESFInventoryOpResult::InvalidItem;
		Result.ErrorText = LOCTEXT("Remove_InvalidItem", "Invalid item.");
		return Result;
	}

	if (Quantity <= 0)
	{
		Result.Result = ESFInventoryOpResult::InvalidQuantity;
		Result.ErrorText = LOCTEXT("Remove_InvalidQuantity", "Quantity must be greater than zero.");
		return Result;
	}

	if (!HasItem(ItemDefinition, Quantity))
	{
		Result.Result = ESFInventoryOpResult::InsufficientQuantity;
		Result.ErrorText = LOCTEXT("Remove_Insufficient", "Not enough quantity in the inventory.");
		return Result;
	}

	int32 RemainingToRemove = Quantity;

	while (RemainingToRemove > 0)
	{
		const int32 RemoveIndex = FindBestRemovalEntryIndex(ItemDefinition);
		if (!InventoryEntries.IsValidIndex(RemoveIndex))
		{
			break;
		}

		FSFInventoryEntry& Entry = InventoryEntries[RemoveIndex];
		const int32 RemovedFromEntry = FMath::Min(RemainingToRemove, Entry.Quantity);

		Entry.Quantity -= RemovedFromEntry;
		Entry.Normalize();

		RemainingToRemove -= RemovedFromEntry;
		Result.RemovedQuantity += RemovedFromEntry;
		Result.AffectedSlots.AddUnique(RemoveIndex);

		if (Entry.Quantity > 0)
		{
			OnInventoryEntryChanged.Broadcast(Entry, RemoveIndex, -RemovedFromEntry);
		}
		else
		{
			const FSFInventoryEntry RemovedEntry = Entry;

			// Unequip this specific entry if it was equipped.
			UnequipEntryIfEquipped(RemovedEntry.EntryId);
			SetEntryEquipped(RemovedEntry.EntryId, false);

			InventoryEntries.RemoveAt(RemoveIndex);
			OnInventoryEntryRemoved.Broadcast(RemovedEntry, RemoveIndex);
		}
	}

	CompactInventory();

	Result.RemainingRequestedQuantity = RemainingToRemove;
	Result.Result = RemainingToRemove == 0
		? ESFInventoryOpResult::Success
		: ESFInventoryOpResult::InsufficientQuantity;

	if (Result.RemovedQuantity > 0)
	{
		OnItemRemoved.Broadcast(Result);

		if (bAutoSortOnChange)
		{
			SortInventory();
		}
		else
		{
			BroadcastInventoryUpdated();
		}
	}

	return Result;
}

bool USFInventoryComponent::HasItem(USFItemDefinition* ItemDefinition, int32 Quantity) const
{
	return Quantity > 0 && GetItemCount(ItemDefinition) >= Quantity;
}

bool USFInventoryComponent::GetInventoryEntryAtIndex(int32 Index, FSFInventoryEntry& OutEntry) const
{
	if (!InventoryEntries.IsValidIndex(Index))
	{
		return false;
	}

	OutEntry = InventoryEntries[Index];
	return true;
}

USFItemDefinition* USFInventoryComponent::GetItemDefinitionAtIndex(int32 Index) const
{
	return InventoryEntries.IsValidIndex(Index)
		? InventoryEntries[Index].ItemDefinition
		: nullptr;
}

int32 USFInventoryComponent::GetItemCount(USFItemDefinition* ItemDefinition) const
{
	if (!ItemDefinition)
	{
		return 0;
	}

	int32 TotalQuantity = 0;
	for (const FSFInventoryEntry& Entry : InventoryEntries)
	{
		if (Entry.ItemDefinition == ItemDefinition)
		{
			TotalQuantity += Entry.Quantity;
		}
	}

	return TotalQuantity;
}

bool USFInventoryComponent::IsFull() const
{
	return InventoryEntries.Num() >= MaxInventorySlots;
}

int32 USFInventoryComponent::GetRemainingSlotCapacity() const
{
	return FMath::Max(0, MaxInventorySlots - InventoryEntries.Num());
}

int32 USFInventoryComponent::GetRemainingCapacityForItem(USFItemDefinition* ItemDefinition) const
{
	FText DummyErrorText;
	return GetSpaceForItem(ItemDefinition, DummyErrorText);
}

int32 USFInventoryComponent::GetSpaceForItem(
	USFItemDefinition* ItemDefinition,
	FText& OutErrorText) const
{
	OutErrorText = FText::GetEmpty();

	if (!ItemDefinition)
	{
		OutErrorText = LOCTEXT("Space_InvalidItem", "Invalid item.");
		return 0;
	}

	int32 RuleLimitedSpace = MAX_int32;
	if (ItemDefinition->MaxOwnedQuantity > 0)
	{
		const int32 CurrentCount = GetItemCount(ItemDefinition);
		RuleLimitedSpace = FMath::Max(0, ItemDefinition->MaxOwnedQuantity - CurrentCount);

		if (RuleLimitedSpace <= 0)
		{
			OutErrorText = LOCTEXT("Space_MaxOwned",
				"You already own the maximum quantity of this item.");
			return 0;
		}
	}

	if (!CanStackItem(ItemDefinition))
	{
		const int32 SlotSpace = GetRemainingSlotCapacity();
		if (SlotSpace <= 0)
		{
			OutErrorText = LOCTEXT("Space_NoSlots_Unique",
				"You do not have any inventory slots left for this item.");
			return 0;
		}

		return FMath::Min(SlotSpace, RuleLimitedSpace);
	}

	const int32 MaxStackSize = GetItemMaxStackSize(ItemDefinition);
	int32 CapacitySpace = GetRemainingSlotCapacity() * MaxStackSize;

	for (const FSFInventoryEntry& Entry : InventoryEntries)
	{
		if (Entry.ItemDefinition == ItemDefinition && Entry.IsStackableEntry())
		{
			CapacitySpace += FMath::Max(0, MaxStackSize - Entry.Quantity);
		}
	}

	if (CapacitySpace <= 0)
	{
		OutErrorText = LOCTEXT("Space_NoSlots_Stackable",
			"You do not have enough space to carry this item.");
		return 0;
	}

	return FMath::Min(CapacitySpace, RuleLimitedSpace);
}

bool USFInventoryComponent::MoveEntry(int32 FromIndex, int32 ToIndex)
{
	if (!InventoryEntries.IsValidIndex(FromIndex) ||
		!InventoryEntries.IsValidIndex(ToIndex) ||
		FromIndex == ToIndex)
	{
		return false;
	}

	InventoryEntries.Swap(FromIndex, ToIndex);
	BroadcastInventoryUpdated();
	return true;
}

bool USFInventoryComponent::SplitStack(int32 SourceIndex, int32 SplitQuantity)
{
	if (!InventoryEntries.IsValidIndex(SourceIndex) || SplitQuantity <= 0)
	{
		return false;
	}

	FSFInventoryEntry& SourceEntry = InventoryEntries[SourceIndex];
	if (!SourceEntry.IsStackableEntry() || SourceEntry.Quantity <= SplitQuantity || IsFull())
	{
		return false;
	}

	SourceEntry.Quantity -= SplitQuantity;
	SourceEntry.Normalize();
	OnInventoryEntryChanged.Broadcast(SourceEntry, SourceIndex, -SplitQuantity);

	FSFInventoryEntry NewEntry(SourceEntry.ItemDefinition, SplitQuantity);
	NewEntry.EnsureEntryId();
	NewEntry.ItemLevel = SourceEntry.ItemLevel;
	NewEntry.InstanceTags = SourceEntry.InstanceTags;
	NewEntry.CurrentDurability = SourceEntry.CurrentDurability;
	NewEntry.bFavorite = SourceEntry.bFavorite;
	NewEntry.bEquipped = false;
	NewEntry.Normalize();

	const int32 NewIndex = InventoryEntries.Add(NewEntry);
	OnInventoryEntryAdded.Broadcast(InventoryEntries[NewIndex], NewIndex, SplitQuantity);

	BroadcastInventoryUpdated();
	return true;
}

bool USFInventoryComponent::SortInventory()
{
	InventoryEntries.Sort([](const FSFInventoryEntry& A, const FSFInventoryEntry& B)
		{
			const FString AName = GetNameSafe(A.ItemDefinition);
			const FString BName = GetNameSafe(B.ItemDefinition);

			if (AName == BName)
			{
				return A.Quantity > B.Quantity;
			}

			return AName < BName;
		});

	BroadcastInventoryUpdated();
	return true;
}

void USFInventoryComponent::ClearInventory()
{
	if (InventoryEntries.IsEmpty())
	{
		return;
	}

	InventoryEntries.Reset();
	OnInventoryCleared.Broadcast();
	BroadcastInventoryUpdated();
}

void USFInventoryComponent::BroadcastInventoryUpdated()
{
	OnInventoryUpdated.Broadcast();
}

bool USFInventoryComponent::CanAcceptItem(USFItemDefinition* ItemDefinition) const
{
	if (!ItemDefinition)
	{
		return false;
	}

	if (ItemDefinition->MaxOwnedQuantity > 0 &&
		GetItemCount(ItemDefinition) >= ItemDefinition->MaxOwnedQuantity)
	{
		return false;
	}

	return true;
}

int32 USFInventoryComponent::FindBestRemovalEntryIndex(USFItemDefinition* ItemDefinition) const
{
	int32 BestIndex = INDEX_NONE;
	int32 BestQuantity = MAX_int32;

	for (int32 Index = 0; Index < InventoryEntries.Num(); ++Index)
	{
		const FSFInventoryEntry& Entry = InventoryEntries[Index];

		if (Entry.ItemDefinition == ItemDefinition && Entry.Quantity < BestQuantity)
		{
			BestIndex = Index;
			BestQuantity = Entry.Quantity;
		}
	}

	return BestIndex;
}

int32 USFInventoryComponent::GetItemMaxStackSize(const USFItemDefinition* ItemDefinition) const
{
	return ItemDefinition ? ItemDefinition->GetEffectiveMaxStackSize() : 1;
}

bool USFInventoryComponent::CanStackItem(const USFItemDefinition* ItemDefinition) const
{
	return ItemDefinition && ItemDefinition->CanStack();
}

void USFInventoryComponent::CompactInventory()
{
	InventoryEntries.RemoveAll([](const FSFInventoryEntry& Entry)
		{
			return Entry.ItemDefinition == nullptr || Entry.Quantity <= 0 || !Entry.IsValid();
		});
}

bool USFInventoryComponent::GetEntryById(const FGuid& EntryId, FSFInventoryEntry& OutEntry) const
{
	for (const FSFInventoryEntry& Entry : InventoryEntries)
	{
		if (Entry.EntryId == EntryId)
		{
			OutEntry = Entry;
			return true;
		}
	}
	return false;
}

int32 USFInventoryComponent::FindEntryIndexById(const FGuid& EntryId) const
{
	for (int32 Index = 0; Index < InventoryEntries.Num(); ++Index)
	{
		if (InventoryEntries[Index].EntryId == EntryId)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

bool USFInventoryComponent::SetEntryFavorite(const FGuid& EntryId, bool bNewFavorite)
{
	const int32 Index = FindEntryIndexById(EntryId);
	if (!InventoryEntries.IsValidIndex(Index))
	{
		return false;
	}

	FSFInventoryEntry& Entry = InventoryEntries[Index];
	if (Entry.bFavorite == bNewFavorite)
	{
		return false;
	}

	Entry.bFavorite = bNewFavorite;
	OnInventoryEntryChanged.Broadcast(Entry, Index, 0);
	BroadcastInventoryUpdated();
	return true;
}

bool USFInventoryComponent::SetEntryEquipped(const FGuid& EntryId, bool bNewEquipped)
{
	const int32 Index = FindEntryIndexById(EntryId);
	if (!InventoryEntries.IsValidIndex(Index))
	{
		return false;
	}

	FSFInventoryEntry& Entry = InventoryEntries[Index];
	if (Entry.bEquipped == bNewEquipped)
	{
		return false;
	}

	Entry.bEquipped = bNewEquipped;
	OnInventoryEntryChanged.Broadcast(Entry, Index, 0);
	BroadcastInventoryUpdated();
	return true;
}

bool USFInventoryComponent::DamageEntryDurability(const FGuid& EntryId, int32 Amount)
{
	if (Amount <= 0)
	{
		return false;
	}

	const int32 Index = FindEntryIndexById(EntryId);
	if (!InventoryEntries.IsValidIndex(Index))
	{
		return false;
	}

	FSFInventoryEntry& Entry = InventoryEntries[Index];
	if (!Entry.HasDurability())
	{
		return false;
	}

	const int32 OldDurability = Entry.CurrentDurability;
	Entry.CurrentDurability = FMath::Max(0, Entry.CurrentDurability - Amount);
	Entry.Normalize();

	if (Entry.CurrentDurability == OldDurability)
	{
		return false;
	}

	OnInventoryEntryChanged.Broadcast(Entry, Index, 0);
	BroadcastInventoryUpdated();
	return true;
}

USFEquipmentComponent* USFInventoryComponent::GetOwnerEquipmentComponent() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<USFEquipmentComponent>() : nullptr;
}

bool USFInventoryComponent::UnequipEntryIfEquipped(const FGuid& EntryId)
{
	if (!EntryId.IsValid())
	{
		return false;
	}

	if (USFEquipmentComponent* EquipmentComponent = GetOwnerEquipmentComponent())
	{
		return EquipmentComponent->UnequipInventoryEntry(EntryId);
	}

	return false;
}

bool USFInventoryComponent::RemoveEntryById(const FGuid& EntryId)
{
	if (!EntryId.IsValid())
	{
		return false;
	}

	// Ensure equipment is in sync before deleting the entry.
	UnequipEntryIfEquipped(EntryId);
	SetEntryEquipped(EntryId, false);

	const int32 Index = FindEntryIndexById(EntryId);
	if (!InventoryEntries.IsValidIndex(Index))
	{
		return false;
	}

	const FSFInventoryEntry RemovedEntry = InventoryEntries[Index];
	InventoryEntries.RemoveAt(Index);
	OnInventoryEntryRemoved.Broadcast(RemovedEntry, Index);

	CompactInventory();
	BroadcastInventoryUpdated();
	return true;
}

bool USFInventoryComponent::DropEntryById(const FGuid& EntryId)
{
	if (!EntryId.IsValid())
	{
		return false;
	}

	UnequipEntryIfEquipped(EntryId);
	SetEntryEquipped(EntryId, false);

	FSFInventoryEntry Entry;
	if (!GetEntryById(EntryId, Entry))
	{
		return false;
	}

	// TODO: spawn pickup actor using Entry info here

	return RemoveEntryById(EntryId);
}

#undef LOCTEXT_NAMESPACE