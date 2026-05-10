// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/SFUserWidgetBase.h"
#include "Narrative/SFQuestDisplayTypes.h"
#include "Narrative/SFQuestLogWidgetController.h"
#include "SFPlayerQuestLogWidget.generated.h"

class UButton;
class UTextBlock;
class UScrollBox;
class UPanelWidget;
class USFQuestEntryWidget;
class USFNarrativeComponent;

/**
 * Player-menu-side quest log. Owns a USFQuestLogWidgetController, listens to
 * its broadcasts, and renders one USFQuestEntryWidget row per quest. Modeled
 * after USFPlayerInventoryWidget so designers see a familiar shape.
 */
UCLASS(Blueprintable)
class SIGNALFORGERPG_API USFPlayerQuestLogWidget : public USFUserWidgetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI")
	void InitializeQuestLogWidget(USFNarrativeComponent* InNarrativeComponent);

	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI")
	void DeinitializeQuestLogWidget();

	UFUNCTION(BlueprintPure, Category = "Narrative|Quest|UI")
	USFQuestLogWidgetController* GetWidgetController() const { return WidgetController; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// --- BindWidgetOptional UMG bindings ---

	/** Container holding the per-quest rows. ScrollBox preferred; any UPanelWidget works. */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> EntryContainer = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EmptyStateText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedQuestNameText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SelectedQuestDescriptionText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SortButton = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SortButtonText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UButton> FilterButton = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FilterButtonText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UButton> AbandonButton = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Quest|UI")
	TSubclassOf<USFQuestEntryWidget> QuestEntryWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	TObjectPtr<USFQuestLogWidgetController> WidgetController = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	TArray<TObjectPtr<USFQuestEntryWidget>> SpawnedEntryWidgets;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	FName SelectedQuestId = NAME_None;

	// --- Controller delegate handlers ---

	UFUNCTION()
	void HandleQuestLogDisplayDataUpdated(const TArray<FSFQuestDisplayEntry>& UpdatedEntries);

	UFUNCTION()
	void HandleQuestUpdated(FName QuestId, const FSFQuestDisplayEntry& UpdatedEntry);

	UFUNCTION()
	void HandleSelectionChanged(FName NewSelectedQuestId);

	UFUNCTION()
	void HandleTrackedQuestChanged(FName TrackedQuestId, bool bIsTracked);

	// --- Per-row delegate handlers ---

	UFUNCTION()
	void HandleEntryClicked(FName QuestId);

	UFUNCTION()
	void HandleEntryTrackToggled(FName QuestId);

	UFUNCTION()
	void HandleEntryAbandonRequested(FName QuestId);

	// --- Toolbar buttons ---

	UFUNCTION()
	void HandleSortButtonClicked();

	UFUNCTION()
	void HandleFilterButtonClicked();

	UFUNCTION()
	void HandleAbandonButtonClicked();

	// --- Internal ---

	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI")
	virtual void RebuildEntryWidgets();

	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI")
	virtual void RefreshSelectedDetails();

	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI")
	virtual void UpdateButtonLabels();

	void ClearEntryWidgets();
	void BindToController();
	void UnbindFromController();

	const FSFQuestDisplayEntry* GetSelectedEntry() const;
	USFQuestEntryWidget* FindEntryWidgetById(FName QuestId) const;
	FText BuildSortLabelText() const;
	FText BuildFilterLabelText() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Narrative|Quest|UI")
	void BP_OnQuestEntriesRebuilt();

	UFUNCTION(BlueprintImplementableEvent, Category = "Narrative|Quest|UI")
	void BP_OnSelectionChanged(const FSFQuestDisplayEntry& SelectedEntry, bool bHasSelection);

	UFUNCTION(BlueprintImplementableEvent, Category = "Narrative|Quest|UI")
	void BP_OnEmptyStateChanged(bool bIsEmpty);
};
