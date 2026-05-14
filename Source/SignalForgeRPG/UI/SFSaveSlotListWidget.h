#pragma once

#include "CoreMinimal.h"
#include "UI/SFUserWidgetBase.h"
#include "Save/SFPlayerSaveTypes.h"
#include "SFSaveSlotListWidget.generated.h"

class UButton;
class UEditableTextBox;
class UPanelWidget;
class UTextBlock;
class USFPlayerSaveService;
class USFSaveSlotEntryWidget;

UENUM(BlueprintType)
enum class ESFSaveSlotListSortMode : uint8
{
	NewestFirst,
	OldestFirst,
	NameAscending
};

/**
 * USFSaveSlotListWidget
 *
 * The slot-browser widget players interact with to pick / overwrite /
 * delete save slots. Responsibilities:
 *
 *   - Pull the slot list from USFPlayerSaveService (auto-refreshes on
 *     OnSlotListChanged so saving from anywhere updates the UI).
 *   - Spawn one USFSaveSlotEntryWidget per slot into RowContainer.
 *   - Forward row clicks into selection / load / save / delete calls
 *     on the service.
 *   - Provide an optional "New Save..." input for the player to create
 *     a slot with a custom name.
 *
 * UMG layout authoring stays in Blueprint. Required setup in the WBP
 * subclass:
 *   1. Reparent your slot-browser WBP to this class.
 *   2. Add a panel widget (UScrollBox / UVerticalBox) named RowContainer.
 *   3. Set RowWidgetClass on the class defaults to your WBP_SaveSlotEntry.
 *   4. (Optional) Add NewSlotNameInput / NewSaveButton / RefreshButton /
 *      CloseButton / EmptyStateText / StatusText. All optional.
 *
 * The widget self-refreshes on Construct, so just adding it to viewport
 * is enough to render the current slot list.
 */
UCLASS(Blueprintable)
class SIGNALFORGERPG_API USFSaveSlotListWidget : public USFUserWidgetBase
{
	GENERATED_BODY()

public:
	/** Force a re-pull from the save service and rebuild the row list. */
	UFUNCTION(BlueprintCallable, Category = "Save|UI")
	void RefreshSlotList();

	/** Programmatic load (same path that a row's Load button uses). */
	UFUNCTION(BlueprintCallable, Category = "Save|UI")
	bool LoadSlot(const FString& SlotName);

	/** Programmatic save. Empty SlotName uses the currently selected row. */
	UFUNCTION(BlueprintCallable, Category = "Save|UI")
	bool SaveSlot(const FString& SlotName);

	/** Programmatic delete. */
	UFUNCTION(BlueprintCallable, Category = "Save|UI")
	bool DeleteSlot(const FString& SlotName);

	/** Saves to whatever the New Save text box currently contains. */
	UFUNCTION(BlueprintCallable, Category = "Save|UI")
	bool SaveToNewSlotFromInput();

	UFUNCTION(BlueprintPure, Category = "Save|UI")
	const FString& GetSelectedSlotName() const { return SelectedSlotName; }

	UFUNCTION(BlueprintCallable, Category = "Save|UI")
	void SetSelectedSlotName(const FString& InSlotName);

	UFUNCTION(BlueprintCallable, Category = "Save|UI")
	void SetSortMode(ESFSaveSlotListSortMode InMode);

	UFUNCTION(BlueprintPure, Category = "Save|UI")
	ESFSaveSlotListSortMode GetSortMode() const { return SortMode; }

	/** Resolves the service from the owning game instance. Cached after first call. */
	UFUNCTION(BlueprintPure, Category = "Save|UI")
	USFPlayerSaveService* GetSaveService() const;

	/** Fired when the user picks a row (selection changed). */
	UPROPERTY(BlueprintAssignable, Category = "Save|UI")
	FSFOnPlayerSaveSlotEvent OnSelectionChanged;

	/** Fired after a slot is successfully loaded -- HUD/menu can close itself. */
	UPROPERTY(BlueprintAssignable, Category = "Save|UI")
	FSFOnPlayerSaveSlotEvent OnSlotLoaded;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Container the rows are spawned into. UScrollBox or UVerticalBox both work. */
	UPROPERTY(BlueprintReadOnly, Category = "Save|UI", meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> RowContainer = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Save|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EmptyStateText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Save|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Save|UI", meta = (BindWidgetOptional))
	TObjectPtr<UButton> RefreshButton = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Save|UI", meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Save|UI", meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> NewSlotNameInput = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Save|UI", meta = (BindWidgetOptional))
	TObjectPtr<UButton> NewSaveButton = nullptr;

	/** WBP child of USFSaveSlotEntryWidget. Required for the list to render rows. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Save|UI")
	TSubclassOf<USFSaveSlotEntryWidget> RowWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Save|UI")
	int32 UserIndex = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Save|UI")
	ESFSaveSlotListSortMode SortMode = ESFSaveSlotListSortMode::NewestFirst;

	UPROPERTY(BlueprintReadOnly, Category = "Save|UI")
	FString SelectedSlotName;

	UPROPERTY(BlueprintReadOnly, Category = "Save|UI")
	TArray<TObjectPtr<USFSaveSlotEntryWidget>> SpawnedRows;

	// Service-side event handlers
	UFUNCTION()
	void HandleServiceSlotListChanged(const FString& SlotName);

	UFUNCTION()
	void HandleServiceAfterSave(const FString& SlotName, bool bSuccess);

	UFUNCTION()
	void HandleServiceAfterLoad(const FString& SlotName, bool bSuccess);

	// Row-side event handlers
	UFUNCTION()
	void HandleRowClicked(const FString& SlotName);

	UFUNCTION()
	void HandleRowLoadClicked(const FString& SlotName);

	UFUNCTION()
	void HandleRowSaveClicked(const FString& SlotName);

	UFUNCTION()
	void HandleRowDeleteClicked(const FString& SlotName);

	// Top-level button handlers
	UFUNCTION()
	void HandleRefreshButtonClicked();

	UFUNCTION()
	void HandleCloseButtonClicked();

	UFUNCTION()
	void HandleNewSaveButtonClicked();

	/** BP hook: list rebuilt. Use for fade-in / scroll-to-top etc. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Save|UI")
	void BP_OnSlotsRebuilt(int32 SlotCount);

	/** BP hook: selection changed. Use to update detail panels etc. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Save|UI")
	void BP_OnSelectionChanged(const FString& NewSelectedSlot);

	/** BP hook: a save attempt completed (success or failure). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Save|UI")
	void BP_OnSaveResult(const FString& SlotName, bool bSuccess);

	/** BP hook: a load attempt completed. Close the menu on success here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Save|UI")
	void BP_OnLoadResult(const FString& SlotName, bool bSuccess);

	/** BP hook: the close button was pressed. Default impl removes from parent. */
	UFUNCTION(BlueprintNativeEvent, Category = "Save|UI")
	void BP_OnCloseRequested();
	virtual void BP_OnCloseRequested_Implementation();

private:
	void BindToService();
	void UnbindFromService();
	void RebuildRows(const TArray<FSFPlayerSaveSlotInfo>& Infos);
	void ApplySort(TArray<FSFPlayerSaveSlotInfo>& InOut) const;
	void UpdateEmptyState(int32 SlotCount);
	void UpdateStatus(const FText& Message);

	UPROPERTY()
	TObjectPtr<USFPlayerSaveService> CachedService = nullptr;
};
