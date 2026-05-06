#include "Dialogue/DialogueGraph/SFDialogueGraphNode_Start.h"

void USFDialogueGraphNode_Start::AllocateDefaultPins()
{
	CreatePin(EGPD_Output, TEXT("MultipleNodes"), TEXT("Start"));
}

FText USFDialogueGraphNode_Start::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(TEXT("Start"));
}

FLinearColor USFDialogueGraphNode_Start::GetNodeTitleColor() const
{
	return FLinearColor(0.1f, 0.5f, 0.1f);
}