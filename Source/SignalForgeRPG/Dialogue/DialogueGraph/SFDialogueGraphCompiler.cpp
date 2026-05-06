#include "Dialogue/DialogueGraph/SFDialogueGraphCompiler.h"

#include "Dialogue/DialogueGraph/SFDialogueGraph.h"
#include "Dialogue/DialogueGraph/SFDialogueEdGraph.h"
#include "Dialogue/DialogueGraph/SFDialogueGraphNode_Base.h"
#include "Dialogue/DialogueGraph/SFDialogueGraphNode_Start.h"
#include "Dialogue/DialogueGraph/SFDialogueGraphNode_Line.h"
#include "Dialogue/DialogueGraph/SFDialogueGraphNode_Choice.h"
#include "Dialogue/DialogueGraph/SFDialogueGraphNode_Event.h"
#include "Dialogue/DialogueGraph/SFDialogueGraphNode_End.h"

#include "Dialogue/Data/SFConversationDataAsset.h"
#include "EdGraph/EdGraphPin.h"

bool FSFDialogueGraphCompiler::Compile(USFDialogueGraph* GraphAsset, TArray<FString>& OutErrors, TArray<FString>& OutWarnings)
{
	OutErrors.Reset();
	OutWarnings.Reset();

	if (!GraphAsset)
	{
		OutErrors.Add(TEXT("GraphAsset was null."));
		return false;
	}

	if (!GraphAsset->EdGraph)
	{
		OutErrors.Add(TEXT("GraphAsset has no EdGraph."));
		return false;
	}

	if (!GraphAsset->CompiledConversation)
	{
		OutErrors.Add(TEXT("GraphAsset has no CompiledConversation asset assigned."));
		return false;
	}

	USFDialogueGraphNode_Start* StartNode = FindStartNode(GraphAsset, OutErrors);
	if (!StartNode)
	{
		return false;
	}

	USFConversationDataAsset* RuntimeAsset = GraphAsset->CompiledConversation;
	RuntimeAsset->ConversationId = GraphAsset->ConversationId;
	RuntimeAsset->Nodes.Reset();
	RuntimeAsset->EntryNodeId = NAME_None;

	USFDialogueGraphNode_Base* EntryNode = GetLinkedNodeFromPin(StartNode->GetOutputPin());
	if (!EntryNode)
	{
		OutErrors.Add(TEXT("Start node is not connected to an entry dialogue node."));
		return false;
	}

	RuntimeAsset->EntryNodeId = EntryNode->GetRuntimeNodeId();

	for (UEdGraphNode* RawNode : GraphAsset->EdGraph->Nodes)
	{
		USFDialogueGraphNode_Base* DialogueNode = Cast<USFDialogueGraphNode_Base>(RawNode);
		if (!DialogueNode || Cast<USFDialogueGraphNode_Start>(DialogueNode))
		{
			continue;
		}

		FSFDialogueNode RuntimeNode;
		if (BuildRuntimeNode(DialogueNode, RuntimeNode, OutErrors, OutWarnings))
		{
			RuntimeAsset->Nodes.Add(RuntimeNode);
		}
	}

	TArray<FString> ValidationErrors;
	TArray<FString> ValidationWarnings;
	const bool bRuntimeValid = RuntimeAsset->ValidateConversation(ValidationErrors, ValidationWarnings);

	OutErrors.Append(ValidationErrors);
	OutWarnings.Append(ValidationWarnings);

	RuntimeAsset->MarkPackageDirty();

	return bRuntimeValid && OutErrors.Num() == 0;
}

USFDialogueGraphNode_Start* FSFDialogueGraphCompiler::FindStartNode(USFDialogueGraph* GraphAsset, TArray<FString>& OutErrors)
{
	USFDialogueGraphNode_Start* FoundStart = nullptr;

	for (UEdGraphNode* RawNode : GraphAsset->EdGraph->Nodes)
	{
		if (USFDialogueGraphNode_Start* StartNode = Cast<USFDialogueGraphNode_Start>(RawNode))
		{
			if (FoundStart)
			{
				OutErrors.Add(TEXT("Multiple Start nodes found."));
				return nullptr;
			}

			FoundStart = StartNode;
		}
	}

	if (!FoundStart)
	{
		OutErrors.Add(TEXT("No Start node found."));
	}

	return FoundStart;
}

USFDialogueGraphNode_Base* FSFDialogueGraphCompiler::GetLinkedNodeFromPin(const UEdGraphPin* Pin)
{
	if (!Pin || Pin->LinkedTo.Num() == 0)
	{
		return nullptr;
	}

	return Cast<USFDialogueGraphNode_Base>(Pin->LinkedTo[0]->GetOwningNode());
}

bool FSFDialogueGraphCompiler::BuildRuntimeNode(
	USFDialogueGraphNode_Base* SourceNode,
	FSFDialogueNode& OutRuntimeNode,
	TArray<FString>& OutErrors,
	TArray<FString>& OutWarnings)
{
	if (!SourceNode)
	{
		OutErrors.Add(TEXT("BuildRuntimeNode received null SourceNode."));
		return false;
	}

	OutRuntimeNode = FSFDialogueNode{};
	OutRuntimeNode.NodeId = SourceNode->GetRuntimeNodeId();
	OutRuntimeNode.NodeType = SourceNode->GetRuntimeNodeType();
	OutRuntimeNode.SpeakerId = SourceNode->SpeakerId;
	OutRuntimeNode.RequiredTags = SourceNode->RequiredTags;
	OutRuntimeNode.BlockedTags = SourceNode->BlockedTags;
	OutRuntimeNode.CameraShot = SourceNode->CameraShot;
	OutRuntimeNode.AdvanceMode = SourceNode->AdvanceMode;
	OutRuntimeNode.AdvanceDelaySeconds = SourceNode->AdvanceDelaySeconds;
	OutRuntimeNode.WordsPerMinute = SourceNode->WordsPerMinute;
	OutRuntimeNode.VoiceClip = SourceNode->VoiceClip;

	if (OutRuntimeNode.NodeId.IsNone())
	{
		OutErrors.Add(FString::Printf(TEXT("Node '%s' has an invalid runtime node id."), *SourceNode->GetName()));
		return false;
	}

	if (USFDialogueGraphNode_Line* LineNode = Cast<USFDialogueGraphNode_Line>(SourceNode))
	{
		OutRuntimeNode.LineText = LineNode->LineText;

		if (USFDialogueGraphNode_Base* NextNode = GetLinkedNodeFromPin(LineNode->GetOutputPin()))
		{
			OutRuntimeNode.NextNodeId = NextNode->GetRuntimeNodeId();
		}
	}
	else if (USFDialogueGraphNode_Event* EventNode = Cast<USFDialogueGraphNode_Event>(SourceNode))
	{
		OutRuntimeNode.EventTag = EventNode->EventTag;

		if (USFDialogueGraphNode_Base* NextNode = GetLinkedNodeFromPin(EventNode->GetOutputPin()))
		{
			OutRuntimeNode.NextNodeId = NextNode->GetRuntimeNodeId();
		}
	}
	else if (USFDialogueGraphNode_Choice* ChoiceNode = Cast<USFDialogueGraphNode_Choice>(SourceNode))
	{
		for (int32 Index = 0; Index < ChoiceNode->Choices.Num(); ++Index)
		{
			FSFDialogueChoice RuntimeChoice;
			RuntimeChoice.ChoiceText = ChoiceNode->Choices[Index].ChoiceText;
			RuntimeChoice.RequiredTags = ChoiceNode->Choices[Index].RequiredTags;
			RuntimeChoice.BlockedTags = ChoiceNode->Choices[Index].BlockedTags;

			const int32 OutputPinIndex = Index + 1;
			if (ChoiceNode->Pins.IsValidIndex(OutputPinIndex))
			{
				if (USFDialogueGraphNode_Base* ChoiceTarget = GetLinkedNodeFromPin(ChoiceNode->Pins[OutputPinIndex]))
				{
					RuntimeChoice.NextNodeId = ChoiceTarget->GetRuntimeNodeId();
				}
			}

			OutRuntimeNode.Choices.Add(RuntimeChoice);
		}
	}
	else if (Cast<USFDialogueGraphNode_End>(SourceNode))
	{
	}
	else
	{
		OutWarnings.Add(FString::Printf(TEXT("Unhandled graph node type for node '%s'."), *SourceNode->GetName()));
	}

	return true;
}