// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/SFUserWidgetBase.h"
#include "Narrative/SFQuestDisplayTypes.h"
#include "SFQuestEntryWidget.generated.h"

class UButton;
class UTextBlock;
class UProgressBar;
class UVerticalBox;
class UBorder;
class USFQuestObjectiveEntryWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestEntryClickedSignature,        FName, QuestId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestEntryTrackToggledSignature,   FName, QuestId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestEntryAbandonRequestedSignature, FName, QuestId);

/**
 * Per-quest tile in the quest log. Renders title, status, progress bar,
 * description, and an expandable list of objective rows.
 *
 * Selection / expansion state is driven by the owning USFPlayerQuestLogWidget,
 * exactly like USFInventoryEntryWidget is driven by USFPlayerInventoryWidget.
 */
UCLASS(Blueprintable)
class SIGNALFORGERPG_API USFQuestEntryWidget : public USFUserWidgetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI")
	void SetQuestEntry(const FSFQuestDisplayEntry& InEntry);

	UFUNCTION(BlueprintPure, Category = "Narrative|Quest|UI")
	const FSFQuestDisplayEntry& GetQuestEntry() const { return CurrentEntry; }

	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI")
	void SetIsSelected(bool bInIsSelected);

	UFUNCTION(BlueprintPure, Category = "Narrative|Quest|UI")
	bool GetIsSelected() const { return bIsSelected; }

	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI")
	void SetIsExpanded(bool bInIsExpanded);

	UFUNCTION(BlueprintPure, Category = "Narrative|Quest|UI")
	bool GetIsExpanded() const { return bIsExpanded; }

	UPROPERTY(BlueprintAssignable, Category = "Narrative|Quest|UI")
	FOnQuestEntryClickedSignature OnQuestEntryClicked;

	UPROPERTY(BlueprintAssignable, Category = "Narrative|Quest|UI")
	FOnQuestEntryTrackToggledSignature OnQuestEntryTrackToggled;

	UPROPERTY(BlueprintAssignable, Category = "Narrative|Quest|UI")
	FOnQuestEntryAbandonRequestedSignature OnQuestEntryAbandonRequested;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// --- BindWidgetOptional UMG bindings ---

	/** Whole row; clicking selects/expands. */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UButton> EntryButton = nullptr;

	/** Pin/track toggle button. */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UButton> TrackButton = nullptr;

	/** Optional abandon button. */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UButton> AbandonButton = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> RootBorder = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> SelectionBorder = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuestNameText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CurrentStateText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DescriptionText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ProgressBar = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TrackButtonText = nullptr;

	/** Container into which objective row widgets are spawned. */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> ObjectivesContainer = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Quest|UI")
	TSubclassOf<USFQuestObjectiveEntryWidget> ObjectiveEntryWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	FSFQuestDisplayEntry CurrentEntry;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	bool bIsSelected = false;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	bool bIsExpanded = false;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	TArray<TObjectPtr<USFQuestObjectiveEntryWidget>> SpawnedObjectiveWidgets;

	UFUNCTION()
	void HandleEntryButtonClicked();

	UFUNCTION()
	void HandleTrackButtonClicked();

	UFUNCTION()
	void HandleAbandonButtonClicked();

	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI")
	virtual void RefreshHeaderVisuals();

	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI")
	virtual void RefreshSelectionVisuals();

	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI")
	virtual void RefreshObjectiveWidgets();

	void ClearObjectiveWidgets();

	UFUNCTION(BlueprintImplementableEvent, Category = "Narrative|Quest|UI")
	void BP_OnQuestEntrySet(const FSFQuestDisplayEntry& InEntry);

	UFUNCTION(BlueprintImplementableEvent, Category = "Narrative|Quest|UI")
	void BP_OnSelectionChanged(bool bNewIsSelected);

	UFUNCTION(BlueprintImplementableEvent, Category = "Narrative|Quest|UI")
	void BP_OnExpansionChanged(bool bNewIsExpanded);

	FText BuildStatusText() const;
};
