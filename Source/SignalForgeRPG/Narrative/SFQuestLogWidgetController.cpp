// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "Narrative/SFQuestLogWidgetController.h"

#include "Narrative/SFNarrativeComponent.h"
#include "Narrative/SFQuestDefinition.h"
#include "Narrative/SFQuestInstance.h"
#include "Narrative/SFQuestRuntime.h"

DEFINE_LOG_CATEGORY_STATIC(LogSFQuestLogController, Log, All);

#define LOCTEXT_NAMESPACE "SFQuestLogWidgetController"

namespace
{
	/**
	 * Builds the FText for an objective row. Falls back to a generated string
	 * built from the task tag and required quantity when the authoring asset
	 * doesn't carry an explicit display name.
	 */
	FText BuildObjectiveDisplayText(const FSFQuestTaskDefinition& Task, int32 CurrentProgress)
	{
		const int32 ClampedProgress = FMath::Clamp(CurrentProgress, 0, Task.RequiredQuantity);

		if (Task.RequiredQuantity > 1)
		{
			return FText::Format(
				LOCTEXT("ObjectiveQuantityFormat", "{0}  ({1}/{2})"),
				FText::FromName(Task.TaskId.IsNone() ? Task.TaskTag.GetTagName() : Task.TaskId),
				FText::AsNumber(ClampedProgress),
				FText::AsNumber(Task.RequiredQuantity));
		}

		return FText::FromName(Task.TaskId.IsNone() ? Task.TaskTag.GetTagName() : Task.TaskId);
	}

	bool IsActiveCompletionState(ESFQuestCompletionState State)
	{
		return State == ESFQuestCompletionState::InProgress
			|| State == ESFQuestCompletionState::NotStarted;
	}
}

void USFQuestLogWidgetController::Initialize(USFNarrativeComponent* InNarrativeComponent)
{
	UnbindFromSources();

	NarrativeComponent = InNarrativeComponent;

	BindToSources();
	RebuildDisplayEntries();

	UE_LOG(LogSFQuestLogController, Log,
		TEXT("[QuestController] Initialize: NarrativeComponent=%s, Runtime=%s, Entries=%d"),
		*GetNameSafe(NarrativeComponent),
		*GetNameSafe(NarrativeComponent ? NarrativeComponent->GetQuestRuntime() : nullptr),
		CurrentDisplayEntries.Num());
}

void USFQuestLogWidgetController::Deinitialize()
{
	UnbindFromSources();
	NarrativeComponent = nullptr;
	CurrentDisplayEntries.Reset();
	SelectedQuestId = NAME_None;
	TrackedQuestId = NAME_None;
	OnQuestLogDisplayDataUpdated.Broadcast(CurrentDisplayEntries);
}

void USFQuestLogWidgetController::BeginDestroy()
{
	UnbindFromSources();
	Super::BeginDestroy();
}

void USFQuestLogWidgetController::BindToSources()
{
	if (!NarrativeComponent)
	{
		return;
	}

	if (!NarrativeComponent->OnQuestStarted.IsAlreadyBound(this, &USFQuestLogWidgetController::HandleQuestStarted))
	{
		NarrativeComponent->OnQuestStarted.AddDynamic(this, &USFQuestLogWidgetController::HandleQuestStarted);
	}
	if (!NarrativeComponent->OnQuestRestarted.IsAlreadyBound(this, &USFQuestLogWidgetController::HandleQuestRestarted))
	{
		NarrativeComponent->OnQuestRestarted.AddDynamic(this, &USFQuestLogWidgetController::HandleQuestRestarted);
	}
	if (!NarrativeComponent->OnQuestAbandoned.IsAlreadyBound(this, &USFQuestLogWidgetController::HandleQuestAbandoned))
	{
		NarrativeComponent->OnQuestAbandoned.AddDynamic(this, &USFQuestLogWidgetController::HandleQuestAbandoned);
	}
	if (!NarrativeComponent->OnQuestStateChanged.IsAlreadyBound(this, &USFQuestLogWidgetController::HandleQuestStateChanged))
	{
		NarrativeComponent->OnQuestStateChanged.AddDynamic(this, &USFQuestLogWidgetController::HandleQuestStateChanged);
	}
	if (!NarrativeComponent->OnQuestTaskProgressed.IsAlreadyBound(this, &USFQuestLogWidgetController::HandleQuestTaskProgressed))
	{
		NarrativeComponent->OnQuestTaskProgressed.AddDynamic(this, &USFQuestLogWidgetController::HandleQuestTaskProgressed);
	}
}

void USFQuestLogWidgetController::UnbindFromSources()
{
	if (!NarrativeComponent)
	{
		return;
	}

	NarrativeComponent->OnQuestStarted.RemoveDynamic(this, &USFQuestLogWidgetController::HandleQuestStarted);
	NarrativeComponent->OnQuestRestarted.RemoveDynamic(this, &USFQuestLogWidgetController::HandleQuestRestarted);
	NarrativeComponent->OnQuestAbandoned.RemoveDynamic(this, &USFQuestLogWidgetController::HandleQuestAbandoned);
	NarrativeComponent->OnQuestStateChanged.RemoveDynamic(this, &USFQuestLogWidgetController::HandleQuestStateChanged);
	NarrativeComponent->OnQuestTaskProgressed.RemoveDynamic(this, &USFQuestLogWidgetController::HandleQuestTaskProgressed);
}

void USFQuestLogWidgetController::RefreshQuestLogDisplayData()
{
	RebuildDisplayEntries();
}

bool USFQuestLogWidgetController::BuildEntryForInstance(USFQuestInstance* Instance, FSFQuestDisplayEntry& OutEntry) const
{
	if (!Instance)
	{
		return false;
	}

	const USFQuestDefinition* Def = Instance->GetDefinition();
	if (!Def)
	{
		return false;
	}

	OutEntry = FSFQuestDisplayEntry{};
	OutEntry.QuestId = Def->QuestId;
	OutEntry.QuestAssetId = Def->GetPrimaryAssetId();
	OutEntry.Definition = Def;
	OutEntry.DisplayName = Def->DisplayName.IsEmpty() ? FText::FromName(Def->QuestId) : Def->DisplayName;
	OutEntry.Description = Def->Description;
	OutEntry.QuestTags = Def->QuestTags;
	OutEntry.CompletionState = Instance->GetCompletionState();
	OutEntry.CurrentStateId = Instance->GetCurrentStateId();
	OutEntry.bIsTracked = (TrackedQuestId == OutEntry.QuestId);

	// Find the active state definition, if any, to enumerate objectives.
	const FSFQuestStateDefinition* ActiveState = nullptr;
	for (const FSFQuestStateDefinition& State : Def->States)
	{
		if (State.StateId == OutEntry.CurrentStateId)
		{
			ActiveState = &State;
			break;
		}
	}

	if (ActiveState)
	{
		OutEntry.CurrentStateDisplayName = ActiveState->DisplayName.IsEmpty()
			? FText::FromName(ActiveState->StateId)
			: ActiveState->DisplayName;

		const TMap<FName, int32>& Progress = Instance->GetTaskProgressByTaskId();
		OutEntry.Objectives.Reserve(ActiveState->Tasks.Num());

		bool bAllComplete = ActiveState->Tasks.Num() > 0;
		for (const FSFQuestTaskDefinition& Task : ActiveState->Tasks)
		{
			FSFQuestObjectiveDisplayEntry Obj;
			Obj.TaskId = Task.TaskId;
			Obj.TaskTag = Task.TaskTag;
			Obj.ContextId = Task.ExpectedContextId;
			Obj.RequiredQuantity = FMath::Max(1, Task.RequiredQuantity);
			Obj.CurrentProgress = 0;
			if (const int32* Found = Progress.Find(Task.TaskId))
			{
				Obj.CurrentProgress = FMath::Clamp(*Found, 0, Obj.RequiredQuantity);
			}
			Obj.bIsComplete = (Obj.CurrentProgress >= Obj.RequiredQuantity);
			Obj.DisplayText = BuildObjectiveDisplayText(Task, Obj.CurrentProgress);
			bAllComplete &= Obj.bIsComplete;

			OutEntry.Objectives.Add(MoveTemp(Obj));
		}
		OutEntry.bAllObjectivesComplete = bAllComplete;
	}
	else
	{
		// No active state (likely terminal); collapse objectives but mark
		// completion based on lifecycle.
		OutEntry.bAllObjectivesComplete = (OutEntry.CompletionState == ESFQuestCompletionState::Succeeded);
	}

	return true;
}

void USFQuestLogWidgetController::RebuildDisplayEntries()
{
	CurrentDisplayEntries.Reset();

	if (!NarrativeComponent)
	{
		UE_LOG(LogSFQuestLogController, Verbose,
			TEXT("[QuestController] RebuildDisplayEntries: NarrativeComponent is null — broadcasting empty list."));
		OnQuestLogDisplayDataUpdated.Broadcast(CurrentDisplayEntries);
		return;
	}

	USFQuestRuntime* Runtime = NarrativeComponent->GetQuestRuntime();
	if (!Runtime)
	{
		UE_LOG(LogSFQuestLogController, Warning,
			TEXT("[QuestController] RebuildDisplayEntries: NarrativeComponent has no QuestRuntime. The narrative component may not have finished initializing yet."));
		OnQuestLogDisplayDataUpdated.Broadcast(CurrentDisplayEntries);
		return;
	}

	const TArray<USFQuestInstance*> Instances = Runtime->GetAllQuestInstances();
	UE_LOG(LogSFQuestLogController, Verbose,
		TEXT("[QuestController] RebuildDisplayEntries: runtime has %d quest instance(s)."),
		Instances.Num());
	CurrentDisplayEntries.Reserve(Instances.Num());

	for (USFQuestInstance* Instance : Instances)
	{
		FSFQuestDisplayEntry Entry;
		if (BuildEntryForInstance(Instance, Entry) && PassesFilter(Entry))
		{
			CurrentDisplayEntries.Add(MoveTemp(Entry));
		}
	}

	ApplySorting();

	// Re-validate selection after rebuild.
	if (!SelectedQuestId.IsNone())
	{
		const bool bStillVisible = CurrentDisplayEntries.ContainsByPredicate(
			[this](const FSFQuestDisplayEntry& E){ return E.QuestId == SelectedQuestId; });
		if (!bStillVisible)
		{
			SelectedQuestId = NAME_None;
			OnSelectionChanged.Broadcast(SelectedQuestId);
		}
	}

	OnQuestLogDisplayDataUpdated.Broadcast(CurrentDisplayEntries);
}

void USFQuestLogWidgetController::RefreshSingleQuestEntry(USFQuestInstance* Instance)
{
	FSFQuestDisplayEntry Updated;
	if (!BuildEntryForInstance(Instance, Updated))
	{
		return;
	}

	const bool bShouldShow = PassesFilter(Updated);
	const int32 ExistingIndex = CurrentDisplayEntries.IndexOfByPredicate(
		[QuestId = Updated.QuestId](const FSFQuestDisplayEntry& E){ return E.QuestId == QuestId; });

	if (!bShouldShow)
	{
		if (ExistingIndex != INDEX_NONE)
		{
			// Filter now excludes it; do a full rebuild + broadcast list update.
			RebuildDisplayEntries();
		}
		return;
	}

	if (ExistingIndex == INDEX_NONE)
	{
		// New row; full rebuild keeps sort order correct.
		RebuildDisplayEntries();
		return;
	}

	// In-place patch + targeted broadcast (no list reshape).
	CurrentDisplayEntries[ExistingIndex] = Updated;
	OnQuestUpdated.Broadcast(Updated.QuestId, Updated);
}

bool USFQuestLogWidgetController::TryGetEntryById(FName QuestId, FSFQuestDisplayEntry& OutEntry) const
{
	for (const FSFQuestDisplayEntry& Entry : CurrentDisplayEntries)
	{
		if (Entry.QuestId == QuestId)
		{
			OutEntry = Entry;
			return true;
		}
	}
	return false;
}

void USFQuestLogWidgetController::SetSortMode(ESFQuestLogSortMode InSortMode)
{
	if (SortMode == InSortMode)
	{
		return;
	}
	SortMode = InSortMode;
	ApplySorting();
	OnQuestLogDisplayDataUpdated.Broadcast(CurrentDisplayEntries);
}

void USFQuestLogWidgetController::CycleSortMode()
{
	const uint8 Next = (static_cast<uint8>(SortMode) + 1) % (static_cast<uint8>(ESFQuestLogSortMode::ProgressDescending) + 1);
	SetSortMode(static_cast<ESFQuestLogSortMode>(Next));
}

void USFQuestLogWidgetController::SetFilterFlags(int32 InFilterFlags)
{
	if (FilterFlags == InFilterFlags)
	{
		return;
	}
	FilterFlags = InFilterFlags;
	RebuildDisplayEntries();
}

bool USFQuestLogWidgetController::SelectQuest(FName QuestId)
{
	if (SelectedQuestId == QuestId)
	{
		return true;
	}
	const bool bExists = QuestId.IsNone()
		|| CurrentDisplayEntries.ContainsByPredicate(
			[QuestId](const FSFQuestDisplayEntry& E){ return E.QuestId == QuestId; });
	if (!bExists)
	{
		return false;
	}
	SelectedQuestId = QuestId;
	OnSelectionChanged.Broadcast(SelectedQuestId);
	return true;
}

void USFQuestLogWidgetController::ClearSelection()
{
	if (SelectedQuestId.IsNone())
	{
		return;
	}
	SelectedQuestId = NAME_None;
	OnSelectionChanged.Broadcast(SelectedQuestId);
}

void USFQuestLogWidgetController::SetTrackedQuest(FName QuestId)
{
	if (TrackedQuestId == QuestId)
	{
		return;
	}
	const FName Previous = TrackedQuestId;
	TrackedQuestId = QuestId;

	// Update the bIsTracked flag in cached entries without full rebuild.
	for (FSFQuestDisplayEntry& Entry : CurrentDisplayEntries)
	{
		const bool bWas = Entry.bIsTracked;
		Entry.bIsTracked = (Entry.QuestId == TrackedQuestId);
		if (bWas != Entry.bIsTracked)
		{
			OnQuestUpdated.Broadcast(Entry.QuestId, Entry);
		}
	}

	OnTrackedQuestChanged.Broadcast(TrackedQuestId, !TrackedQuestId.IsNone());

	// Default sort puts tracked quest first; resort if relevant.
	if (SortMode == ESFQuestLogSortMode::Default && (Previous != TrackedQuestId))
	{
		ApplySorting();
		OnQuestLogDisplayDataUpdated.Broadcast(CurrentDisplayEntries);
	}
}

void USFQuestLogWidgetController::ToggleTrackedQuest(FName QuestId)
{
	if (QuestId.IsNone())
	{
		return;
	}
	SetTrackedQuest(TrackedQuestId == QuestId ? NAME_None : QuestId);
}

bool USFQuestLogWidgetController::AbandonSelectedQuest()
{
	if (SelectedQuestId.IsNone() || !NarrativeComponent)
	{
		return false;
	}
	USFQuestRuntime* Runtime = NarrativeComponent->GetQuestRuntime();
	if (!Runtime)
	{
		return false;
	}

	FSFQuestDisplayEntry Entry;
	if (!TryGetEntryById(SelectedQuestId, Entry) || !Entry.Definition)
	{
		return false;
	}

	// AbandonQuestByDefinition is non-const but takes a non-const pointer.
	USFQuestDefinition* MutableDef = const_cast<USFQuestDefinition*>(Entry.Definition.Get());
	return Runtime->AbandonQuestByDefinition(MutableDef);
}

bool USFQuestLogWidgetController::PassesFilter(const FSFQuestDisplayEntry& Entry) const
{
	if (FilterFlags == 0)
	{
		return true;
	}

	const ESFQuestCompletionState S = Entry.CompletionState;
	const auto Has = [this](ESFQuestLogFilterFlags F)
	{
		return (FilterFlags & static_cast<int32>(F)) != 0;
	};

	if (IsActiveCompletionState(S) && Has(ESFQuestLogFilterFlags::Active)) return true;
	if (S == ESFQuestCompletionState::Succeeded && Has(ESFQuestLogFilterFlags::Succeeded)) return true;
	if (S == ESFQuestCompletionState::Failed    && Has(ESFQuestLogFilterFlags::Failed))    return true;
	if (S == ESFQuestCompletionState::Abandoned && Has(ESFQuestLogFilterFlags::Abandoned)) return true;
	return false;
}

int32 USFQuestLogWidgetController::GetCompletionStateBucket(ESFQuestCompletionState State) const
{
	// Lower number = higher in the list when SortMode == Default.
	switch (State)
	{
	case ESFQuestCompletionState::InProgress: return 0;
	case ESFQuestCompletionState::NotStarted: return 1;
	case ESFQuestCompletionState::Succeeded:  return 2;
	case ESFQuestCompletionState::Failed:     return 3;
	case ESFQuestCompletionState::Abandoned:  return 4;
	default:                                  return 5;
	}
}

void USFQuestLogWidgetController::ApplySorting()
{
	switch (SortMode)
	{
	case ESFQuestLogSortMode::NameAscending:
		CurrentDisplayEntries.StableSort([](const FSFQuestDisplayEntry& A, const FSFQuestDisplayEntry& B)
		{
			return A.DisplayName.CompareTo(B.DisplayName) < 0;
		});
		break;

	case ESFQuestLogSortMode::NameDescending:
		CurrentDisplayEntries.StableSort([](const FSFQuestDisplayEntry& A, const FSFQuestDisplayEntry& B)
		{
			return A.DisplayName.CompareTo(B.DisplayName) > 0;
		});
		break;

	case ESFQuestLogSortMode::ProgressDescending:
		CurrentDisplayEntries.StableSort([](const FSFQuestDisplayEntry& A, const FSFQuestDisplayEntry& B)
		{
			return A.GetAverageObjectiveProgress() > B.GetAverageObjectiveProgress();
		});
		break;

	case ESFQuestLogSortMode::NewestFirst:
		// We don't track per-quest update timestamps yet; fall back to default order.
		// (Hook a timestamp into FSFQuestDisplayEntry later if needed.)
		// Intentional fallthrough into Default.
	case ESFQuestLogSortMode::Default:
	default:
		CurrentDisplayEntries.StableSort([this](const FSFQuestDisplayEntry& A, const FSFQuestDisplayEntry& B)
		{
			// Tracked quest pinned to top.
			if (A.bIsTracked != B.bIsTracked)
			{
				return A.bIsTracked;
			}
			// Then by lifecycle bucket (active first).
			const int32 BucketA = GetCompletionStateBucket(A.CompletionState);
			const int32 BucketB = GetCompletionStateBucket(B.CompletionState);
			if (BucketA != BucketB)
			{
				return BucketA < BucketB;
			}
			// Then by display name for stability.
			return A.DisplayName.CompareTo(B.DisplayName) < 0;
		});
		break;
	}
}

// -----------------------------------------------------------------------------
// Narrative component delegate handlers
// -----------------------------------------------------------------------------

void USFQuestLogWidgetController::HandleQuestStarted(USFQuestInstance* QuestInstance)
{
	// New quest changes the list shape; full rebuild keeps sort/filter correct.
	RebuildDisplayEntries();
}

void USFQuestLogWidgetController::HandleQuestRestarted(USFQuestInstance* QuestInstance)
{
	RebuildDisplayEntries();
}

void USFQuestLogWidgetController::HandleQuestAbandoned(USFQuestInstance* QuestInstance)
{
	// Lifecycle changed; row may need to leave the visible set depending on filter.
	RefreshSingleQuestEntry(QuestInstance);
}

void USFQuestLogWidgetController::HandleQuestStateChanged(USFQuestInstance* QuestInstance, FName /*StateId*/)
{
	// State change rewrites the objective list inside the row but doesn't usually
	// change the row's position; targeted refresh is enough.
	RefreshSingleQuestEntry(QuestInstance);
}

void USFQuestLogWidgetController::HandleQuestTaskProgressed(
	USFQuestInstance* QuestInstance,
	FGameplayTag /*TaskTag*/,
	FName /*ContextId*/,
	int32 /*Quantity*/)
{
	RefreshSingleQuestEntry(QuestInstance);
}

#undef LOCTEXT_NAMESPACE
