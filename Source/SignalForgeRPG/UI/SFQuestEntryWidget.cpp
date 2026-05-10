// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "UI/SFQuestEntryWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/SFQuestObjectiveEntryWidget.h"

#define LOCTEXT_NAMESPACE "SFQuestEntryWidget"

void USFQuestEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (EntryButton && !EntryButton->OnClicked.IsAlreadyBound(this, &USFQuestEntryWidget::HandleEntryButtonClicked))
	{
		EntryButton->OnClicked.AddDynamic(this, &USFQuestEntryWidget::HandleEntryButtonClicked);
	}
	if (TrackButton && !TrackButton->OnClicked.IsAlreadyBound(this, &USFQuestEntryWidget::HandleTrackButtonClicked))
	{
		TrackButton->OnClicked.AddDynamic(this, &USFQuestEntryWidget::HandleTrackButtonClicked);
	}
	if (AbandonButton && !AbandonButton->OnClicked.IsAlreadyBound(this, &USFQuestEntryWidget::HandleAbandonButtonClicked))
	{
		AbandonButton->OnClicked.AddDynamic(this, &USFQuestEntryWidget::HandleAbandonButtonClicked);
	}

	RefreshHeaderVisuals();
	RefreshSelectionVisuals();
	RefreshObjectiveWidgets();
}

void USFQuestEntryWidget::NativeDestruct()
{
	if (EntryButton)
	{
		EntryButton->OnClicked.RemoveDynamic(this, &USFQuestEntryWidget::HandleEntryButtonClicked);
	}
	if (TrackButton)
	{
		TrackButton->OnClicked.RemoveDynamic(this, &USFQuestEntryWidget::HandleTrackButtonClicked);
	}
	if (AbandonButton)
	{
		AbandonButton->OnClicked.RemoveDynamic(this, &USFQuestEntryWidget::HandleAbandonButtonClicked);
	}

	ClearObjectiveWidgets();
	Super::NativeDestruct();
}

FReply USFQuestEntryWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Double-click toggles tracking, mirroring inventory entry's "double-click equip".
	if (!CurrentEntry.QuestId.IsNone())
	{
		OnQuestEntryTrackToggled.Broadcast(CurrentEntry.QuestId);
	}
	return FReply::Handled();
}

void USFQuestEntryWidget::SetQuestEntry(const FSFQuestDisplayEntry& InEntry)
{
	CurrentEntry = InEntry;
	RefreshHeaderVisuals();
	RefreshObjectiveWidgets();
	BP_OnQuestEntrySet(CurrentEntry);
}

void USFQuestEntryWidget::SetIsSelected(bool bInIsSelected)
{
	if (bIsSelected == bInIsSelected)
	{
		return;
	}
	bIsSelected = bInIsSelected;
	RefreshSelectionVisuals();
	BP_OnSelectionChanged(bIsSelected);
}

void USFQuestEntryWidget::SetIsExpanded(bool bInIsExpanded)
{
	if (bIsExpanded == bInIsExpanded)
	{
		return;
	}
	bIsExpanded = bInIsExpanded;

	if (ObjectivesContainer)
	{
		ObjectivesContainer->SetVisibility(
			bIsExpanded
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);
	}

	BP_OnExpansionChanged(bIsExpanded);
}

void USFQuestEntryWidget::HandleEntryButtonClicked()
{
	if (!CurrentEntry.QuestId.IsNone())
	{
		OnQuestEntryClicked.Broadcast(CurrentEntry.QuestId);
	}
}

void USFQuestEntryWidget::HandleTrackButtonClicked()
{
	if (!CurrentEntry.QuestId.IsNone())
	{
		OnQuestEntryTrackToggled.Broadcast(CurrentEntry.QuestId);
	}
}

void USFQuestEntryWidget::HandleAbandonButtonClicked()
{
	if (!CurrentEntry.QuestId.IsNone())
	{
		OnQuestEntryAbandonRequested.Broadcast(CurrentEntry.QuestId);
	}
}

void USFQuestEntryWidget::RefreshHeaderVisuals()
{
	if (QuestNameText)
	{
		QuestNameText->SetText(CurrentEntry.DisplayName);
	}
	if (CurrentStateText)
	{
		CurrentStateText->SetText(CurrentEntry.CurrentStateDisplayName);
		CurrentStateText->SetVisibility(
			CurrentEntry.CurrentStateDisplayName.IsEmpty()
				? ESlateVisibility::Collapsed
				: ESlateVisibility::SelfHitTestInvisible);
	}
	if (StatusText)
	{
		StatusText->SetText(BuildStatusText());
	}
	if (DescriptionText)
	{
		DescriptionText->SetText(CurrentEntry.Description);
		DescriptionText->SetVisibility(
			CurrentEntry.Description.IsEmpty()
				? ESlateVisibility::Collapsed
				: ESlateVisibility::SelfHitTestInvisible);
	}
	if (ProgressBar)
	{
		ProgressBar->SetPercent(CurrentEntry.GetAverageObjectiveProgress());
	}
	if (TrackButtonText)
	{
		TrackButtonText->SetText(
			CurrentEntry.bIsTracked
				? LOCTEXT("UntrackLabel", "Untrack")
				: LOCTEXT("TrackLabel",   "Track"));
	}
	if (TrackButton)
	{
		// Hide tracking entirely for finished quests.
		TrackButton->SetVisibility(
			CurrentEntry.IsFinished()
				? ESlateVisibility::Collapsed
				: ESlateVisibility::Visible);
	}
	if (AbandonButton)
	{
		// Only show abandon for active quests.
		AbandonButton->SetVisibility(
			CurrentEntry.IsActive()
				? ESlateVisibility::Visible
				: ESlateVisibility::Collapsed);
	}
}

void USFQuestEntryWidget::RefreshSelectionVisuals()
{
	if (SelectionBorder)
	{
		SelectionBorder->SetVisibility(
			bIsSelected
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Hidden);
	}
}

void USFQuestEntryWidget::ClearObjectiveWidgets()
{
	if (ObjectivesContainer)
	{
		ObjectivesContainer->ClearChildren();
	}
	SpawnedObjectiveWidgets.Reset();
}

void USFQuestEntryWidget::RefreshObjectiveWidgets()
{
	ClearObjectiveWidgets();

	if (!ObjectivesContainer || !ObjectiveEntryWidgetClass)
	{
		return;
	}

	for (const FSFQuestObjectiveDisplayEntry& Objective : CurrentEntry.Objectives)
	{
		USFQuestObjectiveEntryWidget* Row = CreateWidget<USFQuestObjectiveEntryWidget>(this, ObjectiveEntryWidgetClass);
		if (!Row)
		{
			continue;
		}
		Row->SetObjective(Objective);
		ObjectivesContainer->AddChildToVerticalBox(Row);
		SpawnedObjectiveWidgets.Add(Row);
	}

	if (ObjectivesContainer)
	{
		ObjectivesContainer->SetVisibility(
			bIsExpanded
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);
	}
}

FText USFQuestEntryWidget::BuildStatusText() const
{
	switch (CurrentEntry.CompletionState)
	{
	case ESFQuestCompletionState::NotStarted: return LOCTEXT("StatusNotStarted", "Not Started");
	case ESFQuestCompletionState::InProgress: return LOCTEXT("StatusInProgress", "In Progress");
	case ESFQuestCompletionState::Succeeded:  return LOCTEXT("StatusSucceeded",  "Completed");
	case ESFQuestCompletionState::Failed:     return LOCTEXT("StatusFailed",     "Failed");
	case ESFQuestCompletionState::Abandoned:  return LOCTEXT("StatusAbandoned",  "Abandoned");
	default:                                  return FText::GetEmpty();
	}
}

#undef LOCTEXT_NAMESPACE
