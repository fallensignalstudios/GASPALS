//#include "SFDialogueSession.h"
//
//#include "GameFramework/PlayerState.h"
//#include "SFDialogueDefinition.h"
//#include "SFNarrativeComponent.h"
//
//bool USFDialogueSession::Initialize(USFNarrativeComponent* InOwner, const USFDialogueDefinition* InDefinition, const FSFDialogueStartParams& Params)
//{
//    OwnerComponent = InOwner;
//    Definition = InDefinition;
//    StartParams = Params;
//    CurrentChunk = FSFDialogueChunk();
//    CurrentNodeId = NAME_None;
//    bIsActive = false;
//    LastExitReason = ESFDialogueExitReason::Completed;
//
//    if (!OwnerComponent || !Definition)
//    {
//        return false;
//    }
//
//    const FName DesiredStartNodeId = !Params.StartNodeId.IsNone() ? Params.StartNodeId : Definition->RootNodeId;
//    if (DesiredStartNodeId.IsNone())
//    {
//        return false;
//    }
//
//    return RebuildChunkFromNode(DesiredStartNodeId);
//}
//
//bool USFDialogueSession::Play()
//{
//    if (!Definition || CurrentNodeId.IsNone())
//    {
//        return false;
//    }
//
//    bIsActive = true;
//    return true;
//}
//
//bool USFDialogueSession::IsActive() const
//{
//    return bIsActive;
//}
//
//bool USFDialogueSession::CanSelectOption(FName OptionId) const
//{
//    return bIsActive && !OptionId.IsNone() && CurrentChunk.PlayerReplyIds.Contains(OptionId);
//}
//
//bool USFDialogueSession::SelectOption(FName OptionId, APlayerState* Selector)
//{
//    if (!CanSelectOption(OptionId))
//    {
//        return false;
//    }
//
//    const FSFDialogueNodeDefinition* SelectedNode = FindNodeDefinition(OptionId);
//    if (!SelectedNode || !SelectedNode->bPlayerResponse)
//    {
//        return false;
//    }
//
//    CurrentNodeId = SelectedNode->NodeId;
//    CurrentChunk = FSFDialogueChunk();
//    CurrentChunk.CurrentSpeakerId = SelectedNode->SpeakerId;
//    CurrentChunk.PlayerReplyIds.Add(SelectedNode->NodeId);
//
//    return AdvanceFromNode(*SelectedNode);
//}
//
//bool USFDialogueSession::CanSkipCurrentLine() const
//{
//    return bIsActive && CurrentChunk.NPCNodeIds.Num() > 0 && !CurrentNodeId.IsNone();
//}
//
//bool USFDialogueSession::SkipCurrentLine()
//{
//    if (!CanSkipCurrentLine())
//    {
//        return false;
//    }
//
//    const FSFDialogueNodeDefinition* CurrentNode = FindNodeDefinition(CurrentNodeId);
//    if (!CurrentNode || CurrentNode->bPlayerResponse)
//    {
//        return false;
//    }
//
//    return AdvanceFromNode(*CurrentNode);
//}
//
//bool USFDialogueSession::CanExit() const
//{
//    return bIsActive;
//}
//
//void USFDialogueSession::Exit(ESFDialogueExitReason Reason)
//{
//    LastExitReason = Reason;
//    bIsActive = false;
//    CurrentNodeId = NAME_None;
//    CurrentChunk = FSFDialogueChunk();
//}
//
//ESFDialogueExitReason USFDialogueSession::GetLastExitReason() const
//{
//    return LastExitReason;
//}
//
//const FSFDialogueChunk& USFDialogueSession::GetCurrentChunk() const
//{
//    return CurrentChunk;
//}
//
//const USFDialogueDefinition* USFDialogueSession::GetDefinition() const
//{
//    return Definition;
//}
//
//const FSFDialogueNodeDefinition* USFDialogueSession::FindNodeDefinition(FName NodeId) const
//{
//    if (!Definition || NodeId.IsNone())
//    {
//        return nullptr;
//    }
//
//    return Definition->Nodes.FindByPredicate(
//        [NodeId](const FSFDialogueNodeDefinition& Node)
//        {
//            return Node.NodeId == NodeId;
//        });
//}
//
//bool USFDialogueSession::RebuildChunkFromNode(FName NodeId)
//{
//    const FSFDialogueNodeDefinition* NodeDef = FindNodeDefinition(NodeId);
//    if (!NodeDef)
//    {
//        return false;
//    }
//
//    CurrentNodeId = NodeDef->NodeId;
//    CurrentChunk = FSFDialogueChunk();
//    CurrentChunk.CurrentSpeakerId = NodeDef->SpeakerId;
//
//    if (NodeDef->bPlayerResponse)
//    {
//        CurrentChunk.PlayerReplyIds.Add(NodeDef->NodeId);
//    }
//    else
//    {
//        CurrentChunk.NPCNodeIds.Add(NodeDef->NodeId);
//    }
//
//    for (const FName& NextNodeId : NodeDef->NextNodeIds)
//    {
//        const FSFDialogueNodeDefinition* NextNode = FindNodeDefinition(NextNodeId);
//        if (!NextNode)
//        {
//            continue;
//        }
//
//        if (NextNode->bPlayerResponse)
//        {
//            CurrentChunk.PlayerReplyIds.AddUnique(NextNodeId);
//        }
//        else
//        {
//            CurrentChunk.NPCNodeIds.AddUnique(NextNodeId);
//        }
//    }
//
//    return true;
//}
//
//bool USFDialogueSession::AdvanceFromNode(const FSFDialogueNodeDefinition& NodeDef)
//{
//    if (NodeDef.NextNodeIds.Num() == 0)
//    {
//        Exit(ESFDialogueExitReason::Completed);
//        return true;
//    }
//
//    for (const FName& NextNodeId : NodeDef.NextNodeIds)
//    {
//        if (RebuildChunkFromNode(NextNodeId))
//        {
//            return true;
//        }
//    }
//
//    Exit(ESFDialogueExitReason::NetworkDesync);
//    return false;
//}