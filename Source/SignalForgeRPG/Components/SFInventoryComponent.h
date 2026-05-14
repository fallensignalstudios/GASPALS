#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/SFInventoryTypes.h"
#include "SFInventoryComponent.generated.h"

class USFItemDefinition;
class USFConsumableItemDefinition;
class USFEquipmentComponent;
class UGameplayEffect;
struct FActiveGameplayEffectHandle;

UENUM(BlueprintType)
enum class ESFInventoryOpResult : uint8
{
	Success,
	InvalidItem,
	InvalidQuantity,
	InventoryFull,
	NoCapacity,
	ItemNotFound,
	InsufficientQuantity,
	BlockedByRules
};

UENUM(BlueprintType)
enum class ESFItemUseResult : uint8
{
	Success,            // Effect applied + quantity decremented.
	EntryNotFound,      // No entry with that GUID exists in this inventory.
	NotConsumable,      // Definition is not a USFConsumableItemDefinition.
	OnCooldown,         // Within UseCooldownSeconds since the entry's last use.
	NoEffect,           // ConsumeEffect was unset on the definition.
	NoAbilitySystem,    // Owner has no AbilitySystemComponent.
	AlreadyAtFull,      // bRefuseWhenAtFull blocked the use.
	DisallowedByRules,  // Future-proof: rule layer rejected the use.
	EffectApplyFailed   // ASC accepted the spec but returned an invalid handle.
};

USTRUCT(BlueprintType)
struct FSFItemUseResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	ESFItemUseResult Result = ESFItemUseResult::EntryNotFound;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FGuid EntryId;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<USFItemDefinition> ItemDefinition = nullptr;

	/** Quantity decremented from the stack. 0 if the use failed. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 ConsumedQuantity = 0;

	/** Quantity left on the stack after the use, or 0 if the entry was removed. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 RemainingQuantity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FText ErrorText = FText::GetEmpty();

	bool WasSuccessful() const { return Result == ESFItemUseResult::Success; }
};

USTRUCT(BlueprintType)
struct FSFInventoryAddResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	ESFInventoryOpResult Result = ESFInventoryOpResult::InvalidItem;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<USFItemDefinition> ItemDefinition = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<int32> AffectedSlots;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 RequestedQuantity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 AddedQuantity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 RemainingQuantity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FText ErrorText = FText::GetEmpty();

	bool WasSuccessful() const { return AddedQuantity > 0; }

	static FSFInventoryAddResult AddedNone(
		USFItemDefinition* InItemDefinition,
		int32 InRequestedQuantity,
		const FText& InErrorText,
		ESFInventoryOpResult InResult = ESFInventoryOpResult::NoCapacity)
	{
		FSFInventoryAddResult AddResult;
		AddResult.Result = InResult;
		AddResult.ItemDefinition = InItemDefinition;
		AddResult.RequestedQuantity = InRequestedQuantity;
		AddResult.RemainingQuantity = InRequestedQuantity;
		AddResult.ErrorText = InErrorText;
		return AddResult;
	}

	static FSFInventoryAddResult AddedSome(
		USFItemDefinition* InItemDefinition,
		int32 InRequestedQuantity,
		int32 InAddedQuantity,
		const TArray<int32>& InAffectedSlots,
		const FText& InErrorText)
	{
		FSFInventoryAddResult AddResult;
		AddResult.Result = ESFInventoryOpResult::NoCapacity;
		AddResult.ItemDefinition = InItemDefinition;
		AddResult.RequestedQuantity = InRequestedQuantity;
		AddResult.AddedQuantity = InAddedQuantity;
		AddResult.RemainingQuantity = FMath::Max(0, InRequestedQuantity - InAddedQuantity);
		AddResult.AffectedSlots = InAffectedSlots;
		AddResult.ErrorText = InErrorText;
		return AddResult;
	}

	static FSFInventoryAddResult AddedAll(
		USFItemDefinition* InItemDefinition,
		int32 InRequestedQuantity,
		const TArray<int32>& InAffectedSlots)
	{
		FSFInventoryAddResult AddResult;
		AddResult.Result = ESFInventoryOpResult::Success;
		AddResult.ItemDefinition = InItemDefinition;
		AddResult.RequestedQuantity = InRequestedQuantity;
		AddResult.AddedQuantity = InRequestedQuantity;
		AddResult.RemainingQuantity = 0;
		AddResult.AffectedSlots = InAffectedSlots;
		return AddResult;
	}
};

USTRUCT(BlueprintType)
struct FSFInventoryRemoveResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	ESFInventoryOpResult Result = ESFInventoryOpResult::ItemNotFound;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<USFItemDefinition> ItemDefinition = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<int32> AffectedSlots;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 RequestedQuantity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 RemovedQuantity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 RemainingRequestedQuantity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FText ErrorText = FText::GetEmpty();

	bool WasSuccessful() const { return RemovedQuantity > 0; }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdatedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryItemAddedSignature, const FSFInventoryAddResult&, AddResult);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryItemRemovedSignature, const FSFInventoryRemoveResult&, RemoveResult);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnInventoryEntryAddedSignature, FSFInventoryEntry, Entry, int32, SlotIndex, int32, AddedQuantity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnInventoryEntryChangedSignature, FSFInventoryEntry, Entry, int32, SlotIndex, int32, DeltaQuantity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryEntryRemovedSignature, FSFInventoryEntry, Entry, int32, FormerSlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryClearedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryItemUsedSignature, const FSFItemUseResult&, UseResult);

UCLASS(ClassGroup = (Custom), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class SIGNALFORGERPG_API USFInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFInventoryComponent();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<FSFInventoryEntry> StartingItems;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Inventory")
	TArray<FSFInventoryEntry> InventoryEntries;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Rules", meta = (ClampMin = "1"))
	int32 MaxInventorySlots = 32;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Rules")
	bool bAllowPartialAdds = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Rules")
	bool bAutoSortOnChange = false;

public:
	// Primary API

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FSFInventoryAddResult AddItem(USFItemDefinition* ItemDefinition, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FSFInventoryAddResult AddItemInstance(USFItemDefinition* ItemDefinition, const FSFWeaponInstanceData& WeaponInstance);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FSFInventoryRemoveResult RemoveItem(USFItemDefinition* ItemDefinition, int32 Quantity = 1);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasItem(USFItemDefinition* ItemDefinition, int32 Quantity = 1) const;

	// Entry accessors

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool GetInventoryEntryAtIndex(int32 Index, FSFInventoryEntry& OutEntry) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	USFItemDefinition* GetItemDefinitionAtIndex(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetInventoryEntryCount() const { return InventoryEntries.Num(); }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FSFInventoryEntry>& GetInventoryEntries() const { return InventoryEntries; }

	// Counts / capacity

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemCount(USFItemDefinition* ItemDefinition) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsFull() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetRemainingSlotCapacity() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetRemainingCapacityForItem(USFItemDefinition* ItemDefinition) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetSpaceForItem(USFItemDefinition* ItemDefinition, FText& OutErrorText) const;

	// Manipulation

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool MoveEntry(int32 FromIndex, int32 ToIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SplitStack(int32 SourceIndex, int32 SplitQuantity);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SortInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearInventory();

	/**
	 * Replace the entire inventory with the supplied entries verbatim.
	 *
	 * Unlike AddItem, this PRESERVES per-entry runtime state (EntryId,
	 * LastUseTime, durability, item level, favorite/equipped flags,
	 * instance tags, weapon roll data) so a loaded save reconstructs the
	 * inventory bit-for-bit -- including consumable cooldowns and the
	 * equipped weapon's stable GUID.
	 *
	 * Invalid entries (null definition, broken state) are silently
	 * dropped. OnInventoryUpdated is broadcast exactly once on completion.
	 * Intended for the save/load system; not a general-purpose API.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Save")
	void SetInventoryEntriesFromSave(const TArray<FSFInventoryEntry>& InEntries);

	// New convenience APIs by EntryId (optional, but nice for UI/equipment)

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool GetEntryById(const FGuid& EntryId, FSFInventoryEntry& OutEntry) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 FindEntryIndexById(const FGuid& EntryId) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SetEntryFavorite(const FGuid& EntryId, bool bNewFavorite);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SetEntryEquipped(const FGuid& EntryId, bool bNewEquipped);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool DamageEntryDurability(const FGuid& EntryId, int32 Amount);

	// -------------------------------------------------------------------------
	// Consumable use
	// -------------------------------------------------------------------------

	/**
	 * Use the consumable entry identified by EntryId on the owning character.
	 * Validates the entry is a USFConsumableItemDefinition, respects per-entry
	 * cooldown and the bRefuseWhenAtFull check, applies ConsumeEffect through
	 * the owner's ASC, then decrements the stack by AmountConsumedPerUse.
	 *
	 * Returns a structured result so UI can show a localized failure reason
	 * ("on cooldown", "already at full", etc.) without parsing log lines.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Use")
	FSFItemUseResult UseItem(const FGuid& EntryId);

	/**
	 * Convenience helper for hotbars / quick-use buttons: finds the FIRST
	 * entry whose definition matches and uses it. Skips entries on cooldown
	 * or that fail other gates so a hotbar can immediately try the next
	 * stack instead of getting stuck on one slot.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Use")
	FSFItemUseResult UseFirstItemOfDefinition(USFItemDefinition* ItemDefinition);

	// Events

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryUpdatedSignature OnInventoryUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryItemAddedSignature OnItemAdded;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryItemRemovedSignature OnItemRemoved;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryEntryAddedSignature OnInventoryEntryAdded;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryEntryChangedSignature OnInventoryEntryChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryEntryRemovedSignature OnInventoryEntryRemoved;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryClearedSignature OnInventoryCleared;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Use")
	FOnInventoryItemUsedSignature OnItemUsed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Debug")
	TObjectPtr<USFItemDefinition> DebugItemDefinition = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool UnequipEntryIfEquipped(const FGuid& EntryId);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveEntryById(const FGuid& EntryId);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool DropEntryById(const FGuid& EntryId);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetMaxInventorySlots() const { return MaxInventorySlots; }

protected:
	void BroadcastInventoryUpdated();
	void ApplyStartingItems();
	bool CanAcceptItem(USFItemDefinition* ItemDefinition) const;
	FSFInventoryAddResult TryAddItem_Internal(USFItemDefinition* ItemDefinition, int32 Quantity);

	int32 AddQuantityToExistingStacks(USFItemDefinition* ItemDefinition, int32 QuantityToAdd, TArray<int32>& OutAffectedSlots);
	int32 CreateNewStacksForQuantity(USFItemDefinition* ItemDefinition, int32 QuantityToAdd, TArray<int32>& OutAffectedSlots);

	int32 FindBestRemovalEntryIndex(USFItemDefinition* ItemDefinition) const;
	int32 GetItemMaxStackSize(const USFItemDefinition* ItemDefinition) const;
	bool CanStackItem(const USFItemDefinition* ItemDefinition) const;
	void CompactInventory();
	USFEquipmentComponent* GetOwnerEquipmentComponent() const;
};