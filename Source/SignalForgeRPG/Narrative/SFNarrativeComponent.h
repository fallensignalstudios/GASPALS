#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "UObject/PrimaryAssetId.h"

#include "SFNarrativeTypes.h"
#include "SFNarrativeSaveTypes.h"
#include "SFNarrativeClientSyncTypes.h"
#include "SFNarrativeComponent.generated.h"

class AActor;
class APawn;
class APlayerController;
class APlayerState;
class UDataTable;
class USFConversationDataAsset;
class USFDialogueComponent;
class USFDialogueNarrativeMap;
class USFNarrativeDataAsset;
class USFNarrativeEventHub;
class USFNarrativeReplicatorComponent;
class USFNarrativeSaveService;
class USFNarrativeStateSubsystem;
class USFNarrativeFactSubsystem;
class USFQuestDatabase;
class USFQuestDefinition;
class USFQuestInstance;
class USFQuestRuntime;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSFComponentQuestEvent, USFQuestInstance*, QuestInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSFComponentQuestStateEvent, USFQuestInstance*, QuestInstance, FName, StateId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FSFComponentQuestTaskEvent, USFQuestInstance*, QuestInstance, FGameplayTag, TaskTag, FName, ContextId, int32, Quantity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSFComponentTaskEvent, FGameplayTag, TaskTag);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSFComponentConversationEvent, USFConversationDataAsset*, Conversation, AActor*, SourceActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSFComponentConversationEndedEvent, USFConversationDataAsset*, Conversation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSFComponentConversationNodeEvent, USFConversationDataAsset*, Conversation, FName, NodeId, const FSFDialogueDisplayData&, DisplayData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSFComponentConversationTagEvent, USFConversationDataAsset*, Conversation, FGameplayTag, EventTag);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSFComponentSaveEvent, const FString&, SlotName);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSFComponentWorldFactChanged, FGameplayTag, FactTag, FName, ContextId, const FSFWorldFactValue&, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSFComponentFactionStandingChanged, FGameplayTag, FactionTag, float, OldValue, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSFComponentIdentityAxisChanged, FGameplayTag, AxisTag, float, OldValue, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FSFComponentEndingStateChanged, FGameplayTag, EndingTag, uint8, Availability, float, Score);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSFComponentClientSyncBatchEvent, const FSFNarrativeClientSyncBatch&, Batch);

UCLASS(ClassGroup = (Narrative), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class SIGNALFORGERPG_API USFNarrativeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USFNarrativeComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintPure, Category = "Narrative")
    APawn* GetOwningPawn() const;

    UFUNCTION(BlueprintPure, Category = "Narrative")
    APlayerController* GetOwningController() const;

    UFUNCTION(BlueprintPure, Category = "Narrative")
    APlayerState* GetOwningPlayerState() const;

    UFUNCTION(BlueprintPure, Category = "Narrative")
    bool HasAuthorityOverNarrative() const;

    //
    // Config / dependencies
    //

    UFUNCTION(BlueprintPure, Category = "Narrative|Config")
    USFNarrativeDataAsset* GetNarrativeData() const { return NarrativeData.LoadSynchronous(); }

    UFUNCTION(BlueprintPure, Category = "Narrative|Config")
    USFQuestDatabase* GetQuestDatabase() const { return QuestDatabase.LoadSynchronous(); }

    UFUNCTION(BlueprintPure, Category = "Narrative|Config")
    USFDialogueNarrativeMap* GetDialogueNarrativeMap() const { return DialogueNarrativeMap.LoadSynchronous(); }

    //
    // Quest API
    //

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Narrative|Quests")
    USFQuestInstance* StartQuestByAssetId(FPrimaryAssetId QuestAssetId, FName StartStateId = NAME_None);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Narrative|Quests")
    bool RestartQuestByAssetId(FPrimaryAssetId QuestAssetId, FName StartStateId = NAME_None);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Narrative|Quests")
    bool AbandonQuestByAssetId(FPrimaryAssetId QuestAssetId);

    UFUNCTION(BlueprintPure, Category = "Narrative|Quests")
    USFQuestInstance* GetQuestInstanceByAssetId(FPrimaryAssetId QuestAssetId) const;

    UFUNCTION(BlueprintPure, Category = "Narrative|Quests")
    TArray<USFQuestInstance*> GetAllQuestInstances() const;

    //
    // Dialogue API
    //

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Narrative|Dialogue")
    bool BeginConversation(USFConversationDataAsset* Conversation, AActor* SourceActor = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Dialogue")
    void AdvanceConversation();

    UFUNCTION(BlueprintCallable, Category = "Narrative|Dialogue")
    bool SelectConversationChoice(int32 ChoiceIndex);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Dialogue")
    void EndConversation(ESFDialogueExitReason ExitReason = ESFDialogueExitReason::Cancelled);

    UFUNCTION(BlueprintPure, Category = "Narrative|Dialogue")
    bool IsConversationActive() const;

    UFUNCTION(BlueprintPure, Category = "Narrative|Dialogue")
    USFConversationDataAsset* GetActiveConversation() const;

    UFUNCTION(BlueprintPure, Category = "Narrative|Dialogue")
    AActor* GetActiveConversationSourceActor() const;

    UFUNCTION(BlueprintPure, Category = "Narrative|Dialogue")
    FName GetCurrentConversationNodeId() const;

    UFUNCTION(BlueprintPure, Category = "Narrative|Dialogue")
    USFDialogueComponent* GetDialogueComponent() const { return DialogueComponent; }

    //
    // State API
    //

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Narrative|State")
    bool ApplyNarrativeDelta(const FSFNarrativeDelta& Delta, bool bReplicate = true);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Narrative|State")
    bool ApplyNarrativeDeltas(const TArray<FSFNarrativeDelta>& Deltas, bool bReplicate = true);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Narrative|Tasks")
    bool CompleteNarrativeTask(FGameplayTag TaskTag, FName ContextId, int32 Quantity = 1);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Narrative|World")
    bool SetWorldFact(const FSFWorldFactKey& Key, const FSFWorldFactValue& Value, bool bReplicate = true);

    UFUNCTION(BlueprintPure, Category = "Narrative|World")
    bool GetWorldFactValue(const FSFWorldFactKey& Key, FSFWorldFactValue& OutValue) const;

    UFUNCTION(BlueprintPure, Category = "Narrative|World")
    bool HasWorldFact(const FSFWorldFactKey& Key) const;

    UFUNCTION(BlueprintPure, Category = "Narrative|Faction")
    bool GetFactionScore(FGameplayTag FactionTag, float& OutScore) const;

    UFUNCTION(BlueprintPure, Category = "Narrative|Identity")
    bool GetIdentityAxisValue(FGameplayTag AxisTag, float& OutValue) const;

    UFUNCTION(BlueprintPure, Category = "Narrative|Conditions")
    bool EvaluateConditionSet(const FSFNarrativeConditionSet& ConditionSet, const USFQuestDefinition* QuestDef = nullptr, const USFQuestInstance* QuestInstance = nullptr) const;

    //
    // Save API
    //

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Narrative|Save")
    bool SaveToSlot(const FString& SlotName, int32 SlotIndex = 0);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Narrative|Save")
    bool LoadFromSlot(const FString& SlotName, int32 SlotIndex = 0);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Narrative|Save")
    bool DeleteSave(const FString& SlotName, int32 SlotIndex = 0);

    //
    // Runtime access
    //

    UFUNCTION(BlueprintPure, Category = "Narrative|Subsystems")
    USFNarrativeStateSubsystem* GetNarrativeStateSubsystem() const;

    UFUNCTION(BlueprintPure, Category = "Narrative|Subsystems")
    USFNarrativeFactSubsystem* GetNarrativeFactSubsystem() const;

    UFUNCTION(BlueprintPure, Category = "Narrative|Subsystems")
    USFQuestRuntime* GetQuestRuntime() const { return QuestRuntime; }

    UFUNCTION(BlueprintPure, Category = "Narrative|Subsystems")
    USFNarrativeEventHub* GetEventHub() const { return EventHub; }

    UFUNCTION(BlueprintPure, Category = "Narrative|Subsystems")
    USFNarrativeSaveService* GetSaveService() const { return SaveService; }

    UFUNCTION(BlueprintPure, Category = "Narrative|Subsystems")
    USFNarrativeReplicatorComponent* GetReplicator() const { return Replicator; }

    //
    // Delegates
    //

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Quest")
    FSFComponentQuestEvent OnQuestStarted;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Quest")
    FSFComponentQuestEvent OnQuestRestarted;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Quest")
    FSFComponentQuestEvent OnQuestAbandoned;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Quest")
    FSFComponentQuestStateEvent OnQuestStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Quest")
    FSFComponentQuestTaskEvent OnQuestTaskProgressed;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Task")
    FSFComponentTaskEvent OnNarrativeTaskCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Dialogue")
    FSFComponentConversationEvent OnConversationStarted;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Dialogue")
    FSFComponentConversationEndedEvent OnConversationEnded;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Dialogue")
    FSFComponentConversationNodeEvent OnConversationNodeUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Dialogue")
    FSFComponentConversationTagEvent OnConversationEventTriggered;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Save")
    FSFComponentSaveEvent OnBeginSave;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Save")
    FSFComponentSaveEvent OnSaveComplete;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Save")
    FSFComponentSaveEvent OnBeginLoad;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Save")
    FSFComponentSaveEvent OnLoadComplete;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|World")
    FSFComponentWorldFactChanged OnWorldFactChanged;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Factions")
    FSFComponentFactionStandingChanged OnFactionStandingChanged;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Identity")
    FSFComponentIdentityAxisChanged OnIdentityAxisChanged;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Endings")
    FSFComponentEndingStateChanged OnEndingStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Events|Client")
    FSFComponentClientSyncBatchEvent OnClientSyncBatchReady;

protected:
    UFUNCTION(Server, Reliable)
    void ServerAdvanceConversation();

    UFUNCTION(Server, Reliable)
    void ServerSelectConversationChoice(int32 ChoiceIndex);

    UFUNCTION(Server, Reliable)
    void ServerEndConversation(ESFDialogueExitReason ExitReason);

    void ResolveRuntimeDependencies();
    void BindRuntimeDelegates();
    void UnbindRuntimeDelegates();
    void FlushClientSyncBatch();

    USFQuestDefinition* ResolveQuestDefinitionByAssetId(const FPrimaryAssetId& QuestAssetId) const;
    bool ApplyDialogueNarrativeHooks(bool bOnEnter, int32 ChoiceIndex = INDEX_NONE);
    void EnqueueClientSyncEvent(const FSFNarrativeClientSyncEvent& Event);

    UFUNCTION()
    void HandleReplicatedDeltaApplied(const FSFNarrativeReplicatedDelta& Delta);

    UFUNCTION()
    void HandleHubQuestStarted(USFQuestInstance* QuestInstance);

    UFUNCTION()
    void HandleHubQuestRestarted(USFQuestInstance* QuestInstance);

    UFUNCTION()
    void HandleHubQuestAbandoned(USFQuestInstance* QuestInstance);

    UFUNCTION()
    void HandleHubQuestStateChanged(USFQuestInstance* QuestInstance, FName StateId);

    UFUNCTION()
    void HandleHubQuestTaskProgressed(USFQuestInstance* QuestInstance, FGameplayTag TaskTag, FName ContextId, int32 Quantity);

    UFUNCTION()
    void HandleHubNarrativeTaskCompleted(FGameplayTag TaskTag, FName ContextId, int32 Quantity);

    UFUNCTION()
    void HandleHubBeginSave(const FString& SlotName);

    UFUNCTION()
    void HandleHubCompleteSave(const FString& SlotName);

    UFUNCTION()
    void HandleHubBeginLoad(const FString& SlotName);

    UFUNCTION()
    void HandleHubCompleteLoad(const FString& SlotName);

    UFUNCTION()
    void HandleDialogueConversationStarted(AActor* SourceActor);

    UFUNCTION()
    void HandleDialogueConversationEnded();

    UFUNCTION()
    void HandleDialogueNodeUpdated(const FSFDialogueDisplayData& DisplayData);

    UFUNCTION()
    void HandleDialogueEventTriggered(FGameplayTag EventTag);

protected:
    UPROPERTY(EditDefaultsOnly, Category = "Narrative|Config")
    TSoftObjectPtr<USFNarrativeDataAsset> NarrativeData;

    UPROPERTY(EditDefaultsOnly, Category = "Narrative|Config")
    TSoftObjectPtr<USFQuestDatabase> QuestDatabase;

    UPROPERTY(EditDefaultsOnly, Category = "Narrative|Dialogue")
    TSoftObjectPtr<USFDialogueNarrativeMap> DialogueNarrativeMap;

    UPROPERTY(EditDefaultsOnly, Category = "Narrative|Dialogue")
    bool bTreatDialogueEventTagsAsNarrativeTasks = true;

    UPROPERTY(Transient)
    TObjectPtr<USFQuestRuntime> QuestRuntime = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USFNarrativeEventHub> EventHub = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USFNarrativeSaveService> SaveService = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USFNarrativeReplicatorComponent> Replicator = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USFDialogueComponent> DialogueComponent = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<APlayerController> CachedOwnerController = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USFConversationDataAsset> CachedActiveConversation = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<AActor> CachedConversationSourceActor = nullptr;

    UPROPERTY(Transient)
    FName CachedConversationNodeId = NAME_None;

    UPROPERTY(Transient)
    FSFNarrativeClientSyncBatch PendingClientSyncBatch;
};