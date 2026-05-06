#include "Dialogue/DialogueGraph/SFDialogueGraphNode_Base.h"
#include "EdGraph/EdGraphPin.h"

USFDialogueGraphNode_Base::USFDialogueGraphNode_Base()
{
}

FName USFDialogueGraphNode_Base::GetRuntimeNodeId() const
{
	if (!NodeGuidValue.IsValid())
	{
		return NAME_None;
	}

	return FName(*NodeGuidValue.ToString(EGuidFormats::Digits));
}

void USFDialogueGraphNode_Base::PostPlacedNewNode()
{
	Super::PostPlacedNewNode();

	if (!NodeGuidValue.IsValid())
	{
		NodeGuidValue = FGuid::NewGuid();
	}
}

void USFDialogueGraphNode_Base::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, TEXT("MultipleNodes"), TEXT("In"));
	CreatePin(EGPD_Output, TEXT("MultipleNodes"), TEXT("Out"));
}

FText USFDialogueGraphNode_Base::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(TEXT("Dialogue Node"));
}

FLinearColor USFDialogueGraphNode_Base::GetNodeTitleColor() const
{
	return FLinearColor(0.15f, 0.15f, 0.15f);
}

UEdGraphPin* USFDialogueGraphNode_Base::GetInputPin() const
{
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin && Pin->Direction == EGPD_Input)
		{
			return Pin;
		}
	}
	return nullptr;
}

UEdGraphPin* USFDialogueGraphNode_Base::GetOutputPin() const
{
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin && Pin->Direction == EGPD_Output)
		{
			return Pin;
		}
	}
	return nullptr;
}

void USFDialogueGraphNode_Base::PostLoad()
{
	Super::PostLoad();

	if (!NodeGuidValue.IsValid())
	{
		NodeGuidValue = FGuid::NewGuid();
	}
}