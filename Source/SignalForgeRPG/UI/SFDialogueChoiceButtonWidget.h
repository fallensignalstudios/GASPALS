#pragma once

#include "CoreMinimal.h"
#include "UI/SFUserWidgetBase.h"
#include "SFDialogueChoiceButtonWidget.generated.h"

class UButton;
class UTextBlock;
class URichTextBlock;
class USizeBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSFOnDialogueChoiceClicked, int32, ChoiceIndex);

/**
 * Native parent for the dialogue choice button widget.
 *
 * Reparent WBP_DialogueChoiceButton to this class to inherit the correct
 * layout configuration. The most common cause of "text wraps after every
 * word" in choice buttons is a TextBlock with AutoWrapText=true sitting
 * inside a SizeToContent parent (HorizontalBox / VerticalBox /
 * Button content slot) with no minimum width. Slate then collapses the
 * desired size to a single word per line.
 *
 * This widget enforces:
 *   - PromptText AutoWrapText=true with a sane MinDesiredWidth.
 *   - WrapTextAt=0 (let layout decide) and WrappingPolicy=DefaultWrapping
 *     so per-character wrapping is disabled.
 *   - HorizontalAlignment=Fill on the text block in its parent slot, via
 *     a SizeBox wrapper if MinChoiceWidth is set.
 *
 * Either bind PromptText/ChoiceButton in the Blueprint with the exact
 * names below, OR drive everything from the BP designer and just call
 * SetChoiceText / SetChoiceIndex from outside.
 */
UCLASS(Abstract, Blueprintable)
class SIGNALFORGERPG_API USFDialogueChoiceButtonWidget : public USFUserWidgetBase
{
	GENERATED_BODY()

public:
	/** Set the displayed choice text and apply wrapping configuration. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Choice")
	void SetChoiceText(const FText& InText);

	/** Index in the visible choice array (passed back to the controller on click). */
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Choice")
	void SetChoiceIndex(int32 InIndex) { ChoiceIndex = InIndex; }

	UFUNCTION(BlueprintPure, Category = "Dialogue|Choice")
	int32 GetChoiceIndex() const { return ChoiceIndex; }

	/** Marks this choice as locked (failed tag check). UI may grey it out. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Choice")
	void SetLocked(bool bInLocked);

	UFUNCTION(BlueprintPure, Category = "Dialogue|Choice")
	bool IsLocked() const { return bLocked; }

	/** Broadcast when the button is clicked. */
	UPROPERTY(BlueprintAssignable, Category = "Dialogue|Choice")
	FSFOnDialogueChoiceClicked OnChoiceClicked;

	/** Optional Blueprint hook so designers can react to lock state changes. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue|Choice")
	void OnLockedChanged(bool bNewLocked);

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Bind these widgets in the Blueprint to wire them up automatically. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ChoiceButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PromptText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<URichTextBlock> PromptRichText = nullptr;

	/**
	 * Optional SizeBox wrapping the text. If present, we set its
	 * MinDesiredWidth to MinChoiceWidth so the button doesn't collapse
	 * narrower than this and force per-word wrapping.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> PromptSizeBox = nullptr;

	/**
	 * Minimum desired width (px) for the choice text. If the text block is
	 * wrapped in a USizeBox bound as PromptSizeBox, this value is applied to
	 * the size box. Otherwise we apply MinDesiredWidth on the text block.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue|Choice", meta = (ClampMin = "0", UIMin = "0"))
	float MinChoiceWidth = 480.0f;

	UFUNCTION()
	void HandleButtonClicked();

private:
	UPROPERTY(Transient)
	int32 ChoiceIndex = INDEX_NONE;

	UPROPERTY(Transient)
	bool bLocked = false;

	void ApplyWrappingConfig();
};
