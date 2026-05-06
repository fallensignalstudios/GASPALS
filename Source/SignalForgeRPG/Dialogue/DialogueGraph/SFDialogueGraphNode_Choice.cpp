#include "Dialogue/DialogueGraph/SFDialogueGraphNode_Choice.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraph.h"
#include "UObject/UnrealType.h"

void USFDialogueGraphNode_Choice::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, TEXT("MultipleNodes"), TEXT("In"));

	if (Choices.Num() == 0)
	{
		CreatePin(EGPD_Output, TEXT("MultipleNodes"), TEXT("Choice_0"));
		return;
	}

	for (int32 Index = 0; Index < Choices.Num(); ++Index)
	{
		CreatePin(EGPD_Output, TEXT("MultipleNodes"), *FString::Printf(TEXT("Choice_%d"), Index));
	}
}

void USFDialogueGraphNode_Choice::RebuildChoiceOutputPins()
{
	TArray<UEdGraphPin*> OldPins = Pins;
	Pins.Empty();

	AllocateDefaultPins();

	for (UEdGraphPin* OldPin : OldPins)
	{
		if (OldPin)
		{
			OldPin->BreakAllPinLinks();
		}
	}

	if (UEdGraph* Graph = GetGraph())
	{
		Graph->NotifyGraphChanged();
	}
}

FText USFDialogueGraphNode_Choice::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(FString::Printf(TEXT("Choice (%d)"), Choices.Num()));
}

FLinearColor USFDialogueGraphNode_Choice::GetNodeTitleColor() const
{
	return FLinearColor(0.7f, 0.5f, 0.1f);
}

void USFDialogueGraphNode_Choice::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(USFDialogueGraphNode_Choice, Choices))
	{
		RebuildChoiceOutputPins();
	}
}