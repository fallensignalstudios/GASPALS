#pragma once

#include "CoreMinimal.h"
#include "Dialogue/DialogueGraph/SFDialogueGraphNode_Base.h"
#include "Dialogue/DialogueGraph/SFDialogueGraphChoiceEntry.h"
#include "SFDialogueGraphNode_Choice.generated.h"

UCLASS()
class SIGNALFORGERPG_API USFDialogueGraphNode_Choice : public USFDialogueGraphNode_Base
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	TArray<FSFDialogueGraphChoiceEntry> Choices;

	virtual ESFDialogueNodeType GetRuntimeNodeType() const override { return ESFDialogueNodeType::Choice; }
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;

	void RebuildChoiceOutputPins();

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
};