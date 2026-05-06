#pragma once

#include "CoreMinimal.h"
#include "Dialogue/DialogueGraph/SFDialogueGraphNode_Base.h"
#include "SFDialogueGraphNode_Line.generated.h"

UCLASS()
class SIGNALFORGERPG_API USFDialogueGraphNode_Line : public USFDialogueGraphNode_Base
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FText LineText;

	virtual ESFDialogueNodeType GetRuntimeNodeType() const override { return ESFDialogueNodeType::Line; }
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
};