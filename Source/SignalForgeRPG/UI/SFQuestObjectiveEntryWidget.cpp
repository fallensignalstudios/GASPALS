// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "UI/SFQuestObjectiveEntryWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "SFQuestObjectiveEntryWidget"

void USFQuestObjectiveEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshVisuals();
}

void USFQuestObjectiveEntryWidget::SetObjective(const FSFQuestObjectiveDisplayEntry& InObjective)
{
	CurrentObjective = InObjective;
	RefreshVisuals();
	BP_OnObjectiveSet(CurrentObjective);
}

void USFQuestObjectiveEntryWidget::RefreshVisuals()
{
	if (ObjectiveText)
	{
		ObjectiveText->SetText(CurrentObjective.DisplayText);
	}

	if (ProgressText)
	{
		if (CurrentObjective.RequiredQuantity > 1)
		{
			ProgressText->SetText(FText::Format(
				LOCTEXT("ProgressFormat", "{0}/{1}"),
				FText::AsNumber(CurrentObjective.CurrentProgress),
				FText::AsNumber(CurrentObjective.RequiredQuantity)));
			ProgressText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			ProgressText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (ProgressBar)
	{
		if (CurrentObjective.RequiredQuantity > 1)
		{
			ProgressBar->SetPercent(CurrentObjective.GetProgressFraction());
			ProgressBar->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			ProgressBar->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (CompletionCheckImage)
	{
		CompletionCheckImage->SetVisibility(
			CurrentObjective.bIsComplete
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Hidden);
	}
}

#undef LOCTEXT_NAMESPACE
