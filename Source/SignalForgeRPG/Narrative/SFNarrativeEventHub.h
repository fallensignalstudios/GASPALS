#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeEventHub.generated.h"

class AActor;
class USFConversationDataAsset;
class USFQuestInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSFQuestEvent, USFQuestInstance*, QuestInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSFQuestStateEvent, USFQuestInstance*, QuestInstance, FName, StateId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FSFQuestTaskEvent, USFQuestInstance*, QuestInstance, FGameplayTag, TaskTag, FName, ContextId, int32, Quantity);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSFDialogueStartedEvent, USFConversationDataAsset*, Conversation, AActor*, SourceActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSFDialogueEndedEvent, USFConversationDataAsset*, Conversation, ESFDialogueExitReason, ExitReason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSFDialogueNodeEvent, USFConversationDataAsset*, Conversation, FName, NodeId, const FSFDialogueDisplayData&, DisplayData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSFDialogueTagEvent, FGameplayTag, EventTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSFDialogueChoiceEvent, USFConversationDataAsset*, Conversation, FName, NodeId, FName, SpeakerId);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSFTaskEvent, FGameplayTag, TaskTag, FName, ContextId, int32, Quantity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSFSaveEvent, const FString&, SlotName);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSFWorldFactChangedEvent, FGameplayTag, FactTag, FName, ContextId, const FSFWorldFactValue&, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSFWorldFactRemovedEvent, FGameplayTag, FactTag, FName, ContextId);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSFFactionStandingChangedEvent, FGameplayTag, FactionTag, const FSFFactionStandingValue&, OldStanding, const FSFFactionStandingValue&, NewStanding);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSFFactionThresholdEvent, FGameplayTag, FactionTag, ESFFactionStandingBand, NewStandingBand);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSFIdentityAxisChangedEvent, FGameplayTag, AxisTag, float, OldValue, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSFIdentityDirectionEvent, FGameplayTag, AxisTag, ESFIdentityDriftDirection, DriftDirection);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSFOutcomeAppliedEvent, FPrimaryAssetId, OutcomeAssetId, FName, ContextId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSFEndingStateChangedEvent, FGameplayTag, EndingTag, ESFEndingAvailability, Availability, float, Score);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSFEndingLockedEvent, FGameplayTag, EndingTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSFPointOfNoReturnEvent, FGameplayTag, PointTag);

UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFNarrativeEventHub : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Quest")
    FSFQuestEvent OnQuestStarted;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Quest")
    FSFQuestEvent OnQuestRestarted;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Quest")
    FSFQuestEvent OnQuestAbandoned;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Quest")
    FSFQuestStateEvent OnQuestStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Quest")
    FSFQuestTaskEvent OnQuestTaskProgressed;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Dialogue")
    FSFDialogueStartedEvent OnDialogueStarted;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Dialogue")
    FSFDialogueEndedEvent OnDialogueEnded;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Dialogue")
    FSFDialogueNodeEvent OnDialogueNodeUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Dialogue")
    FSFDialogueTagEvent OnDialogueEventTriggered;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Dialogue")
    FSFDialogueChoiceEvent OnDialogueChoiceSelected;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Task")
    FSFTaskEvent OnNarrativeTaskCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|World")
    FSFWorldFactChangedEvent OnWorldFactChanged;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|World")
    FSFWorldFactRemovedEvent OnWorldFactRemoved;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Factions")
    FSFFactionStandingChangedEvent OnFactionStandingChanged;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Factions")
    FSFFactionThresholdEvent OnFactionStandingBandChanged;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Identity")
    FSFIdentityAxisChangedEvent OnIdentityAxisChanged;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Identity")
    FSFIdentityDirectionEvent OnIdentityDriftDirectionChanged;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Outcomes")
    FSFOutcomeAppliedEvent OnOutcomeApplied;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Endings")
    FSFEndingStateChangedEvent OnEndingStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Endings")
    FSFEndingLockedEvent OnEndingLocked;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Endings")
    FSFPointOfNoReturnEvent OnPointOfNoReturnReached;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Save")
    FSFSaveEvent OnBeginSave;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Save")
    FSFSaveEvent OnCompleteSave;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Save")
    FSFSaveEvent OnBeginLoad;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Save")
    FSFSaveEvent OnCompleteLoad;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Save")
    FSFSaveEvent OnBeforeApplyLoadedState;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Save")
    FSFSaveEvent OnAfterApplyLoadedState;

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events|Quest")
    void BroadcastQuestStarted(USFQuestInstance* QuestInstance);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events|Quest")
    void BroadcastQuestRestarted(USFQuestInstance* QuestInstance);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events|Quest")
    void BroadcastQuestAbandoned(USFQuestInstance* QuestInstance);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events|Quest")
    void BroadcastQuestStateChanged(USFQuestInstance* QuestInstance, FName StateId);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events|Quest")
    void BroadcastQuestTaskProgressed(USFQuestInstance* QuestInstance, FGameplayTag TaskTag, FName ContextId, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events|Dialogue")
    void BroadcastDialogueStarted(USFConversationDataAsset* Conversation, AActor* SourceActor);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events|Dialogue")
    void BroadcastDialogueEnded(USFConversationDataAsset* Conversation, ESFDialogueExitReason ExitReason);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events|Dialogue")
    void BroadcastDialogueNodeUpdated(USFConversationDataAsset* Conversation, FName NodeId, const FSFDialogueDisplayData& DisplayData);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events|Dialogue")
    void BroadcastDialogueEventTriggered(FGameplayTag EventTag);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events|Dialogue")
    void BroadcastDialogueChoiceSelected(USFConversationDataAsset* Conversation, FName NodeId, FName SpeakerId);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events|Task")
    void BroadcastNarrativeTaskCompleted(FGameplayTag TaskTag, FName ContextId, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events|World")
    void BroadcastWorldFactChanged(FGameplayTag FactTag, FName ContextId, const FSFWorldFactValue& NewValue);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events|World")
    void BroadcastWorldFactRemoved(FGameplayTag FactTag, FName ContextId);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events|Factions")
    void BroadcastFactionStandingChanged(FGameplayTag FactionTag, const FSFFactionStandingValue& OldStanding, const FSFFactionStandingValue& NewStanding);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events|Factions")
    void BroadcastFactionStandingBandChanged(FGameplayTag FactionTag, ESFFactionStandingBand NewStandingBand);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events|Identity")
    void BroadcastIdentityAxisChanged(FGameplayTag AxisTag, float OldValue, float NewValue);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events|Identity")
    void BroadcastIdentityDriftDirectionChanged(FGameplayTag AxisTag, ESFIdentityDriftDirection DriftDirection);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events|Outcomes")
    void BroadcastOutcomeApplied(FPrimaryAssetId OutcomeAssetId, FName ContextId);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events|Endings")
    void BroadcastEndingStateChanged(FGameplayTag EndingTag, ESFEndingAvailability Availability, float Score);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events|Endings")
    void BroadcastEndingLocked(FGameplayTag EndingTag);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events|Endings")
    void BroadcastPointOfNoReturnReached(FGameplayTag PointTag);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events|Save")
    void BroadcastBeginSave(const FString& SlotName);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events|Save")
    void BroadcastCompleteSave(const FString& SlotName);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events|Save")
    void BroadcastBeginLoad(const FString& SlotName);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events|Save")
    void BroadcastCompleteLoad(const FString& SlotName);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events|Save")
    void BroadcastBeforeApplyLoadedState(const FString& SlotName);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Events|Save")
    void BroadcastAfterApplyLoadedState(const FString& SlotName);
};