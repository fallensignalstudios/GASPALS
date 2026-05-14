// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
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

	/**
	 * If we don't have a narrative component yet, walk owning-player
	 * → PlayerState → NarrativeComponent and try to attach. Returns true if
	 * the controller is now wired to a non-null narrative component.
	 */
	bool TryAutoResolveNarrativeComponent();

	/**
	 * If auto-resolve fails on NativeConstruct (e.g. PlayerState hasn't
	 * replicated yet, or the player just travelled into the level), keep
	 * retrying on a low-frequency timer so the panel populates as soon as the
	 * narrative component becomes available. The timer self-cancels on first
	 * success or after MaxAutoResolveRetrySeconds, whichever comes first.
	 */
	UFUNCTION()
	void TickAutoResolveRetry();

	void StartAutoResolveRetryTimer();
	void StopAutoResolveRetryTimer();

	/** Period between retry attempts. Short enough to feel snappy, long enough not to spam logs. */
	UPROPERTY(EditDefaultsOnly, Category = "Narrative|Quest|UI", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float AutoResolveRetryPeriodSeconds = 0.5f;

	/** Total time we'll keep retrying before giving up. */
	UPROPERTY(EditDefaultsOnly, Category = "Narrative|Quest|UI", meta = (ClampMin = "0.5", ClampMax = "60.0"))
	float MaxAutoResolveRetrySeconds = 10.0f;

	FTimerHandle AutoResolveRetryHandle;
	float AutoResolveRetryElapsedSeconds = 0.0f;

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
