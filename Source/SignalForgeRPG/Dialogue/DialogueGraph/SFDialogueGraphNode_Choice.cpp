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
	// Snapshot the existing pin pointers so we can destroy them after the new
	// pin set is allocated. Just clearing the Pins array would orphan the old
	// UEdGraphPin* objects (pin leak) and leave dangling LinkedTo entries on
	// any nodes they were connected to.
	TArray<UEdGraphPin*> OldPins = Pins;
	Pins.Reset();

	AllocateDefaultPins();

	for (UEdGraphPin* OldPin : OldPins)
	{
		if (OldPin)
		{
			OldPin->BreakAllPinLinks();
			OldPin->MarkAsGarbage();
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

	// GetMemberPropertyName resolves to the outer property the user edited,
	// even when the change happened inside an inner struct field (e.g. typing
	// into Choices[0].ChoiceText). GetPropertyName alone would miss those.
	const FName MemberName = PropertyChangedEvent.GetMemberPropertyName();
	if (MemberName != GET_MEMBER_NAME_CHECKED(USFDialogueGraphNode_Choice, Choices))
	{
		return;
	}

	// Only structural changes to the Choices array require rebuilding output
	// pins — editing a choice's text or tags does not change the pin count.
	const EPropertyChangeType::Type ChangeType = PropertyChangedEvent.ChangeType;
	const bool bStructuralChange =
		ChangeType == EPropertyChangeType::ArrayAdd ||
		ChangeType == EPropertyChangeType::ArrayRemove ||
		ChangeType == EPropertyChangeType::ArrayClear ||
		ChangeType == EPropertyChangeType::ArrayMove ||
		ChangeType == EPropertyChangeType::Duplicate ||
		ChangeType == EPropertyChangeType::ValueSet;

	if (bStructuralChange)
	{
		RebuildChoiceOutputPins();
	}
}
