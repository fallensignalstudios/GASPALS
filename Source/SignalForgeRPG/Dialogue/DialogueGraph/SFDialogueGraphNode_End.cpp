#include "Dialogue/DialogueGraph/SFDialogueGraphNode_End.h"

void USFDialogueGraphNode_End::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, TEXT("MultipleNodes"), TEXT("In"));
}

FText USFDialogueGraphNode_End::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(TEXT("End"));
}

FLinearColor USFDialogueGraphNode_End::GetNodeTitleColor() const
{
	return FLinearColor(0.6f, 0.1f, 0.1f);
}