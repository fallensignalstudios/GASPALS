#pragma once

#include "CoreMinimal.h"
#include "Dialogue/DialogueGraph/SFDialogueGraphNode_Base.h"
#include "SFDialogueGraphNode_End.generated.h"

UCLASS()
class SIGNALFORGERPG_API USFDialogueGraphNode_End : public USFDialogueGraphNode_Base
{
	GENERATED_BODY()

public:
	virtual ESFDialogueNodeType GetRuntimeNodeType() const override { return ESFDialogueNodeType::End; }
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
};