#include "SFDialogueGraphSchema.h"

#include "Dialogue/DialogueGraph/SFDialogueGraphNode_Start.h"
#include "Dialogue/DialogueGraph/SFDialogueGraphNode_Line.h"
#include "Dialogue/DialogueGraph/SFDialogueGraphNode_Choice.h"
#include "Dialogue/DialogueGraph/SFDialogueGraphNode_Event.h"
#include "Dialogue/DialogueGraph/SFDialogueGraphNode_End.h"
#include "Dialogue/DialogueGraph/SFDialogueGraphNode_Base.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "Framework/Commands/GenericCommands.h"
#include "ScopedTransaction.h"

namespace SFDialogueGraphSchemaUtils
{
	template<typename TNodeClass>
	void SpawnDialogueNode(UEdGraph* ParentGraph, const FVector2D Location, bool bSelectNewNode)
	{
		if (!ParentGraph)
		{
			return;
		}

		TNodeClass* NewNode = NewObject<TNodeClass>(
			ParentGraph,
			TNodeClass::StaticClass(),
			NAME_None,
			RF_Transactional
		);

		ParentGraph->Modify();
		ParentGraph->AddNode(NewNode, true, bSelectNewNode);

		NewNode->CreateNewGuid();
		NewNode->NodePosX = static_cast<int32>(Location.X);
		NewNode->NodePosY = static_cast<int32>(Location.Y);
		NewNode->SnapToGrid(16);
		NewNode->AllocateDefaultPins();
		NewNode->PostPlacedNewNode();
	}
}

struct FSFDialogueGraphSchemaAction_NewNode : public FEdGraphSchemaAction
{
public:
	TFunction<void(UEdGraph*, const FVector2D&, bool)> PerformActionLambda;

	FSFDialogueGraphSchemaAction_NewNode()
		: FEdGraphSchemaAction()
	{
	}

	FSFDialogueGraphSchemaAction_NewNode(
		const FText& InNodeCategory,
		const FText& InMenuDesc,
		const FText& InToolTip,
		int32 InGrouping)
		: FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InToolTip, InGrouping)
	{
	}

	virtual UEdGraphNode* PerformAction(
		UEdGraph* ParentGraph,
		UEdGraphPin* FromPin,
		const FVector2D Location,
		bool bSelectNewNode = true) override
	{
		if (!ParentGraph || !PerformActionLambda)
		{
			return nullptr;
		}

		const FScopedTransaction Transaction(NSLOCTEXT("SFDialogueGraph", "AddNode", "Add Dialogue Graph Node"));
		ParentGraph->Modify();

		PerformActionLambda(ParentGraph, Location, bSelectNewNode);

		UEdGraphNode* NewNode = ParentGraph->Nodes.Num() > 0 ? ParentGraph->Nodes.Last() : nullptr;

		if (FromPin && NewNode)
		{
			UEdGraphPin* TargetPin = nullptr;

			for (UEdGraphPin* Pin : NewNode->Pins)
			{
				if (!Pin)
				{
					continue;
				}

				if (FromPin->Direction == EGPD_Output && Pin->Direction == EGPD_Input)
				{
					TargetPin = Pin;
					break;
				}

				if (FromPin->Direction == EGPD_Input && Pin->Direction == EGPD_Output)
				{
					TargetPin = Pin;
					break;
				}
			}

			if (TargetPin)
			{
				FromPin->MakeLinkTo(TargetPin);
			}
		}

		return NewNode;
	}
};

void USFDialogueGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	bool bHasStartNode = false;

	if (ContextMenuBuilder.CurrentGraph)
	{
		for (UEdGraphNode* Node : ContextMenuBuilder.CurrentGraph->Nodes)
		{
			if (Cast<USFDialogueGraphNode_Start>(Node))
			{
				bHasStartNode = true;
				break;
			}
		}
	}
	if (!bHasStartNode)
	{
		TSharedPtr<FSFDialogueGraphSchemaAction_NewNode> Action =
			MakeShared<FSFDialogueGraphSchemaAction_NewNode>(
				FText::FromString(TEXT("Dialogue")),
				FText::FromString(TEXT("Add Start Node")),
				FText::FromString(TEXT("Creates a Start node.")),
				0);

		Action->PerformActionLambda = [](UEdGraph* Graph, const FVector2D& Location, bool bSelectNewNode)
			{
				SFDialogueGraphSchemaUtils::SpawnDialogueNode<USFDialogueGraphNode_Start>(Graph, Location, bSelectNewNode);
			};

		ContextMenuBuilder.AddAction(Action);
	}

	{
		TSharedPtr<FSFDialogueGraphSchemaAction_NewNode> Action =
			MakeShared<FSFDialogueGraphSchemaAction_NewNode>(
				FText::FromString(TEXT("Dialogue")),
				FText::FromString(TEXT("Add Line Node")),
				FText::FromString(TEXT("Creates a Line node.")),
				0);

		Action->PerformActionLambda = [](UEdGraph* Graph, const FVector2D& Location, bool bSelectNewNode)
			{
				SFDialogueGraphSchemaUtils::SpawnDialogueNode<USFDialogueGraphNode_Line>(Graph, Location, bSelectNewNode);
			};

		ContextMenuBuilder.AddAction(Action);
	}

	{
		TSharedPtr<FSFDialogueGraphSchemaAction_NewNode> Action =
			MakeShared<FSFDialogueGraphSchemaAction_NewNode>(
				FText::FromString(TEXT("Dialogue")),
				FText::FromString(TEXT("Add Choice Node")),
				FText::FromString(TEXT("Creates a Choice node.")),
				0);

		Action->PerformActionLambda = [](UEdGraph* Graph, const FVector2D& Location, bool bSelectNewNode)
			{
				SFDialogueGraphSchemaUtils::SpawnDialogueNode<USFDialogueGraphNode_Choice>(Graph, Location, bSelectNewNode);
			};

		ContextMenuBuilder.AddAction(Action);
	}

	{
		TSharedPtr<FSFDialogueGraphSchemaAction_NewNode> Action =
			MakeShared<FSFDialogueGraphSchemaAction_NewNode>(
				FText::FromString(TEXT("Dialogue")),
				FText::FromString(TEXT("Add Event Node")),
				FText::FromString(TEXT("Creates an Event node.")),
				0);

		Action->PerformActionLambda = [](UEdGraph* Graph, const FVector2D& Location, bool bSelectNewNode)
			{
				SFDialogueGraphSchemaUtils::SpawnDialogueNode<USFDialogueGraphNode_Event>(Graph, Location, bSelectNewNode);
			};

		ContextMenuBuilder.AddAction(Action);
	}

	{
		TSharedPtr<FSFDialogueGraphSchemaAction_NewNode> Action =
			MakeShared<FSFDialogueGraphSchemaAction_NewNode>(
				FText::FromString(TEXT("Dialogue")),
				FText::FromString(TEXT("Add End Node")),
				FText::FromString(TEXT("Creates an End node.")),
				0);

		Action->PerformActionLambda = [](UEdGraph* Graph, const FVector2D& Location, bool bSelectNewNode)
			{
				SFDialogueGraphSchemaUtils::SpawnDialogueNode<USFDialogueGraphNode_End>(Graph, Location, bSelectNewNode);
			};

		ContextMenuBuilder.AddAction(Action);
	}
}

const FPinConnectionResponse USFDialogueGraphSchema::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
	if (!A || !B)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Invalid pins."));
	}

	if (A == B)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Cannot connect a pin to itself."));
	}

	if (A->GetOwningNode() == B->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Cannot connect pins on the same node."));
	}

	if (A->Direction == B->Direction)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Pins must connect input to output."));
	}

	const UEdGraphPin* OutputPin = (A->Direction == EGPD_Output) ? A : B;
	const UEdGraphPin* InputPin = (A->Direction == EGPD_Input) ? A : B;

	const UEdGraphNode* OutputNode = OutputPin ? OutputPin->GetOwningNode() : nullptr;
	const UEdGraphNode* InputNode = InputPin ? InputPin->GetOwningNode() : nullptr;

	if (!OutputNode || !InputNode)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Invalid nodes."));
	}

	if (Cast<USFDialogueGraphNode_End>(OutputNode))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("End nodes cannot have outgoing connections."));
	}

	if (Cast<USFDialogueGraphNode_Start>(InputNode))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Start nodes cannot have incoming connections."));
	}

	// Only one inbound link per input pin.
	if (InputPin->LinkedTo.Num() > 0)
	{
		return FPinConnectionResponse(
			(InputPin == A) ? CONNECT_RESPONSE_BREAK_OTHERS_A : CONNECT_RESPONSE_BREAK_OTHERS_B,
			TEXT("Replacing existing input connection."));
	}

	if (OutputPin->LinkedTo.Num() > 0)
	{
		return FPinConnectionResponse(
			(OutputPin == A) ? CONNECT_RESPONSE_BREAK_OTHERS_A : CONNECT_RESPONSE_BREAK_OTHERS_B,
			TEXT("Replacing existing output connection."));
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
}

bool USFDialogueGraphSchema::ShouldHidePinDefaultValue(UEdGraphPin* Pin) const
{
	return true;
}