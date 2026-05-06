#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Inventory/SFInventoryDisplayTypes.h"
#include "Combat/SFWeaponInstanceTypes.h"
#include "SFInventoryWidgetController.generated.h"

class USFInventoryComponent;
class USFEquipmentComponent;
class USFWeaponData;
class USFItemDefinition;
struct FSFInventoryEntry;

UENUM(BlueprintType)
enum class ESFInventoryDisplaySortMode : uint8
{
	Default,
	NameAscending,
	NameDescending,
	QuantityDescending,
	QuantityAscending
};

UENUM(BlueprintType)
enum class ESFInventoryWidgetOpResult : uint8
{
	Success,
	InvalidIndex,
	InvalidDisplayEntry,
	MissingInventoryComponent,
	MissingEquipmentComponent,
	ItemNotFound,
	EquipFailed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryDisplayDataUpdatedSignature, const TArray<FSFInventoryDisplayEntry>&, InventoryEntries);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventorySelectionChangedSignature, int32, SelectedIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryEquipRequestedSignature, ESFInventoryWidgetOpResult, Result);

UCLASS(BlueprintType, Blueprintable)
class SIGNALFORGERPG_API USFInventoryWidgetController : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void Initialize(USFInventoryComponent* InInventoryComponent, USFEquipmentComponent* InEquipmentComponent);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void Deinitialize();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RefreshInventoryDisplayData();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	ESFInventoryWidgetOpResult EquipItemAtIndex(int32 DisplayIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	ESFInventoryWidgetOpResult EquipSelectedItem();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SelectIndex(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearSelection();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetSortMode(ESFInventoryDisplaySortMode InSortMode);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetShowOnlyEquippable(bool bInShowOnlyEquippable);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FSFInventoryDisplayEntry>& GetCurrentDisplayEntries() const
	{
		return CurrentDisplayEntries;
	}

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetSelectedIndex() const
	{
		return SelectedIndex;
	}

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasValidSelection() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	ESFInventoryDisplaySortMode GetSortMode() const
	{
		return SortMode;
	}

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryDisplayDataUpdatedSignature OnInventoryDisplayDataUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventorySelectionChangedSignature OnInventorySelectionChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryEquipRequestedSignature OnInventoryEquipRequested;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool GetShowOnlyEquippable() const
	{
		return bShowOnlyEquippable;
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void CycleSortMode();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ToggleEquippableFilter();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetHideEquippedEntries(bool bInHideEquippedEntries);

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	bool bHideEquippedEntries = true;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool GetHideEquippedEntries() const
	{
		return bHideEquippedEntries;
	}

	UFUNCTION(BlueprintPure, Category = "Inventory")
	USFInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetMaxInventorySlots() const;

protected:
	virtual void BeginDestroy() override;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<USFInventoryComponent> InventoryComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<USFEquipmentComponent> EquipmentComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<FSFInventoryDisplayEntry> CurrentDisplayEntries;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 SelectedIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FGuid SelectedEntryGuid;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	ESFInventoryDisplaySortMode SortMode = ESFInventoryDisplaySortMode::Default;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	bool bShowOnlyEquippable = false;

	UFUNCTION()
	void HandleInventoryUpdated();

	UFUNCTION()
	void HandleEquippedWeaponChanged(USFWeaponData* NewWeaponData, FSFWeaponInstanceData NewWeaponInstance);

	void BindToSources();
	void UnbindFromSources();
	void RebuildDisplayEntries();
	void ApplySorting();
	void ValidateSelection();

	bool PassesDisplayFilters(const USFItemDefinition* ItemDefinition) const;
	bool IsEntryEquipped(const FSFInventoryEntry& Entry) const;
	bool TryGetInventoryEntryByGuid(const FGuid& EntryGuid, FSFInventoryEntry& OutEntry) const;
	ESFInventoryWidgetOpResult EquipDisplayEntry(const FSFInventoryDisplayEntry& DisplayEntry);


};