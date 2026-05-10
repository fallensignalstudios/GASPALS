// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "Narrative/SFNarrativeTypes.h"
#include "Narrative/SFQuestDisplayTypes.h"
#include "SFQuestLogWidgetController.generated.h"

class USFNarrativeComponent;
class USFQuestRuntime;
class USFQuestInstance;
class USFQuestDefinition;

/**
 * Sort order for the quest log list.
 */
UENUM(BlueprintType)
enum class ESFQuestLogSortMode : uint8
{
	Default,            // Tracked first, then InProgress, then finished; stable within each bucket.
	NameAscending,
	NameDescending,
	NewestFirst,        // Most recently updated first.
	ProgressDescending  // Highest objective progress first.
};

/**
 * Filter for which quest lifecycle states the log should surface.
 */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ESFQuestLogFilterFlags : uint8
{
	None        = 0 UMETA(Hidden),
	Active      = 1 << 0,
	Succeeded   = 1 << 1,
	Failed      = 1 << 2,
	Abandoned   = 1 << 3
};
ENUM_CLASS_FLAGS(ESFQuestLogFilterFlags);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnQuestLogDisplayDataUpdatedSignature,
	const TArray<FSFQuestDisplayEntry>&, QuestEntries);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnQuestLogQuestUpdatedSignature,
	FName, QuestId,
	const FSFQuestDisplayEntry&, UpdatedEntry);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnQuestLogSelectionChangedSignature,
	FName, SelectedQuestId);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnQuestLogTrackedQuestChangedSignature,
	FName, TrackedQuestId,
	bool, bIsTracked);

/**
 * Controller for the player quest log widget. Mirrors the inventory widget
 * controller pattern: it listens to USFNarrativeComponent quest delegates,
 * builds an array of FSFQuestDisplayEntry rows, and broadcasts to the widget
 * whenever something changes.
 *
 * The controller is intentionally headless / Slate-free so it can be unit
 * tested and reused by the menu, the HUD tracker, and any pause-screen UI.
 */
UCLASS(BlueprintType, Blueprintable)
class SIGNALFORGERPG_API USFQuestLogWidgetController : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI")
	void Initialize(USFNarrativeComponent* InNarrativeComponent);

	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI")
	void Deinitialize();

	/** Force a full rebuild from runtime state. Cheap; safe to call repeatedly. */
	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI")
	void RefreshQuestLogDisplayData();

	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI")
	void SetSortMode(ESFQuestLogSortMode InSortMode);

	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI")
	void CycleSortMode();

	/** Bitmask filter; default shows Active. Pass 0 to show everything. */
	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI", meta = (Bitmask, BitmaskEnum = "/Script/SignalForgeRPG.ESFQuestLogFilterFlags"))
	void SetFilterFlags(int32 InFilterFlags);

	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI")
	bool SelectQuest(FName QuestId);

	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI")
	void ClearSelection();

	/** Toggle this quest as the tracked (pinned) one. Pass NAME_None to untrack. */
	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI")
	void SetTrackedQuest(FName QuestId);

	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI")
	void ToggleTrackedQuest(FName QuestId);

	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI")
	bool AbandonSelectedQuest();

	UFUNCTION(BlueprintPure, Category = "Narrative|Quest|UI")
	const TArray<FSFQuestDisplayEntry>& GetCurrentDisplayEntries() const
	{
		return CurrentDisplayEntries;
	}

	UFUNCTION(BlueprintPure, Category = "Narrative|Quest|UI")
	bool TryGetEntryById(FName QuestId, FSFQuestDisplayEntry& OutEntry) const;

	UFUNCTION(BlueprintPure, Category = "Narrative|Quest|UI")
	FName GetSelectedQuestId() const { return SelectedQuestId; }

	UFUNCTION(BlueprintPure, Category = "Narrative|Quest|UI")
	FName GetTrackedQuestId() const { return TrackedQuestId; }

	UFUNCTION(BlueprintPure, Category = "Narrative|Quest|UI")
	ESFQuestLogSortMode GetSortMode() const { return SortMode; }

	UFUNCTION(BlueprintPure, Category = "Narrative|Quest|UI", meta = (Bitmask, BitmaskEnum = "/Script/SignalForgeRPG.ESFQuestLogFilterFlags"))
	int32 GetFilterFlags() const { return FilterFlags; }

	UFUNCTION(BlueprintPure, Category = "Narrative|Quest|UI")
	USFNarrativeComponent* GetNarrativeComponent() const { return NarrativeComponent; }

	/** Fires whenever the full list is rebuilt (sort, filter, quest added/removed). */
	UPROPERTY(BlueprintAssignable, Category = "Narrative|Quest|UI")
	FOnQuestLogDisplayDataUpdatedSignature OnQuestLogDisplayDataUpdated;

	/** Fires when a single quest's row updates (state change, task progress) but the list shape didn't. */
	UPROPERTY(BlueprintAssignable, Category = "Narrative|Quest|UI")
	FOnQuestLogQuestUpdatedSignature OnQuestUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Narrative|Quest|UI")
	FOnQuestLogSelectionChangedSignature OnSelectionChanged;

	UPROPERTY(BlueprintAssignable, Category = "Narrative|Quest|UI")
	FOnQuestLogTrackedQuestChangedSignature OnTrackedQuestChanged;

protected:
	virtual void BeginDestroy() override;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	TObjectPtr<USFNarrativeComponent> NarrativeComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	TArray<FSFQuestDisplayEntry> CurrentDisplayEntries;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	FName SelectedQuestId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	FName TrackedQuestId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	ESFQuestLogSortMode SortMode = ESFQuestLogSortMode::Default;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (Bitmask, BitmaskEnum = "/Script/SignalForgeRPG.ESFQuestLogFilterFlags"))
	int32 FilterFlags = static_cast<int32>(ESFQuestLogFilterFlags::Active);

	// --- Narrative component delegate handlers ---

	UFUNCTION()
	void HandleQuestStarted(USFQuestInstance* QuestInstance);

	UFUNCTION()
	void HandleQuestRestarted(USFQuestInstance* QuestInstance);

	UFUNCTION()
	void HandleQuestAbandoned(USFQuestInstance* QuestInstance);

	UFUNCTION()
	void HandleQuestStateChanged(USFQuestInstance* QuestInstance, FName StateId);

	UFUNCTION()
	void HandleQuestTaskProgressed(USFQuestInstance* QuestInstance, FGameplayTag TaskTag, FName ContextId, int32 Quantity);

	void BindToSources();
	void UnbindFromSources();

	/** Full rebuild of CurrentDisplayEntries. */
	void RebuildDisplayEntries();

	/** Build a single FSFQuestDisplayEntry from a runtime instance. */
	bool BuildEntryForInstance(USFQuestInstance* Instance, FSFQuestDisplayEntry& OutEntry) const;

	/** Patch (or insert) a single quest's entry in CurrentDisplayEntries and broadcast OnQuestUpdated. */
	void RefreshSingleQuestEntry(USFQuestInstance* Instance);

	/** Sort/filter helpers. */
	void ApplySorting();
	bool PassesFilter(const FSFQuestDisplayEntry& Entry) const;
	int32 GetCompletionStateBucket(ESFQuestCompletionState State) const;
};
