#pragma once

#include "CoreMinimal.h"
#include "UI/SFUserWidgetBase.h"
#include "Inventory/SFInventoryWidgetController.h"
#include "Inventory/SFInventoryDisplayTypes.h"
#include "SFPlayerInventoryWidget.generated.h"

class UButton;
class UTextBlock;
class UUniformGridPanel;
class UUniformGridSlot;
class USFInventoryEntryWidget;
class USFInventoryComponent;
class USFEquipmentComponent;

UCLASS(Blueprintable)
class SIGNALFORGERPG_API USFPlayerInventoryWidget : public USFUserWidgetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InitializeInventoryWidget(USFInventoryComponent* InInventoryComponent, USFEquipmentComponent* InEquipmentComponent);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void DeinitializeInventoryWidget();

	UFUNCTION(BlueprintPure, Category = "Inventory")
	USFInventoryWidgetController* GetWidgetController() const
	{
		return WidgetController;
	}

	UFUNCTION()
	void HandleEntryWidgetEquipRequested(FGuid EntryGuid);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory", meta = (BindWidgetOptional))
	TObjectPtr<UUniformGridPanel> EntryContainer = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedItemNameText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedItemMetaText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedItemDescriptionText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EmptyStateText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EquipButtonText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SortButtonText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FilterButtonText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory", meta = (BindWidgetOptional))
	TObjectPtr<UButton> EquipButton = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SortButton = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory", meta = (BindWidgetOptional))
	TObjectPtr<UButton> FilterButton = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TSubclassOf<USFInventoryEntryWidget> InventoryEntryWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Grid", meta = (ClampMin = "1"))
	int32 GridColumns = 6;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Grid")
	FMargin GridSlotPadding = FMargin(4.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<USFInventoryWidgetController> WidgetController = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TArray<TObjectPtr<USFInventoryEntryWidget>> SpawnedEntryWidgets;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FGuid SelectedEntryId;

	UFUNCTION()
	void HandleInventoryDisplayDataUpdated(const TArray<FSFInventoryDisplayEntry>& UpdatedEntries);

	UFUNCTION()
	void HandleInventorySelectionChanged(int32 NewSelectedIndex);

	UFUNCTION()
	void HandleEquipRequested(ESFInventoryWidgetOpResult Result);

	UFUNCTION()
	void HandleEquipButtonClicked();

	UFUNCTION()
	void HandleSortButtonClicked();

	UFUNCTION()
	void HandleFilterButtonClicked();

	UFUNCTION()
	void HandleEntryWidgetClicked(FGuid EntryGuid);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	virtual void BuildWidgetLayout();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	virtual void RefreshSelectedDetails();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	virtual void RefreshActionStates();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	virtual void UpdateButtonLabels();

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void BP_OnInventoryEntriesRebuilt();

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void BP_OnSelectionChanged(const FSFInventoryDisplayEntry& SelectedEntry, bool bHasSelection);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void BP_OnEquipRequestCompleted(ESFInventoryWidgetOpResult Result);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void BP_OnEmptyStateChanged(bool bIsEmpty);

	const FSFInventoryDisplayEntry* GetSelectedDisplayEntry() const;
	void RebuildEntryWidgets();
	void ClearEntryWidgets();
	void BindToController();
	void UnbindFromController();

	FText BuildSelectedMetaText(const FSFInventoryDisplayEntry* SelectedEntry) const;
	FText BuildSelectedDescriptionText(const FSFInventoryDisplayEntry* SelectedEntry) const;
	FText BuildSortLabelText() const;
	FText BuildFilterLabelText() const;
};