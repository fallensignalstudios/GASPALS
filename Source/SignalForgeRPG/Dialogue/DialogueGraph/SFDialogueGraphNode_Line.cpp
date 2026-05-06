#include "Dialogue/DialogueGraph/SFDialogueGraphNode_Line.h"

FText USFDialogueGraphNode_Line::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FString Speaker = SpeakerId != NAME_None ? SpeakerId.ToString() : TEXT("Line");

	FString Preview = LineText.ToString().Left(40);
	if (LineText.ToString().Len() > 40)
	{
		Preview += TEXT("...");
	}

	return FText::FromString(FString::Printf(TEXT("%s: %s"), *Speaker, *Preview));
}

FLinearColor USFDialogueGraphNode_Line::GetNodeTitleColor() const
{
	return FLinearColor(0.1f, 0.3f, 0.7f);
}