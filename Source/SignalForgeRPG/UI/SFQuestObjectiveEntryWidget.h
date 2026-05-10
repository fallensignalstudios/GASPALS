// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/SFUserWidgetBase.h"
#include "Narrative/SFQuestDisplayTypes.h"
#include "SFQuestObjectiveEntryWidget.generated.h"

class UTextBlock;
class UProgressBar;
class UImage;

/**
 * Per-objective row inside a quest entry. Shows the objective text, an
 * optional progress bar (when RequiredQuantity > 1), and a checkmark when
 * complete.
 */
UCLASS(Blueprintable)
class SIGNALFORGERPG_API USFQuestObjectiveEntryWidget : public USFUserWidgetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI")
	void SetObjective(const FSFQuestObjectiveDisplayEntry& InObjective);

	UFUNCTION(BlueprintPure, Category = "Narrative|Quest|UI")
	const FSFQuestObjectiveDisplayEntry& GetObjective() const { return CurrentObjective; }

protected:
	virtual void NativeConstruct() override;

	// --- BindWidgetOptional bindings (designer-friendly) ---

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ObjectiveText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ProgressText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ProgressBar = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UImage> CompletionCheckImage = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	FSFQuestObjectiveDisplayEntry CurrentObjective;

	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI")
	virtual void RefreshVisuals();

	/** Designer hook fired after each SetObjective call. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Narrative|Quest|UI")
	void BP_OnObjectiveSet(const FSFQuestObjectiveDisplayEntry& InObjective);
};
