#include "UI/SFInteractionPromptWidget.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "UI/SFPlayerHUDWidgetController.h"

void USFInteractionPromptWidget::NativeOnPlayerHUDWidgetControllerSet()
{
	Super::NativeOnPlayerHUDWidgetControllerSet();

	// The PlayerController sets the controller AFTER widget construction in
	// most cases, but if it's set before NativeConstruct we still want to
	// bind so the prime-state path runs. Either order is safe — BindToController
	// short-circuits when the controller is already bound.
	BindToController();
}

void USFInteractionPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Default to "no prompt" visuals on construction so the HUD does not
	// flash a stale "Talk" before the first delegate broadcast.
	ApplyPromptState(/*bHasPrompt*/ false, FText::GetEmpty(), FText::GetEmpty(), /*bAvailable*/ false);

	BindToController();
}

void USFInteractionPromptWidget::NativeDestruct()
{
	UnbindFromController();
	Super::NativeDestruct();
}

void USFInteractionPromptWidget::BindToController()
{
	if (!PlayerHUDWidgetController)
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("SFInteractionPromptWidget: no controller assigned yet on '%s'."),
			*GetNameSafe(this));
		return;
	}

	// Already bound to this exact controller — nothing to do.
	if (BoundController == PlayerHUDWidgetController)
	{
		return;
	}

	// Different controller previously bound: unhook first so we don't leak
	// duplicate handlers if the pawn swaps.
	UnbindFromController();

	BoundController = PlayerHUDWidgetController;

	BoundController->OnInteractionPromptChanged.AddDynamic(
		this, &USFInteractionPromptWidget::HandleInteractionPromptChanged);
	BoundController->OnInteractionOptionsChanged.AddDynamic(
		this, &USFInteractionPromptWidget::HandleInteractionOptionsChanged);

	// Prime the widget with whatever the controller already knows so the
	// prompt is correct on level-load even if no change has fired yet.
	HandleInteractionPromptChanged(
		BoundController->HasInteractionPrompt(),
		BoundController->GetInteractionPromptText());
}

void USFInteractionPromptWidget::UnbindFromController()
{
	if (!BoundController)
	{
		return;
	}

	BoundController->OnInteractionPromptChanged.RemoveDynamic(
		this, &USFInteractionPromptWidget::HandleInteractionPromptChanged);
	BoundController->OnInteractionOptionsChanged.RemoveDynamic(
		this, &USFInteractionPromptWidget::HandleInteractionOptionsChanged);

	BoundController = nullptr;
}

void USFInteractionPromptWidget::HandleInteractionPromptChanged(
	bool bHasPrompt,
	const FText& PromptText)
{
	// Prompt-only path: no option metadata, treat as Available.
	ApplyPromptState(bHasPrompt, PromptText, FText::GetEmpty(), /*bAvailable*/ true);
}

void USFInteractionPromptWidget::HandleInteractionOptionsChanged(
	bool bHasInteractable,
	FSFInteractionOption PrimaryOption,
	const TArray<FSFInteractionOption>& /*AllOptions*/)
{
	const bool bAvailable = PrimaryOption.IsAvailable();
	const bool bHasPrompt =
		bHasInteractable
		&& !PrimaryOption.PromptText.IsEmpty()
		&& (bAvailable || PrimaryOption.bShowPromptWhenUnavailable);

	ApplyPromptState(
		bHasPrompt,
		PrimaryOption.PromptText,
		PrimaryOption.SubPromptText,
		bAvailable);
}

void USFInteractionPromptWidget::ApplyPromptState(
	bool bHasPrompt,
	const FText& PromptText,
	const FText& SubPromptText,
	bool bAvailable)
{
	const bool bShouldShow = bHasPrompt && (bAvailable || !bHideWhenUnavailable);

	// Visibility: prefer the bound PromptRoot so the widget itself can keep
	// reacting to anim notifies; fall back to self if no root is bound.
	if (PromptRoot)
	{
		PromptRoot->SetVisibility(bShouldShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	else
	{
		SetVisibility(bShouldShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	const FLinearColor TintColor = bAvailable ? AvailableColor : UnavailableColor;

	if (PromptTextBlock)
	{
		PromptTextBlock->SetText(PromptText);
		PromptTextBlock->SetColorAndOpacity(FSlateColor(TintColor));
	}

	if (SubPromptTextBlock)
	{
		SubPromptTextBlock->SetText(SubPromptText);
		SubPromptTextBlock->SetColorAndOpacity(FSlateColor(TintColor));
		SubPromptTextBlock->SetVisibility(
			SubPromptText.IsEmpty()
				? ESlateVisibility::Collapsed
				: ESlateVisibility::HitTestInvisible);
	}
}
