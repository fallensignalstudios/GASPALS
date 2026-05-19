#include "UI/SFDialogueChoiceButtonWidget.h"

#include "Components/Button.h"
#include "Components/RichTextBlock.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

void USFDialogueChoiceButtonWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	ApplyWrappingConfig();
}

void USFDialogueChoiceButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ApplyWrappingConfig();

	if (ChoiceButton)
	{
		ChoiceButton->OnClicked.RemoveDynamic(this, &USFDialogueChoiceButtonWidget::HandleButtonClicked);
		ChoiceButton->OnClicked.AddDynamic(this, &USFDialogueChoiceButtonWidget::HandleButtonClicked);
	}
}

void USFDialogueChoiceButtonWidget::NativeDestruct()
{
	if (ChoiceButton)
	{
		ChoiceButton->OnClicked.RemoveDynamic(this, &USFDialogueChoiceButtonWidget::HandleButtonClicked);
	}
	Super::NativeDestruct();
}

void USFDialogueChoiceButtonWidget::SetChoiceText(const FText& InText)
{
	if (PromptText)
	{
		PromptText->SetText(InText);
	}
	if (PromptRichText)
	{
		PromptRichText->SetText(InText);
	}
	ApplyWrappingConfig();
}

void USFDialogueChoiceButtonWidget::SetLocked(bool bInLocked)
{
	if (bLocked == bInLocked)
	{
		return;
	}
	bLocked = bInLocked;

	if (ChoiceButton)
	{
		ChoiceButton->SetIsEnabled(!bLocked);
	}

	OnLockedChanged(bLocked);
}

void USFDialogueChoiceButtonWidget::HandleButtonClicked()
{
	if (bLocked)
	{
		return;
	}
	OnChoiceClicked.Broadcast(ChoiceIndex);
}

void USFDialogueChoiceButtonWidget::ApplyWrappingConfig()
{
	// The classic "wraps after every word" symptom comes from a text block
	// with AutoWrapText=true whose parent slot has no minimum width — Slate
	// resolves the desired size to a single word, then wraps at every space.
	//
	// Configure both the TextBlock/RichTextBlock and the optional SizeBox
	// to enforce a sensible minimum width and disable per-character wrap.

	if (PromptText)
	{
		PromptText->SetAutoWrapText(true);
		PromptText->SetWrapTextAt(0.0f);
		PromptText->SetMinDesiredWidth(MinChoiceWidth);

		// Belt and suspenders: ensure justification doesn't fight the parent.
		PromptText->SetJustification(ETextJustify::Left);
	}

	if (PromptRichText)
	{
		PromptRichText->SetAutoWrapText(true);
		PromptRichText->SetWrapTextAt(0.0f);
		// URichTextBlock doesn't expose SetMinDesiredWidth in all engine versions.
		// The SizeBox wrapping handles minimum width when present.
		PromptRichText->SetJustification(ETextJustify::Left);
	}

	if (PromptSizeBox)
	{
		PromptSizeBox->SetMinDesiredWidth(MinChoiceWidth);
	}
}
