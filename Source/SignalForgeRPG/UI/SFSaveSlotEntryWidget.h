#pragma once

#include "CoreMinimal.h"
#include "UI/SFUserWidgetBase.h"
#include "Save/SFPlayerSaveTypes.h"
#include "SFSaveSlotEntryWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSFOnSaveSlotRowClickedSignature, const FString&, SlotName);

/**
 * USFSaveSlotEntryWidget
 *
 * One row in the save-slot browser. Renders a single FSFPlayerSaveSlotInfo
 * (friendly name, timestamp, level, level name, schema version) and
 * exposes Load / Save-overwrite / Delete buttons.
 *
 * UMG layout authoring stays in Blueprint -- this base class only owns:
 *   - data plumbing (which slot does this row represent?)
 *   - optional BindWidget hooks for common labels and buttons
 *   - native button-click handlers that bubble up via delegates
 *
 * Create a Widget Blueprint child (e.g. WBP_SaveSlotEntry) that reparents
 * this class, lay out the visuals, and bind the same UProperty names below
 * (those marked BindWidgetOptional). The list widget will spawn rows of
 * this class and call SetSlotInfo on each.
 */
UCLASS(Blueprintable, Abstract)
class SIGNALFORGERPG_API USFSaveSlotEntryWidget : public USFUserWidgetBase
{
	GENERATED_BODY()

public:
	/** Set / refresh the displayed metadata for this row. */
	UFUNCTION(BlueprintCallable, Category = "Save|Slot")
	void SetSlotInfo(const FSFPlayerSaveSlotInfo& InInfo);

	UFUNCTION(BlueprintPure, Category = "Save|Slot")
	const FSFPlayerSaveSlotInfo& GetSlotInfo() const { return CurrentInfo; }

	UFUNCTION(BlueprintPure, Category = "Save|Slot")
	const FString& GetSlotName() const { return CurrentInfo.SlotName; }

	/** Highlight this row (driven by the parent list when selected). */
	UFUNCTION(BlueprintCallable, Category = "Save|Slot")
	void SetIsSelected(bool bInSelected);

	UFUNCTION(BlueprintPure, Category = "Save|Slot")
	bool IsSelected() const { return bIsSelected; }

	/** Fired when the user clicks anywhere on the row (selects it). */
	UPROPERTY(BlueprintAssignable, Category = "Save|Slot")
	FSFOnSaveSlotRowClickedSignature OnRowClicked;

	/** Fired when the row's "Load" button is pressed. */
	UPROPERTY(BlueprintAssignable, Category = "Save|Slot")
	FSFOnSaveSlotRowClickedSignature OnLoadClicked;

	/** Fired when the row's "Save here / Overwrite" button is pressed. */
	UPROPERTY(BlueprintAssignable, Category = "Save|Slot")
	FSFOnSaveSlotRowClickedSignature OnSaveClicked;

	/** Fired when the row's "Delete" button is pressed. */
	UPROPERTY(BlueprintAssignable, Category = "Save|Slot")
	FSFOnSaveSlotRowClickedSignature OnDeleteClicked;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Whole-row click target. Optional -- if absent, BP can route through OnRowClicked manually. */
	UPROPERTY(BlueprintReadOnly, Category = "Save|Slot", meta = (BindWidgetOptional))
	TObjectPtr<UButton> RowButton = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Save|Slot", meta = (BindWidgetOptional))
	TObjectPtr<UButton> LoadButton = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Save|Slot", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SaveButton = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Save|Slot", meta = (BindWidgetOptional))
	TObjectPtr<UButton> DeleteButton = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Save|Slot", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SlotNameText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Save|Slot", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FriendlyNameText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Save|Slot", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TimestampText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Save|Slot", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Save|Slot", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelNameText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Save|Slot", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PlaytimeText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Save|Slot", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText = nullptr;

	/** Current slot info displayed by this row. */
	UPROPERTY(BlueprintReadOnly, Category = "Save|Slot")
	FSFPlayerSaveSlotInfo CurrentInfo;

	UPROPERTY(BlueprintReadOnly, Category = "Save|Slot")
	bool bIsSelected = false;

	/** Re-pushes CurrentInfo into the bound text widgets. Override-friendly. */
	UFUNCTION(BlueprintCallable, Category = "Save|Slot")
	virtual void RefreshVisuals();

	/** BP hook: row got a new slot info. Use for custom rebinds / animations. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Save|Slot")
	void BP_OnSlotInfoSet(const FSFPlayerSaveSlotInfo& InInfo);

	/** BP hook: selection state flipped. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Save|Slot")
	void BP_OnSelectionChanged(bool bNewSelected);

private:
	UFUNCTION()
	void HandleRowButtonClicked();

	UFUNCTION()
	void HandleLoadButtonClicked();

	UFUNCTION()
	void HandleSaveButtonClicked();

	UFUNCTION()
	void HandleDeleteButtonClicked();

	/** Format MM:SS or HH:MM:SS depending on size. */
	static FText FormatPlaytime(float Seconds);
};
