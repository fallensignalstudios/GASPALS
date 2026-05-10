// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "UI/SFPlayerQuestLogWidget.h"

#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Narrative/SFNarrativeComponent.h"
#include "UI/SFQuestEntryWidget.h"

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
	UpdateButtonLabels();
	RefreshSelectedDetails();
}

void USFPlayerQuestLogWidget::NativeDestruct()
{
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

	if (WidgetController)
	{
		WidgetController->Initialize(InNarrativeComponent);
		WidgetController->RefreshQuestLogDisplayData();
	}

	UpdateButtonLabels();
	RefreshSelectedDetails();
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
