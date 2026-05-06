#pragma once

#include "CoreMinimal.h"
#include "Dialogue/Data/SFDialogueTypes.h"

class UEdGraphNode;
class UEdGraphPin;
class USFDialogueGraph;
class USFConversationDataAsset;
class USFDialogueGraphNode_Base;
class USFDialogueGraphNode_Start;

struct FSFDialogueCompilerMessage
{
	FString Message;
	bool bIsError = false;
	TWeakObjectPtr<UEdGraphNode> Node;

	FSFDialogueCompilerMessage() = default;

	FSFDialogueCompilerMessage(const FString& InMessage, bool bInIsError, UEdGraphNode* InNode = nullptr)
		: Message(InMessage)
		, bIsError(bInIsError)
		, Node(InNode)
	{
	}
};

class SIGNALFORGERPG_API FSFDialogueGraphCompiler
{
public:
	static bool Compile(USFDialogueGraph* GraphAsset, TArray<FString>& OutErrors, TArray<FString>& OutWarnings);

private:
	static USFDialogueGraphNode_Start* FindStartNode(USFDialogueGraph* GraphAsset, TArray<FString>& OutErrors);

	static bool BuildRuntimeNode(
		USFDialogueGraphNode_Base* SourceNode,
		FSFDialogueNode& OutRuntimeNode,
		TArray<FString>& OutErrors,
		TArray<FString>& OutWarnings);

	static USFDialogueGraphNode_Base* GetLinkedNodeFromPin(const UEdGraphPin* Pin);
};