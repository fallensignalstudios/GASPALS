#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Dialogue/DialogueGraph/SFDialogueGraphNode_Base.h"
#include "SFDialogueGraphNode_Event.generated.h"

UCLASS()
class SIGNALFORGERPG_API USFDialogueGraphNode_Event : public USFDialogueGraphNode_Base
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FGameplayTag EventTag;

	virtual ESFDialogueNodeType GetRuntimeNodeType() const override { return ESFDialogueNodeType::Event; }
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
};