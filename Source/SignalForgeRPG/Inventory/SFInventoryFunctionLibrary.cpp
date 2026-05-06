#include "Inventory/SFInventoryFunctionLibrary.h"

#include "Components/SFEquipmentComponent.h"
#include "Components/SFInventoryComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Inventory/SFItemDefinition.h"

USFInventoryComponent* USFInventoryFunctionLibrary::GetInventoryComponentFromTarget(AActor* Target)
{
	if (!Target)
	{
		return nullptr;
	}

	if (USFInventoryComponent* InventoryComp = FindInventoryComponentOnActor(Target))
	{
		return InventoryComp;
	}

	if (const APawn* OwningPawn = Cast<APawn>(Target))
	{
		if (APlayerState* PlayerState = OwningPawn->GetPlayerState())
		{
			if (USFInventoryComponent* InventoryComp = FindInventoryComponentOnActor(PlayerState))
			{
				return InventoryComp;
			}
		}

		if (AController* Controller = OwningPawn->GetController())
		{
			if (USFInventoryComponent* InventoryComp = FindInventoryComponentOnActor(Controller))
			{
				return InventoryComp;
			}
		}
	}

	if (APlayerController* OwningController = Cast<APlayerController>(Target))
	{
		if (APlayerState* PlayerState = OwningController->GetPlayerState<APlayerState>())
		{
			if (USFInventoryComponent* InventoryComp = FindInventoryComponentOnActor(PlayerState))
			{
				return InventoryComp;
			}
		}

		if (APawn* Pawn = OwningController->GetPawn())
		{
			if (USFInventoryComponent* InventoryComp = FindInventoryComponentOnActor(Pawn))
			{
				return InventoryComp;
			}
		}
	}

	if (APlayerState* PlayerState = Cast<APlayerState>(Target))
	{
		if (APawn* Pawn = PlayerState->GetPawn())
		{
			if (USFInventoryComponent* InventoryComp = FindInventoryComponentOnActor(Pawn))
			{
				return InventoryComp;
			}
		}
	}

	return nullptr;
}

USFEquipmentComponent* USFInventoryFunctionLibrary::GetEquipmentComponentFromTarget(AActor* Target)
{
	if (!Target)
	{
		return nullptr;
	}

	if (USFEquipmentComponent* EquipmentComp = FindEquipmentComponentOnActor(Target))
	{
		return EquipmentComp;
	}

	if (const APawn* OwningPawn = Cast<APawn>(Target))
	{
		if (APlayerState* PlayerState = OwningPawn->GetPlayerState())
		{
			if (USFEquipmentComponent* EquipmentComp = FindEquipmentComponentOnActor(PlayerState))
			{
				return EquipmentComp;
			}
		}

		if (AController* Controller = OwningPawn->GetController())
		{
			if (USFEquipmentComponent* EquipmentComp = FindEquipmentComponentOnActor(Controller))
			{
				return EquipmentComp;
			}
		}
	}

	if (APlayerController* OwningController = Cast<APlayerController>(Target))
	{
		if (APlayerState* PlayerState = OwningController->GetPlayerState<APlayerState>())
		{
			if (USFEquipmentComponent* EquipmentComp = FindEquipmentComponentOnActor(PlayerState))
			{
				return EquipmentComp;
			}
		}

		if (APawn* Pawn = OwningController->GetPawn())
		{
			if (USFEquipmentComponent* EquipmentComp = FindEquipmentComponentOnActor(Pawn))
			{
				return EquipmentComp;
			}
		}
	}

	if (APlayerState* PlayerState = Cast<APlayerState>(Target))
	{
		if (APawn* Pawn = PlayerState->GetPawn())
		{
			if (USFEquipmentComponent* EquipmentComp = FindEquipmentComponentOnActor(Pawn))
			{
				return EquipmentComp;
			}
		}
	}

	return nullptr;
}

TArray<FSFInventoryDisplayEntry> USFInventoryFunctionLibrary::SortDisplayEntries(
	const TArray<FSFInventoryDisplayEntry>& InEntries,
	ESFInventoryDisplaySortMode SortMode,
	bool bEquippedFirst)
{
	TArray<FSFInventoryDisplayEntry> OutEntries = InEntries;

	OutEntries.Sort([SortMode, bEquippedFirst](const FSFInventoryDisplayEntry& A, const FSFInventoryDisplayEntry& B)
		{
			return CompareDisplayEntries(A, B, SortMode, bEquippedFirst) < 0;
		});

	return OutEntries;
}

bool USFInventoryFunctionLibrary::FindDisplayEntryByGuid(
	const TArray<FSFInventoryDisplayEntry>& InEntries,
	FGuid EntryGuid,
	FSFInventoryDisplayEntry& OutEntry)
{
	for (const FSFInventoryDisplayEntry& Entry : InEntries)
	{
		if (Entry.InventoryEntryGuid == EntryGuid)
		{
			OutEntry = Entry;
			return true;
		}
	}

	return false;
}

int32 USFInventoryFunctionLibrary::GetDisplayIndexByGuid(
	const TArray<FSFInventoryDisplayEntry>& InEntries,
	FGuid EntryGuid)
{
	if (!EntryGuid.IsValid())
	{
		return INDEX_NONE;
	}

	for (int32 Index = 0; Index < InEntries.Num(); ++Index)
	{
		if (InEntries[Index].InventoryEntryGuid == EntryGuid)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

bool USFInventoryFunctionLibrary::ContainsDisplayEntryGuid(
	const TArray<FSFInventoryDisplayEntry>& InEntries,
	FGuid EntryGuid)
{
	return GetDisplayIndexByGuid(InEntries, EntryGuid) != INDEX_NONE;
}

TArray<FSFInventoryDisplayEntry> USFInventoryFunctionLibrary::FilterDisplayEntriesToEquippable(
	const TArray<FSFInventoryDisplayEntry>& InEntries)
{
	TArray<FSFInventoryDisplayEntry> OutEntries;
	OutEntries.Reserve(InEntries.Num());

	for (const FSFInventoryDisplayEntry& Entry : InEntries)
	{
		if (!Entry.ItemDefinition)
		{
			continue;
		}

		const ESFItemType ItemType = Entry.ItemDefinition->ItemType;
		if (ItemType == ESFItemType::Weapon || ItemType == ESFItemType::Armor)
		{
			OutEntries.Add(Entry);
		}
	}

	return OutEntries;
}

USFInventoryComponent* USFInventoryFunctionLibrary::FindInventoryComponentOnActor(const AActor* Target)
{
	return Target ? Target->FindComponentByClass<USFInventoryComponent>() : nullptr;
}

USFEquipmentComponent* USFInventoryFunctionLibrary::FindEquipmentComponentOnActor(const AActor* Target)
{
	return Target ? Target->FindComponentByClass<USFEquipmentComponent>() : nullptr;
}

int32 USFInventoryFunctionLibrary::CompareDisplayEntries(
	const FSFInventoryDisplayEntry& A,
	const FSFInventoryDisplayEntry& B,
	ESFInventoryDisplaySortMode SortMode,
	bool bEquippedFirst)
{
	if (bEquippedFirst && A.bIsEquipped != B.bIsEquipped)
	{
		return A.bIsEquipped ? -1 : 1;
	}

	auto CompareNamesAscending = [&A, &B]() -> int32
		{
			const FString NameA = A.DisplayName.ToString();
			const FString NameB = B.DisplayName.ToString();
			if (NameA < NameB) return -1;
			if (NameA > NameB) return 1;
			return 0;
		};

	auto CompareNamesDescending = [&A, &B]() -> int32
		{
			const FString NameA = A.DisplayName.ToString();
			const FString NameB = B.DisplayName.ToString();
			if (NameA > NameB) return -1;
			if (NameA < NameB) return 1;
			return 0;
		};

	switch (SortMode)
	{
	case ESFInventoryDisplaySortMode::NameDescending:
	{
		const int32 NameResult = CompareNamesDescending();
		if (NameResult != 0)
		{
			return NameResult;
		}
		break;
	}

	case ESFInventoryDisplaySortMode::QuantityDescending:
		if (A.Quantity != B.Quantity)
		{
			return (A.Quantity > B.Quantity) ? -1 : 1;
		}
		{
			const int32 NameResult = CompareNamesAscending();
			if (NameResult != 0)
			{
				return NameResult;
			}
		}
		break;

	case ESFInventoryDisplaySortMode::QuantityAscending:
		if (A.Quantity != B.Quantity)
		{
			return (A.Quantity < B.Quantity) ? -1 : 1;
		}
		{
			const int32 NameResult = CompareNamesAscending();
			if (NameResult != 0)
			{
				return NameResult;
			}
		}
		break;

	case ESFInventoryDisplaySortMode::Default:
	case ESFInventoryDisplaySortMode::NameAscending:
	default:
	{
		const int32 NameResult = CompareNamesAscending();
		if (NameResult != 0)
		{
			return NameResult;
		}
	}
	break;
	}

	if (A.InventoryEntryGuid.IsValid() && B.InventoryEntryGuid.IsValid())
	{
		const FString GuidA = A.InventoryEntryGuid.ToString(EGuidFormats::DigitsWithHyphensLower);
		const FString GuidB = B.InventoryEntryGuid.ToString(EGuidFormats::DigitsWithHyphensLower);
		if (GuidA < GuidB) return -1;
		if (GuidA > GuidB) return 1;
	}

	return 0;
}