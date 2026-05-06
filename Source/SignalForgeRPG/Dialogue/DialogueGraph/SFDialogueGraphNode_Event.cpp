#include "Dialogue/DialogueGraph/SFDialogueGraphNode_Event.h"

FText USFDialogueGraphNode_Event::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (EventTag.IsValid())
	{
		return FText::FromString(FString::Printf(TEXT("Event: %s"), *EventTag.GetTagName().ToString()));
	}

	return FText::FromString(TEXT("Event"));
}

FLinearColor USFDialogueGraphNode_Event::GetNodeTitleColor() const
{
	return FLinearColor(0.5f, 0.2f, 0.7f);
}