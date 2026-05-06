#include "Dialogue/DialogueGraph/SFDialogueGraph.h"

#include "Dialogue/DialogueGraph/SFDialogueEdGraph.h"
#include "Dialogue/DialogueGraph/SFDialogueGraphNode_Start.h"
#include "Dialogue/DialogueGraph/SFDialogueGraphCompiler.h"

#include "EdGraph/EdGraphNode.h"
#include "UObject/ObjectSaveContext.h"

USFDialogueGraph::USFDialogueGraph()
{
}

void USFDialogueGraph::EnsureGraphInitialized()
{
	if (!EdGraph)
	{
		EdGraph = NewObject<USFDialogueEdGraph>(
			this,
			USFDialogueEdGraph::StaticClass(),
			TEXT("DialogueEdGraph"),
			RF_Transactional
		);
	}

	bool bHasStartNode = false;
	for (UEdGraphNode* Node : EdGraph->Nodes)
	{
		if (Cast<USFDialogueGraphNode_Start>(Node))
		{
			bHasStartNode = true;
			break;
		}
	}

	if (!bHasStartNode)
	{
		EdGraph->Modify();

		USFDialogueGraphNode_Start* StartNode = NewObject<USFDialogueGraphNode_Start>(
			EdGraph,
			USFDialogueGraphNode_Start::StaticClass(),
			NAME_None,
			RF_Transactional
		);

		EdGraph->AddNode(StartNode, true, false);

		StartNode->CreateNewGuid();
		StartNode->NodePosX = 0;
		StartNode->NodePosY = 0;
		StartNode->SnapToGrid(16);
		StartNode->AllocateDefaultPins();
		StartNode->PostPlacedNewNode();
	}
}

void USFDialogueGraph::PreSave(FObjectPreSaveContext SaveContext)
{
	Super::PreSave(SaveContext);

	EnsureGraphInitialized();

	TArray<FString> Errors;
	TArray<FString> Warnings;
	FSFDialogueGraphCompiler::Compile(this, Errors, Warnings);

	for (const FString& ErrorString : Errors)
	{
		UE_LOG(LogTemp, Error, TEXT("Dialogue PreSave Compile Error: %s"), *ErrorString);
	}

	for (const FString& WarningString : Warnings)
	{
		UE_LOG(LogTemp, Warning, TEXT("Dialogue PreSave Compile Warning: %s"), *WarningString);
	}
}