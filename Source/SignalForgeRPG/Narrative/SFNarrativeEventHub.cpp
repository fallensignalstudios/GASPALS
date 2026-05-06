// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "SFNarrativeEventHub.h"

void USFNarrativeEventHub::BroadcastQuestStarted(USFQuestInstance* QuestInstance)
{
    OnQuestStarted.Broadcast(QuestInstance);
}

void USFNarrativeEventHub::BroadcastQuestRestarted(USFQuestInstance* QuestInstance)
{
    OnQuestRestarted.Broadcast(QuestInstance);
}

void USFNarrativeEventHub::BroadcastQuestAbandoned(USFQuestInstance* QuestInstance)
{
    OnQuestAbandoned.Broadcast(QuestInstance);
}

void USFNarrativeEventHub::BroadcastQuestStateChanged(USFQuestInstance* QuestInstance, FName StateId)
{
    OnQuestStateChanged.Broadcast(QuestInstance, StateId);
}

void USFNarrativeEventHub::BroadcastQuestTaskProgressed(USFQuestInstance* QuestInstance, FGameplayTag TaskTag, FName ContextId, int32 Quantity)
{
    OnQuestTaskProgressed.Broadcast(QuestInstance, TaskTag, ContextId, Quantity);
}

void USFNarrativeEventHub::BroadcastDialogueStarted(USFConversationDataAsset* Conversation, AActor* SourceActor)
{
    OnDialogueStarted.Broadcast(Conversation, SourceActor);
}

void USFNarrativeEventHub::BroadcastDialogueEnded(USFConversationDataAsset* Conversation, ESFDialogueExitReason ExitReason)
{
    OnDialogueEnded.Broadcast(Conversation, ExitReason);
}

void USFNarrativeEventHub::BroadcastDialogueNodeUpdated(USFConversationDataAsset* Conversation, FName NodeId, const FSFDialogueDisplayData& DisplayData)
{
    OnDialogueNodeUpdated.Broadcast(Conversation, NodeId, DisplayData);
}

void USFNarrativeEventHub::BroadcastDialogueEventTriggered(FGameplayTag EventTag)
{
    OnDialogueEventTriggered.Broadcast(EventTag);
}

void USFNarrativeEventHub::BroadcastDialogueChoiceSelected(USFConversationDataAsset* Conversation, FName NodeId, FName SpeakerId)
{
    OnDialogueChoiceSelected.Broadcast(Conversation, NodeId, SpeakerId);
}

void USFNarrativeEventHub::BroadcastNarrativeTaskCompleted(FGameplayTag TaskTag, FName ContextId, int32 Quantity)
{
    OnNarrativeTaskCompleted.Broadcast(TaskTag, ContextId, Quantity);
}

void USFNarrativeEventHub::BroadcastWorldFactChanged(FGameplayTag FactTag, FName ContextId, const FSFWorldFactValue& NewValue)
{
    OnWorldFactChanged.Broadcast(FactTag, ContextId, NewValue);
}

void USFNarrativeEventHub::BroadcastWorldFactRemoved(FGameplayTag FactTag, FName ContextId)
{
    OnWorldFactRemoved.Broadcast(FactTag, ContextId);
}

void USFNarrativeEventHub::BroadcastFactionStandingChanged(FGameplayTag FactionTag, const FSFFactionStandingValue& OldStanding, const FSFFactionStandingValue& NewStanding)
{
    OnFactionStandingChanged.Broadcast(FactionTag, OldStanding, NewStanding);
}

void USFNarrativeEventHub::BroadcastFactionStandingBandChanged(FGameplayTag FactionTag, ESFFactionStandingBand NewStandingBand)
{
    OnFactionStandingBandChanged.Broadcast(FactionTag, NewStandingBand);
}

void USFNarrativeEventHub::BroadcastIdentityAxisChanged(FGameplayTag AxisTag, float OldValue, float NewValue)
{
    OnIdentityAxisChanged.Broadcast(AxisTag, OldValue, NewValue);
}

void USFNarrativeEventHub::BroadcastIdentityDriftDirectionChanged(FGameplayTag AxisTag, ESFIdentityDriftDirection DriftDirection)
{
    OnIdentityDriftDirectionChanged.Broadcast(AxisTag, DriftDirection);
}

void USFNarrativeEventHub::BroadcastOutcomeApplied(FPrimaryAssetId OutcomeAssetId, FName ContextId)
{
    OnOutcomeApplied.Broadcast(OutcomeAssetId, ContextId);
}

void USFNarrativeEventHub::BroadcastEndingStateChanged(FGameplayTag EndingTag, ESFEndingAvailability Availability, float Score)
{
    OnEndingStateChanged.Broadcast(EndingTag, Availability, Score);
}

void USFNarrativeEventHub::BroadcastEndingLocked(FGameplayTag EndingTag)
{
    OnEndingLocked.Broadcast(EndingTag);
}

void USFNarrativeEventHub::BroadcastPointOfNoReturnReached(FGameplayTag PointTag)
{
    OnPointOfNoReturnReached.Broadcast(PointTag);
}

void USFNarrativeEventHub::BroadcastBeginSave(const FString& SlotName)
{
    OnBeginSave.Broadcast(SlotName);
}

void USFNarrativeEventHub::BroadcastCompleteSave(const FString& SlotName)
{
    OnCompleteSave.Broadcast(SlotName);
}

void USFNarrativeEventHub::BroadcastBeginLoad(const FString& SlotName)
{
    OnBeginLoad.Broadcast(SlotName);
}

void USFNarrativeEventHub::BroadcastCompleteLoad(const FString& SlotName)
{
    OnCompleteLoad.Broadcast(SlotName);
}

void USFNarrativeEventHub::BroadcastBeforeApplyLoadedState(const FString& SlotName)
{
    OnBeforeApplyLoadedState.Broadcast(SlotName);
}

void USFNarrativeEventHub::BroadcastAfterApplyLoadedState(const FString& SlotName)
{
    OnAfterApplyLoadedState.Broadcast(SlotName);
}
