#include "Dialogue/Data/SFConversationDataAsset.h"

DEFINE_LOG_CATEGORY_STATIC(LogSFConversationAsset, Log, All);

const FSFDialogueNode* USFConversationDataAsset::FindNodeById(FName NodeId) const
{
	if (NodeId == NAME_None)
	{
		return nullptr;
	}

	for (const FSFDialogueNode& Node : Nodes)
	{
		if (Node.NodeId == NodeId)
		{
			return &Node;
		}
	}

	return nullptr;
}

bool USFConversationDataAsset::IsValidConversation() const
{
	TArray<FString> Errors;
	TArray<FString> Warnings;
	return ValidateConversation(Errors, Warnings);
}

bool USFConversationDataAsset::ValidateConversation(TArray<FString>& OutErrors, TArray<FString>& OutWarnings) const
{
	OutErrors.Reset();
	OutWarnings.Reset();

	if (Nodes.IsEmpty())
	{
		OutErrors.Add(TEXT("Conversation has no nodes."));
		return false;
	}

	if (EntryNodeId == NAME_None)
	{
		OutErrors.Add(TEXT("Conversation EntryNodeId is None."));
	}
	else if (!FindNodeById(EntryNodeId))
	{
		OutErrors.Add(FString::Printf(TEXT("Entry node '%s' was not found in the conversation."), *EntryNodeId.ToString()));
	}

	const bool bNodeIdsValid = ValidateNodeIds(OutErrors);
	const bool bLinksValid = ValidateNodeLinks(OutErrors, OutWarnings);

	return OutErrors.Num() == 0 && bNodeIdsValid && bLinksValid;
}

bool USFConversationDataAsset::ValidateNodeIds(TArray<FString>& OutErrors) const
{
	bool bIsValid = true;
	TSet<FName> SeenNodeIds;

	for (int32 NodeIndex = 0; NodeIndex < Nodes.Num(); ++NodeIndex)
	{
		const FSFDialogueNode& Node = Nodes[NodeIndex];

		if (Node.NodeId == NAME_None)
		{
			OutErrors.Add(FString::Printf(TEXT("Node at index %d has an invalid NodeId of None."), NodeIndex));
			bIsValid = false;
			continue;
		}

		if (SeenNodeIds.Contains(Node.NodeId))
		{
			OutErrors.Add(FString::Printf(TEXT("Duplicate NodeId detected: '%s'."), *Node.NodeId.ToString()));
			bIsValid = false;
			continue;
		}

		SeenNodeIds.Add(Node.NodeId);
	}

	return bIsValid;
}

bool USFConversationDataAsset::ValidateNodeLinks(TArray<FString>& OutErrors, TArray<FString>& OutWarnings) const
{
	bool bIsValid = true;

	for (const FSFDialogueNode& Node : Nodes)
	{
		switch (Node.NodeType)
		{
		case ESFDialogueNodeType::Line:
		{
			if (Node.LineText.IsEmpty())
			{
				OutWarnings.Add(FString::Printf(TEXT("Line node '%s' has empty LineText."), *Node.NodeId.ToString()));
			}

			if (Node.NextNodeId != NAME_None && !FindNodeById(Node.NextNodeId))
			{
				OutErrors.Add(FString::Printf(
					TEXT("Line node '%s' points to missing NextNodeId '%s'."),
					*Node.NodeId.ToString(),
					*Node.NextNodeId.ToString()
				));
				bIsValid = false;
			}
			break;
		}

		case ESFDialogueNodeType::Choice:
		{
			if (Node.Choices.Num() == 0)
			{
				OutErrors.Add(FString::Printf(TEXT("Choice node '%s' has no choices."), *Node.NodeId.ToString()));
				bIsValid = false;
				break;
			}

			int32 ValidChoiceCount = 0;

			for (int32 ChoiceIndex = 0; ChoiceIndex < Node.Choices.Num(); ++ChoiceIndex)
			{
				const FSFDialogueChoice& Choice = Node.Choices[ChoiceIndex];

				if (Choice.ChoiceText.IsEmpty())
				{
					OutWarnings.Add(FString::Printf(
						TEXT("Choice node '%s' has a choice at index %d with empty ChoiceText."),
						*Node.NodeId.ToString(),
						ChoiceIndex
					));
				}

				if (Choice.NextNodeId == NAME_None)
				{
					OutWarnings.Add(FString::Printf(
						TEXT("Choice node '%s' has a choice at index %d with no NextNodeId. This will end the conversation if selected."),
						*Node.NodeId.ToString(),
						ChoiceIndex
					));
					++ValidChoiceCount;
					continue;
				}

				if (!FindNodeById(Choice.NextNodeId))
				{
					OutErrors.Add(FString::Printf(
						TEXT("Choice node '%s' has a choice at index %d pointing to missing NextNodeId '%s'."),
						*Node.NodeId.ToString(),
						ChoiceIndex,
						*Choice.NextNodeId.ToString()
					));
					bIsValid = false;
					continue;
				}

				++ValidChoiceCount;
			}

			if (ValidChoiceCount == 0)
			{
				OutErrors.Add(FString::Printf(
					TEXT("Choice node '%s' has no valid destination choices."),
					*Node.NodeId.ToString()
				));
				bIsValid = false;
			}
			break;
		}

		case ESFDialogueNodeType::Event:
		{
			if (!Node.EventTag.IsValid())
			{
				OutWarnings.Add(FString::Printf(
					TEXT("Event node '%s' has no valid EventTag."),
					*Node.NodeId.ToString()
				));
			}

			if (Node.NextNodeId == NAME_None)
			{
				OutWarnings.Add(FString::Printf(
					TEXT("Event node '%s' has no NextNodeId and will end the conversation after firing."),
					*Node.NodeId.ToString()
				));
			}
			else if (!FindNodeById(Node.NextNodeId))
			{
				OutErrors.Add(FString::Printf(
					TEXT("Event node '%s' points to missing NextNodeId '%s'."),
					*Node.NodeId.ToString(),
					*Node.NextNodeId.ToString()
				));
				bIsValid = false;
			}
			break;
		}

		case ESFDialogueNodeType::End:
		{
			if (!Node.LineText.IsEmpty())
			{
				OutWarnings.Add(FString::Printf(
					TEXT("End node '%s' has LineText set. End nodes currently terminate immediately and will not display this text."),
					*Node.NodeId.ToString()
				));
			}

			if (Node.NextNodeId != NAME_None)
			{
				OutWarnings.Add(FString::Printf(
					TEXT("End node '%s' has NextNodeId '%s', but End nodes terminate immediately."),
					*Node.NodeId.ToString(),
					*Node.NextNodeId.ToString()
				));
			}
			break;
		}

		default:
			OutErrors.Add(FString::Printf(
				TEXT("Node '%s' has an unknown or unsupported node type."),
				*Node.NodeId.ToString()
			));
			bIsValid = false;
			break;
		}
	}

	return bIsValid;
}