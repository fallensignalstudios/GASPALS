#pragma once

#include "CoreMinimal.h"
#include "UI/SFUserWidgetBase.h"
#include "Dialogue/Data/SFDialogueTypes.h"
#include "SFDialoguePanelWidget.generated.h"

class APlayerController;
class APawn;
class UPanelWidget;
class UTextBlock;
class URichTextBlock;
class USFDialogueChoiceButtonWidget;
class USFDialogueComponent;
class USFDialogueWidgetController;

/**
 * Native parent for WBP_DialoguePanel.
 *
 * Owns the wiring between the dialogue runtime (USFDialogueComponent /
 * USFDialogueWidgetController) and the on-screen widgets, so the panel works
 * the same whether the Blueprint is empty or richly designed.
 *
 * Required bound widgets in the BP:
 *   - ChoicesContainer (UPanelWidget — typically a VerticalBox)
 * Optional bound widgets:
 *   - SpeakerNameText (UTextBlock)
 *   - LineText (UTextBlock OR LineRichText URichTextBlock)
 *   - LineRichText (URichTextBlock)
 *
 * Choice buttons are spawned from ChoiceButtonClass (a
 * USFDialogueChoiceButtonWidget subclass), so set that in the BP defaults
 * to your WBP_DialogueChoiceButton class.
 */
UCLASS(Abstract, Blueprintable)
class SIGNALFORGERPG_API USFDialoguePanelWidget : public USFUserWidgetBase
{
	GENERATED_BODY()

public:
	/** Manually set the source dialogue component (e.g. companion conversation). */
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Panel")
	void SetDialogueComponent(USFDialogueComponent* InComponent);

	/** Looks up the local player pawn's USFDialogueComponent and binds to it. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Panel")
	void BindToLocalPlayerDialogueComponent();

	/** Forwarded to the dialogue runtime. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue|Panel")
	void AdvanceDialogue();

	UFUNCTION(BlueprintCallable, Category = "Dialogue|Panel")
	bool SelectChoice(int32 ChoiceIndex);

	UFUNCTION(BlueprintCallable, Category = "Dialogue|Panel")
	void EndDialogue();

	/** Blueprint hooks for visual polish (typewriter, fade in/out, etc.). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue|Panel")
	void OnDialogueOpened();

	UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue|Panel")
	void OnDialogueClosed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue|Panel")
	void OnDisplayDataChanged(const FSFDialogueDisplayData& DisplayData);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// === Bound widgets ===

	/** Container that will be populated with choice button instances. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> ChoicesContainer = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SpeakerNameText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LineText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<URichTextBlock> LineRichText = nullptr;

	// === Config ===

	/** Choice button class spawned into ChoicesContainer. Set in the BP. */
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue|Panel")
	TSubclassOf<USFDialogueChoiceButtonWidget> ChoiceButtonClass;

	/** If true, the panel auto-binds to the local player's dialogue component on construct. */
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue|Panel")
	bool bAutoBindOnConstruct = true;

	/** If true, the panel hides itself when no conversation is active. */
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue|Panel")
	bool bAutoHideWhenInactive = true;

	// === State ===

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue|Panel")
	TObjectPtr<USFDialogueComponent> BoundDialogueComponent = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USFDialogueChoiceButtonWidget>> SpawnedChoiceButtons;

	// === Runtime handlers ===

	UFUNCTION()
	void HandleConversationStarted(AActor* SourceActor);

	UFUNCTION()
	void HandleConversationEnded();

	UFUNCTION()
	void HandleDialogueNodeUpdated(const FSFDialogueDisplayData& DisplayData);

	UFUNCTION()
	void HandleChoiceButtonClicked(int32 ChoiceIndex);

	void BindToComponent(USFDialogueComponent* InComponent);
	void UnbindFromComponent(USFDialogueComponent* InComponent);

	void RebuildChoices(const TArray<FSFDialogueChoice>& Choices);
	void ApplyDisplayData(const FSFDialogueDisplayData& DisplayData);
	void RefreshVisibility(bool bDialogueActive);
};
