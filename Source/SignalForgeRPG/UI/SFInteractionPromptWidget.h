#pragma once

#include "CoreMinimal.h"
#include "Components/SFInteractableInterface.h"
#include "UI/SFUserWidgetBase.h"
#include "SFInteractionPromptWidget.generated.h"

class UTextBlock;
class UPanelWidget;
class UWidget;

/**
 * Drop-in HUD widget that renders the current interaction prompt.
 *
 * Blueprint subclasses (e.g. WBP_InteractionPrompt) can wire any of these
 * BindWidgetOptional members:
 *
 *   - PromptTextBlock      : main prompt text ("Talk", "Pick up", ...)
 *   - SubPromptTextBlock   : optional sub-text ("Press E", "Hold F", ...)
 *   - PromptRoot           : container shown / hidden based on prompt presence
 *
 * The widget binds itself to the player's USFPlayerHUDWidgetController in
 * NativeConstruct (and again whenever SetPlayerHUDWidgetController fires)
 * and reflects OnInteractionPromptChanged / OnInteractionOptionsChanged
 * with no Blueprint scripting required.
 *
 * If the prompt root binding is absent the widget falls back to setting
 * its own visibility, so designers can also just place the widget in the
 * HUD with no BindWidget at all.
 */
UCLASS()
class SIGNALFORGERPG_API USFInteractionPromptWidget : public USFUserWidgetBase
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	/**
	 * Visual style used when the prompt option is currently available
	 * (player is in range, fact-gates pass, etc.).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction Prompt")
	FLinearColor AvailableColor = FLinearColor::White;

	/**
	 * Visual style used when the option is surfaced but not yet available
	 * (e.g. OutOfRange while bShowPromptWhenUnavailable=true). Defaults to
	 * a translucent grey so designers get a ghost prompt for free.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction Prompt")
	FLinearColor UnavailableColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.4f);

	/**
	 * If true, hide the prompt entirely when the option is not Available.
	 * Default is false so soft prompts (OutOfRange, etc.) still render in
	 * the unavailable color.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction Prompt")
	bool bHideWhenUnavailable = false;

	/** Main prompt text block ("Talk"). */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Interaction Prompt")
	TObjectPtr<UTextBlock> PromptTextBlock;

	/** Optional sub-prompt text block ("Press E"). */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Interaction Prompt")
	TObjectPtr<UTextBlock> SubPromptTextBlock;

	/**
	 * Optional container that wraps the prompt visuals. When bound, the
	 * widget toggles its visibility instead of toggling the root widget,
	 * which lets designers keep the widget itself always-visible (for
	 * animation hooks) while only the inner panel collapses.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Interaction Prompt")
	TObjectPtr<UPanelWidget> PromptRoot;

protected:
	UFUNCTION()
	void HandleInteractionPromptChanged(bool bHasPrompt, FText PromptText);

	UFUNCTION()
	void HandleInteractionOptionsChanged(
		bool bHasInteractable,
		FSFInteractionOption PrimaryOption,
		const TArray<FSFInteractionOption>& AllOptions);

	/** Called from NativeConstruct + when the controller is set later. */
	void BindToController();
	void UnbindFromController();

	void ApplyPromptState(bool bHasPrompt, const FText& PromptText, const FText& SubPromptText, bool bAvailable);

	/** Cached so we can unbind cleanly on destruct or controller swap. */
	UPROPERTY(Transient)
	TObjectPtr<class USFPlayerHUDWidgetController> BoundController;

	/** Re-bind hook fired by USFUserWidgetBase whenever the controller changes. */
	virtual void NativeOnPlayerHUDWidgetControllerSet() override;
};
