// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "UI/SFPlayerQuestLogWidget.h"

#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Core/SFPlayerState.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Narrative/SFNarrativeComponent.h"
#include "TimerManager.h"
#include "UI/SFQuestEntryWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogSFQuestLogWidget, Log, All);

#define LOCTEXT_NAMESPACE "SFPlayerQuestLogWidget"

void USFPlayerQuestLogWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetController)
	{
		WidgetController = NewObject<USFQuestLogWidgetController>(this, TEXT("QuestLogWidgetController"));
	}

	if (SortButton && !SortButton->OnClicked.IsAlreadyBound(this, &USFPlayerQuestLogWidget::HandleSortButtonClicked))
	{
		SortButton->OnClicked.AddDynamic(this, &USFPlayerQuestLogWidget::HandleSortButtonClicked);
	}
	if (FilterButton && !FilterButton->OnClicked.IsAlreadyBound(this, &USFPlayerQuestLogWidget::HandleFilterButtonClicked))
	{
		FilterButton->OnClicked.AddDynamic(this, &USFPlayerQuestLogWidget::HandleFilterButtonClicked);
	}
	if (AbandonButton && !AbandonButton->OnClicked.IsAlreadyBound(this, &USFPlayerQuestLogWidget::HandleAbandonButtonClicked))
	{
		AbandonButton->OnClicked.AddDynamic(this, &USFPlayerQuestLogWidget::HandleAbandonButtonClicked);
	}

	BindToController();

	// Self-healing path: even when the parent menu didn't (or couldn't yet)
	// provide a narrative component, try to find one on the local player so
	// the widget still populates if/when the menu re-opens after a quest exists.
	if (WidgetController && !WidgetController->GetNarrativeComponent())
	{
		if (!TryAutoResolveNarrativeComponent())
		{
			// PlayerState/NarrativeComponent likely hasn't replicated yet — keep
			// retrying on a low-frequency timer so we light up the panel the
			// moment it becomes available.
			StartAutoResolveRetryTimer();
		}
	}
	else if (WidgetController && WidgetController->GetNarrativeComponent())
	{
		// We already have a component but the panel may still be showing the
		// pre-init empty broadcast. Force a fresh broadcast so any rows that
		// already exist appear.
		WidgetController->RefreshQuestLogDisplayData();
	}

	UpdateButtonLabels();
	RefreshSelectedDetails();

	UE_LOG(LogSFQuestLogWidget, Verbose,
		TEXT("[QuestLog] NativeConstruct: NarrativeComponent=%s, EntryContainer=%s, QuestEntryWidgetClass=%s"),
		*GetNameSafe(WidgetController ? WidgetController->GetNarrativeComponent() : nullptr),
		*GetNameSafe(EntryContainer),
		*GetNameSafe(QuestEntryWidgetClass));
}

void USFPlayerQuestLogWidget::NativeDestruct()
{
	StopAutoResolveRetryTimer();
	DeinitializeQuestLogWidget();
	UnbindFromController();

	if (SortButton)
	{
		SortButton->OnClicked.RemoveDynamic(this, &USFPlayerQuestLogWidget::HandleSortButtonClicked);
	}
	if (FilterButton)
	{
		FilterButton->OnClicked.RemoveDynamic(this, &USFPlayerQuestLogWidget::HandleFilterButtonClicked);
	}
	if (AbandonButton)
	{
		AbandonButton->OnClicked.RemoveDynamic(this, &USFPlayerQuestLogWidget::HandleAbandonButtonClicked);
	}

	Super::NativeDestruct();
}

void USFPlayerQuestLogWidget::InitializeQuestLogWidget(USFNarrativeComponent* InNarrativeComponent)
{
	if (!WidgetController)
	{
		WidgetController = NewObject<USFQuestLogWidgetController>(this, TEXT("QuestLogWidgetController"));
	}

	BindToController();

	// If the caller couldn't supply a narrative component (e.g. PlayerState
	// hadn't replicated yet), fall back to walking the owning player so we
	// don't silently leave the controller in a broken/empty state.
	USFNarrativeComponent* ResolvedComponent = InNarrativeComponent;
	if (!ResolvedComponent)
	{
		UE_LOG(LogSFQuestLogWidget, Warning,
			TEXT("[QuestLog] InitializeQuestLogWidget called with null NarrativeComponent. Attempting auto-resolve from owning player."));
	}

	if (WidgetController)
	{
		WidgetController->Initialize(ResolvedComponent);

		if (!ResolvedComponent)
		{
			if (!TryAutoResolveNarrativeComponent())
			{
				StartAutoResolveRetryTimer();
			}
		}
		else
		{
			StopAutoResolveRetryTimer();
			WidgetController->RefreshQuestLogDisplayData();
		}
	}

	UpdateButtonLabels();
	RefreshSelectedDetails();

	USFNarrativeComponent* FinalComponent = WidgetController ? WidgetController->GetNarrativeComponent() : nullptr;
	const int32 EntryCount = WidgetController ? WidgetController->GetCurrentDisplayEntries().Num() : 0;
	UE_LOG(LogSFQuestLogWidget, Log,
		TEXT("[QuestLog] InitializeQuestLogWidget done. NarrativeComponent=%s, Entries=%d, EntryContainer=%s, QuestEntryWidgetClass=%s"),
		*GetNameSafe(FinalComponent),
		EntryCount,
		*GetNameSafe(EntryContainer),
		*GetNameSafe(QuestEntryWidgetClass));

	if (!EntryContainer)
	{
		UE_LOG(LogSFQuestLogWidget, Warning,
			TEXT("[QuestLog] EntryContainer is null. Add a UPanelWidget (ScrollBox or VerticalBox) named 'EntryContainer' inside WBP_PlayerQuestLog and check Is Variable."));
	}
	if (!QuestEntryWidgetClass)
	{
		UE_LOG(LogSFQuestLogWidget, Warning,
			TEXT("[QuestLog] QuestEntryWidgetClass is unset. Open WBP_PlayerQuestLog Class Defaults and assign your WBP_QuestEntry to 'Quest Entry Widget Class'."));
	}
}

void USFPlayerQuestLogWidget::DeinitializeQuestLogWidget()
{
	ClearEntryWidgets();
	SelectedQuestId = NAME_None;

	if (WidgetController)
	{
		UnbindFromController();
		WidgetController->Deinitialize();
	}
}

void USFPlayerQuestLogWidget::BindToController()
{
	if (!WidgetController)
	{
		return;
	}

	if (!WidgetController->OnQuestLogDisplayDataUpdated.IsAlreadyBound(this, &USFPlayerQuestLogWidget::HandleQuestLogDisplayDataUpdated))
	{
		WidgetController->OnQuestLogDisplayDataUpdated.AddDynamic(this, &USFPlayerQuestLogWidget::HandleQuestLogDisplayDataUpdated);
	}
	if (!WidgetController->OnQuestUpdated.IsAlreadyBound(this, &USFPlayerQuestLogWidget::HandleQuestUpdated))
	{
		WidgetController->OnQuestUpdated.AddDynamic(this, &USFPlayerQuestLogWidget::HandleQuestUpdated);
	}
	if (!WidgetController->OnSelectionChanged.IsAlreadyBound(this, &USFPlayerQuestLogWidget::HandleSelectionChanged))
	{
		WidgetController->OnSelectionChanged.AddDynamic(this, &USFPlayerQuestLogWidget::HandleSelectionChanged);
	}
	if (!WidgetController->OnTrackedQuestChanged.IsAlreadyBound(this, &USFPlayerQuestLogWidget::HandleTrackedQuestChanged))
	{
		WidgetController->OnTrackedQuestChanged.AddDynamic(this, &USFPlayerQuestLogWidget::HandleTrackedQuestChanged);
	}
}

bool USFPlayerQuestLogWidget::TryAutoResolveNarrativeComponent()
{
	if (!WidgetController)
	{
		return false;
	}
	if (WidgetController->GetNarrativeComponent())
	{
		return true;
	}

	// Walk the local player chain to find an ASFPlayerState that owns a
	// USFNarrativeComponent. Works regardless of which actor created this widget.
	APlayerController* PC = GetOwningPlayer();
	APlayerState* PS = PC ? PC->PlayerState : nullptr;
	ASFPlayerState* SFPS = Cast<ASFPlayerState>(PS);
	USFNarrativeComponent* NC = SFPS ? SFPS->GetNarrativeComponent() : nullptr;

	if (!NC)
	{
		UE_LOG(LogSFQuestLogWidget, Warning,
			TEXT("[QuestLog] Auto-resolve failed. PC=%s PS=%s SFPS=%s NC=null. Verify the active GameMode's PlayerStateClass is ASFPlayerState (or a subclass)."),
			*GetNameSafe(PC), *GetNameSafe(PS), *GetNameSafe(SFPS));
		return false;
	}

	WidgetController->Initialize(NC);
	WidgetController->RefreshQuestLogDisplayData();

	// Successful resolve — stop the retry pump if it was running.
	StopAutoResolveRetryTimer();

	UE_LOG(LogSFQuestLogWidget, Log,
		TEXT("[QuestLog] Auto-resolved NarrativeComponent from %s. Entries=%d."),
		*GetNameSafe(SFPS),
		WidgetController->GetCurrentDisplayEntries().Num());
	return true;
}

void USFPlayerQuestLogWidget::StartAutoResolveRetryTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (World->GetTimerManager().IsTimerActive(AutoResolveRetryHandle))
	{
		return;
	}

	AutoResolveRetryElapsedSeconds = 0.0f;
	const float Period = FMath::Max(0.1f, AutoResolveRetryPeriodSeconds);
	World->GetTimerManager().SetTimer(
		AutoResolveRetryHandle,
		this,
		&USFPlayerQuestLogWidget::TickAutoResolveRetry,
		Period,
		/*bLoop=*/true);
}

void USFPlayerQuestLogWidget::StopAutoResolveRetryTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoResolveRetryHandle);
	}
	AutoResolveRetryElapsedSeconds = 0.0f;
}

void USFPlayerQuestLogWidget::TickAutoResolveRetry()
{
	// Success path: stop and refresh.
	if (TryAutoResolveNarrativeComponent())
	{
		StopAutoResolveRetryTimer();
		return;
	}

	AutoResolveRetryElapsedSeconds += FMath::Max(0.1f, AutoResolveRetryPeriodSeconds);
	if (AutoResolveRetryElapsedSeconds >= MaxAutoResolveRetrySeconds)
	{
		UE_LOG(LogSFQuestLogWidget, Warning,
			TEXT("[QuestLog] Gave up auto-resolving NarrativeComponent after %.1fs. ")
			TEXT("Quest panel will stay empty. Confirm GameMode->PlayerStateClass is ASFPlayerState (or a subclass) and that the player has a USFNarrativeComponent."),
			AutoResolveRetryElapsedSeconds);
		StopAutoResolveRetryTimer();
	}
}

void USFPlayerQuestLogWidget::UnbindFromController()
{
	if (!WidgetController)
	{
		return;
	}
	WidgetController->OnQuestLogDisplayDataUpdated.RemoveDynamic(this, &USFPlayerQuestLogWidget::HandleQuestLogDisplayDataUpdated);
	WidgetController->OnQuestUpdated.RemoveDynamic(this, &USFPlayerQuestLogWidget::HandleQuestUpdated);
	WidgetController->OnSelectionChanged.RemoveDynamic(this, &USFPlayerQuestLogWidget::HandleSelectionChanged);
	WidgetController->OnTrackedQuestChanged.RemoveDynamic(this, &USFPlayerQuestLogWidget::HandleTrackedQuestChanged);
}

void USFPlayerQuestLogWidget::ClearEntryWidgets()
{
	if (EntryContainer)
	{
		EntryContainer->ClearChildren();
	}
	for (TObjectPtr<USFQuestEntryWidget>& W : SpawnedEntryWidgets)
	{
		if (W)
		{
			W->OnQuestEntryClicked.RemoveDynamic(this, &USFPlayerQuestLogWidget::HandleEntryClicked);
			W->OnQuestEntryTrackToggled.RemoveDynamic(this, &USFPlayerQuestLogWidget::HandleEntryTrackToggled);
			W->OnQuestEntryAbandonRequested.RemoveDynamic(this, &USFPlayerQuestLogWidget::HandleEntryAbandonRequested);
		}
	}
	SpawnedEntryWidgets.Reset();
}

void USFPlayerQuestLogWidget::RebuildEntryWidgets()
{
	ClearEntryWidgets();

	if (!EntryContainer || !QuestEntryWidgetClass || !WidgetController)
	{
		BP_OnEmptyStateChanged(true);
		if (EmptyStateText)
		{
			EmptyStateText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		return;
	}

	const TArray<FSFQuestDisplayEntry>& Entries = WidgetController->GetCurrentDisplayEntries();
	const bool bEmpty = (Entries.Num() == 0);

	if (EmptyStateText)
	{
		EmptyStateText->SetVisibility(bEmpty ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	for (const FSFQuestDisplayEntry& Entry : Entries)
	{
		USFQuestEntryWidget* Row = CreateWidget<USFQuestEntryWidget>(this, QuestEntryWidgetClass);
		if (!Row)
		{
			continue;
		}

		Row->SetQuestEntry(Entry);
		Row->SetIsSelected(Entry.QuestId == SelectedQuestId);
		Row->SetIsExpanded(Entry.QuestId == SelectedQuestId);

		Row->OnQuestEntryClicked.AddDynamic(this, &USFPlayerQuestLogWidget::HandleEntryClicked);
		Row->OnQuestEntryTrackToggled.AddDynamic(this, &USFPlayerQuestLogWidget::HandleEntryTrackToggled);
		Row->OnQuestEntryAbandonRequested.AddDynamic(this, &USFPlayerQuestLogWidget::HandleEntryAbandonRequested);

		EntryContainer->AddChild(Row);
		SpawnedEntryWidgets.Add(Row);
	}

	BP_OnQuestEntriesRebuilt();
	BP_OnEmptyStateChanged(bEmpty);
}

USFQuestEntryWidget* USFPlayerQuestLogWidget::FindEntryWidgetById(FName QuestId) const
{
	for (const TObjectPtr<USFQuestEntryWidget>& W : SpawnedEntryWidgets)
	{
		if (W && W->GetQuestEntry().QuestId == QuestId)
		{
			return W.Get();
		}
	}
	return nullptr;
}

const FSFQuestDisplayEntry* USFPlayerQuestLogWidget::GetSelectedEntry() const
{
	if (!WidgetController || SelectedQuestId.IsNone())
	{
		return nullptr;
	}
	for (const FSFQuestDisplayEntry& E : WidgetController->GetCurrentDisplayEntries())
	{
		if (E.QuestId == SelectedQuestId)
		{
			return &E;
		}
	}
	return nullptr;
}

void USFPlayerQuestLogWidget::RefreshSelectedDetails()
{
	const FSFQuestDisplayEntry* Selected = GetSelectedEntry();
	const bool bHasSelection = (Selected != nullptr);

	if (SelectedQuestNameText)
	{
		SelectedQuestNameText->SetText(bHasSelection ? Selected->DisplayName : FText::GetEmpty());
	}
	if (SelectedQuestDescriptionText)
	{
		SelectedQuestDescriptionText->SetText(bHasSelection ? Selected->Description : FText::GetEmpty());
	}
	if (AbandonButton)
	{
		AbandonButton->SetIsEnabled(bHasSelection && Selected->IsActive());
	}

	BP_OnSelectionChanged(
		bHasSelection ? *Selected : FSFQuestDisplayEntry{},
		bHasSelection);
}

FText USFPlayerQuestLogWidget::BuildSortLabelText() const
{
	const ESFQuestLogSortMode Mode = WidgetController ? WidgetController->GetSortMode() : ESFQuestLogSortMode::Default;
	switch (Mode)
	{
	case ESFQuestLogSortMode::NameAscending:      return LOCTEXT("SortNameAsc",      "Name A-Z");
	case ESFQuestLogSortMode::NameDescending:     return LOCTEXT("SortNameDesc",     "Name Z-A");
	case ESFQuestLogSortMode::NewestFirst:        return LOCTEXT("SortNewest",       "Newest");
	case ESFQuestLogSortMode::ProgressDescending: return LOCTEXT("SortProgress",     "Progress");
	case ESFQuestLogSortMode::Default:
	default:                                      return LOCTEXT("SortDefault",      "Default");
	}
}

FText USFPlayerQuestLogWidget::BuildFilterLabelText() const
{
	if (!WidgetController)
	{
		return LOCTEXT("FilterAll", "All");
	}
	const int32 Flags = WidgetController->GetFilterFlags();
	if (Flags == 0)
	{
		return LOCTEXT("FilterAll", "All");
	}
	if (Flags == static_cast<int32>(ESFQuestLogFilterFlags::Active))
	{
		return LOCTEXT("FilterActive", "Active");
	}
	return LOCTEXT("FilterCustom", "Filtered");
}

void USFPlayerQuestLogWidget::UpdateButtonLabels()
{
	if (SortButtonText)
	{
		SortButtonText->SetText(BuildSortLabelText());
	}
	if (FilterButtonText)
	{
		FilterButtonText->SetText(BuildFilterLabelText());
	}
}

// -----------------------------------------------------------------------------
// Controller delegate handlers
// -----------------------------------------------------------------------------

void USFPlayerQuestLogWidget::HandleQuestLogDisplayDataUpdated(const TArray<FSFQuestDisplayEntry>& /*UpdatedEntries*/)
{
	RebuildEntryWidgets();
	RefreshSelectedDetails();
	UpdateButtonLabels();
}

void USFPlayerQuestLogWidget::HandleQuestUpdated(FName QuestId, const FSFQuestDisplayEntry& UpdatedEntry)
{
	if (USFQuestEntryWidget* Row = FindEntryWidgetById(QuestId))
	{
		Row->SetQuestEntry(UpdatedEntry);
	}
	if (QuestId == SelectedQuestId)
	{
		RefreshSelectedDetails();
	}
}

void USFPlayerQuestLogWidget::HandleSelectionChanged(FName NewSelectedQuestId)
{
	if (SelectedQuestId == NewSelectedQuestId)
	{
		return;
	}

	const FName Previous = SelectedQuestId;
	SelectedQuestId = NewSelectedQuestId;

	if (USFQuestEntryWidget* PrevRow = FindEntryWidgetById(Previous))
	{
		PrevRow->SetIsSelected(false);
		PrevRow->SetIsExpanded(false);
	}
	if (USFQuestEntryWidget* NewRow = FindEntryWidgetById(SelectedQuestId))
	{
		NewRow->SetIsSelected(true);
		NewRow->SetIsExpanded(true);
	}

	RefreshSelectedDetails();
}

void USFPlayerQuestLogWidget::HandleTrackedQuestChanged(FName /*TrackedQuestId*/, bool /*bIsTracked*/)
{
	// Per-row visuals (the "Track/Untrack" button label) update via OnQuestUpdated;
	// nothing extra to do at the list level for now.
}

// -----------------------------------------------------------------------------
// Per-row delegate handlers
// -----------------------------------------------------------------------------

void USFPlayerQuestLogWidget::HandleEntryClicked(FName QuestId)
{
	if (!WidgetController)
	{
		return;
	}
	// Toggle selection: clicking the selected row collapses it.
	if (SelectedQuestId == QuestId)
	{
		WidgetController->ClearSelection();
	}
	else
	{
		WidgetController->SelectQuest(QuestId);
	}
}

void USFPlayerQuestLogWidget::HandleEntryTrackToggled(FName QuestId)
{
	if (WidgetController)
	{
		WidgetController->ToggleTrackedQuest(QuestId);
	}
}

void USFPlayerQuestLogWidget::HandleEntryAbandonRequested(FName QuestId)
{
	if (!WidgetController)
	{
		return;
	}
	// Make sure the selection is on this quest, then abandon.
	WidgetController->SelectQuest(QuestId);
	WidgetController->AbandonSelectedQuest();
}

// -----------------------------------------------------------------------------
// Toolbar buttons
// -----------------------------------------------------------------------------

void USFPlayerQuestLogWidget::HandleSortButtonClicked()
{
	if (WidgetController)
	{
		WidgetController->CycleSortMode();
	}
	UpdateButtonLabels();
}

void USFPlayerQuestLogWidget::HandleFilterButtonClicked()
{
	if (!WidgetController)
	{
		return;
	}
	// Simple toggle: Active-only <-> All.
	const int32 Current = WidgetController->GetFilterFlags();
	const int32 ActiveOnly = static_cast<int32>(ESFQuestLogFilterFlags::Active);
	WidgetController->SetFilterFlags(Current == ActiveOnly ? 0 : ActiveOnly);
	UpdateButtonLabels();
}

void USFPlayerQuestLogWidget::HandleAbandonButtonClicked()
{
	if (WidgetController)
	{
		WidgetController->AbandonSelectedQuest();
	}
}

#undef LOCTEXT_NAMESPACE
