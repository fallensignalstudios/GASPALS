// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "SFNarrativeComponent.h"

#include "Dialogue/Data/SFConversationDataAsset.h"
#include "Dialogue/Data/SFDialogueComponent.h"

#include "Engine/AssetManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

#include "SFDialogueNarrativeMap.h"
#include "SFNarrativeBlueprintLibrary.h"
#include "SFNarrativeDataAsset.h"
#include "SFNarrativeEventHub.h"
#include "SFNarrativeFactSubsystem.h"
#include "SFNarrativeFunctionLibrary.h"
#include "SFNarrativeReplicatorComponent.h"
#include "SFNarrativeSaveService.h"
#include "SFNarrativeStateQueryLibrary.h"
#include "SFNarrativeStateSubsystem.h"
#include "SFQuestDatabase.h"
#include "SFQuestDefinition.h"
#include "SFQuestInstance.h"
#include "SFQuestRuntime.h"

USFNarrativeComponent::USFNarrativeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

//
// Soft-pointer accessors (defined out-of-line because TSoftObjectPtr<T>::LoadSynchronous
// instantiates Cast<T> at the use site, which requires the full T definition).
//

USFNarrativeDataAsset* USFNarrativeComponent::GetNarrativeData() const
{
    return NarrativeData.LoadSynchronous();
}

USFQuestDatabase* USFNarrativeComponent::GetQuestDatabase() const
{
    return QuestDatabase.LoadSynchronous();
}

USFDialogueNarrativeMap* USFNarrativeComponent::GetDialogueNarrativeMap() const
{
    return DialogueNarrativeMap.LoadSynchronous();
}

void USFNarrativeComponent::BeginPlay()
{
    Super::BeginPlay();

    CachedOwnerController = GetOwningController();
    ResolveRuntimeDependencies();
    BindRuntimeDelegates();
}

void USFNarrativeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnbindRuntimeDelegates();

    if (DialogueComponent && DialogueComponent->IsConversationActive())
    {
        DialogueComponent->EndConversation();
    }

    CachedActiveConversation = nullptr;
    CachedConversationSourceActor = nullptr;
    CachedConversationNodeId = NAME_None;
    PendingClientSyncBatch.Reset();

    Super::EndPlay(EndPlayReason);
}

void USFNarrativeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

APawn* USFNarrativeComponent::GetOwningPawn() const
{
    if (CachedOwnerController)
    {
        return CachedOwnerController->GetPawn();
    }

    if (const APlayerController* PC = GetOwningController())
    {
        return PC->GetPawn();
    }

    return Cast<APawn>(GetOwner());
}

APlayerController* USFNarrativeComponent::GetOwningController() const
{
    if (CachedOwnerController)
    {
        return CachedOwnerController;
    }

    if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
    {
        return PC;
    }

    if (const APawn* PawnOwner = Cast<APawn>(GetOwner()))
    {
        return Cast<APlayerController>(PawnOwner->GetController());
    }

    if (const APlayerState* PlayerStateOwner = Cast<APlayerState>(GetOwner()))
    {
        return Cast<APlayerController>(PlayerStateOwner->GetOwner());
    }

    return nullptr;
}

APlayerState* USFNarrativeComponent::GetOwningPlayerState() const
{
    if (const APlayerController* PC = GetOwningController())
    {
        return PC->PlayerState;
    }

    return Cast<APlayerState>(GetOwner());
}

bool USFNarrativeComponent::HasAuthorityOverNarrative() const
{
    return GetOwner() && GetOwner()->HasAuthority();
}

USFNarrativeStateSubsystem* USFNarrativeComponent::GetNarrativeStateSubsystem() const
{
    if (const UObject* WorldContext = GetOwner() ? static_cast<const UObject*>(GetOwner()) : static_cast<const UObject*>(this))
    {
        if (UWorld* World = WorldContext->GetWorld())
        {
            return World->GetSubsystem<USFNarrativeStateSubsystem>();
        }
    }

    return nullptr;
}

USFNarrativeFactSubsystem* USFNarrativeComponent::GetNarrativeFactSubsystem() const
{
    if (const UObject* WorldContext = GetOwner() ? static_cast<const UObject*>(GetOwner()) : static_cast<const UObject*>(this))
    {
        if (UWorld* World = WorldContext->GetWorld())
        {
            return World->GetSubsystem<USFNarrativeFactSubsystem>();
        }
    }

    return nullptr;
}

USFQuestDefinition* USFNarrativeComponent::ResolveQuestDefinitionByAssetId(const FPrimaryAssetId& QuestAssetId) const
{
    if (!QuestAssetId.IsValid())
    {
        return nullptr;
    }

    if (USFQuestDatabase* Database = GetQuestDatabase())
    {
        if (USFQuestDefinition* FromDb = Database->GetQuestById(QuestAssetId))
        {
            return FromDb;
        }
    }

    return Cast<USFQuestDefinition>(UAssetManager::Get().GetPrimaryAssetObject(QuestAssetId));
}

void USFNarrativeComponent::ResolveRuntimeDependencies()
{
    if (!EventHub)
    {
        EventHub = NewObject<USFNarrativeEventHub>(this);
    }

    if (!QuestRuntime)
    {
        QuestRuntime = NewObject<USFQuestRuntime>(this);
    }

    if (QuestRuntime)
    {
        QuestRuntime->Initialize(this);
    }

    if (!SaveService)
    {
        SaveService = NewObject<USFNarrativeSaveService>(this);
    }

    if (SaveService)
    {
        SaveService->Initialize(this, QuestRuntime);
    }

    if (AActor* OwnerActor = GetOwner())
    {
        DialogueComponent = OwnerActor->FindComponentByClass<USFDialogueComponent>();
        Replicator = OwnerActor->FindComponentByClass<USFNarrativeReplicatorComponent>();

        if (!Replicator && OwnerActor->HasAuthority())
        {
            Replicator = NewObject<USFNarrativeReplicatorComponent>(OwnerActor, TEXT("NarrativeReplicator"));
            if (Replicator)
            {
                Replicator->RegisterComponent();
            }
        }

        if (Replicator)
        {
            Replicator->Initialize(this);
        }
    }
}

void USFNarrativeComponent::BindRuntimeDelegates()
{
    if (Replicator)
    {
        Replicator->OnDeltaApplied.AddDynamic(this, &USFNarrativeComponent::HandleReplicatedDeltaApplied);
    }

    if (EventHub)
    {
        EventHub->OnQuestStarted.AddDynamic(this, &USFNarrativeComponent::HandleHubQuestStarted);
        EventHub->OnQuestRestarted.AddDynamic(this, &USFNarrativeComponent::HandleHubQuestRestarted);
        EventHub->OnQuestAbandoned.AddDynamic(this, &USFNarrativeComponent::HandleHubQuestAbandoned);
        EventHub->OnQuestStateChanged.AddDynamic(this, &USFNarrativeComponent::HandleHubQuestStateChanged);
        EventHub->OnQuestTaskProgressed.AddDynamic(this, &USFNarrativeComponent::HandleHubQuestTaskProgressed);
        EventHub->OnNarrativeTaskCompleted.AddDynamic(this, &USFNarrativeComponent::HandleHubNarrativeTaskCompleted);
        EventHub->OnBeginSave.AddDynamic(this, &USFNarrativeComponent::HandleHubBeginSave);
        EventHub->OnCompleteSave.AddDynamic(this, &USFNarrativeComponent::HandleHubCompleteSave);
        EventHub->OnBeginLoad.AddDynamic(this, &USFNarrativeComponent::HandleHubBeginLoad);
        EventHub->OnCompleteLoad.AddDynamic(this, &USFNarrativeComponent::HandleHubCompleteLoad);
    }

    if (DialogueComponent)
    {
        DialogueComponent->OnConversationStarted.AddDynamic(this, &USFNarrativeComponent::HandleDialogueConversationStarted);
        DialogueComponent->OnConversationEnded.AddDynamic(this, &USFNarrativeComponent::HandleDialogueConversationEnded);
        DialogueComponent->OnDialogueNodeUpdated.AddDynamic(this, &USFNarrativeComponent::HandleDialogueNodeUpdated);
        DialogueComponent->OnDialogueEventTriggered.AddDynamic(this, &USFNarrativeComponent::HandleDialogueEventTriggered);
    }
}

void USFNarrativeComponent::UnbindRuntimeDelegates()
{
    if (Replicator)
    {
        Replicator->OnDeltaApplied.RemoveAll(this);
    }

    if (EventHub)
    {
        EventHub->OnQuestStarted.RemoveAll(this);
        EventHub->OnQuestRestarted.RemoveAll(this);
        EventHub->OnQuestAbandoned.RemoveAll(this);
        EventHub->OnQuestStateChanged.RemoveAll(this);
        EventHub->OnQuestTaskProgressed.RemoveAll(this);
        EventHub->OnNarrativeTaskCompleted.RemoveAll(this);
        EventHub->OnBeginSave.RemoveAll(this);
        EventHub->OnCompleteSave.RemoveAll(this);
        EventHub->OnBeginLoad.RemoveAll(this);
        EventHub->OnCompleteLoad.RemoveAll(this);
    }

    if (DialogueComponent)
    {
        DialogueComponent->OnConversationStarted.RemoveAll(this);
        DialogueComponent->OnConversationEnded.RemoveAll(this);
        DialogueComponent->OnDialogueNodeUpdated.RemoveAll(this);
        DialogueComponent->OnDialogueEventTriggered.RemoveAll(this);
    }
}

USFQuestInstance* USFNarrativeComponent::StartQuestByAssetId(FPrimaryAssetId QuestAssetId, FName StartStateId)
{
    if (!HasAuthorityOverNarrative() || !QuestRuntime || !QuestAssetId.IsValid())
    {
        return nullptr;
    }

    if (USFQuestDefinition* QuestDef = ResolveQuestDefinitionByAssetId(QuestAssetId))
    {
        return QuestRuntime->StartQuestByDefinition(QuestDef, StartStateId);
    }

    return nullptr;
}

bool USFNarrativeComponent::RestartQuestByAssetId(FPrimaryAssetId QuestAssetId, FName StartStateId)
{
    if (!HasAuthorityOverNarrative() || !QuestRuntime || !QuestAssetId.IsValid())
    {
        return false;
    }

    if (USFQuestDefinition* QuestDef = ResolveQuestDefinitionByAssetId(QuestAssetId))
    {
        return QuestRuntime->RestartQuestByDefinition(QuestDef, StartStateId);
    }

    return false;
}

bool USFNarrativeComponent::AbandonQuestByAssetId(FPrimaryAssetId QuestAssetId)
{
    if (!HasAuthorityOverNarrative() || !QuestRuntime || !QuestAssetId.IsValid())
    {
        return false;
    }

    if (USFQuestDefinition* QuestDef = ResolveQuestDefinitionByAssetId(QuestAssetId))
    {
        return QuestRuntime->AbandonQuestByDefinition(QuestDef);
    }

    return false;
}

USFQuestInstance* USFNarrativeComponent::GetQuestInstanceByAssetId(FPrimaryAssetId QuestAssetId) const
{
    return (QuestRuntime && QuestAssetId.IsValid())
        ? QuestRuntime->GetQuestInstanceByAssetId(QuestAssetId)
        : nullptr;
}

TArray<USFQuestInstance*> USFNarrativeComponent::GetAllQuestInstances() const
{
    return QuestRuntime ? QuestRuntime->GetAllQuestInstances() : TArray<USFQuestInstance*>();
}

bool USFNarrativeComponent::BeginConversation(USFConversationDataAsset* Conversation, AActor* SourceActor)
{
    if (!HasAuthorityOverNarrative() || !DialogueComponent || !Conversation)
    {
        return false;
    }

    if (DialogueComponent->IsConversationActive())
    {
        DialogueComponent->EndConversation();
    }

    CachedActiveConversation = Conversation;
    CachedConversationSourceActor = SourceActor;
    CachedConversationNodeId = Conversation->EntryNodeId;

    return DialogueComponent->StartConversation(Conversation, SourceActor);
}

void USFNarrativeComponent::AdvanceConversation()
{
    if (!DialogueComponent || !DialogueComponent->IsConversationActive())
    {
        return;
    }

    if (HasAuthorityOverNarrative())
    {
        ApplyDialogueNarrativeHooks(false, INDEX_NONE);
        DialogueComponent->AdvanceConversation();
        return;
    }

    ServerAdvanceConversation();
}

bool USFNarrativeComponent::SelectConversationChoice(int32 ChoiceIndex)
{
    if (!DialogueComponent || !DialogueComponent->IsConversationActive())
    {
        return false;
    }

    if (HasAuthorityOverNarrative())
    {
        ApplyDialogueNarrativeHooks(false, ChoiceIndex);
        return DialogueComponent->SelectChoice(ChoiceIndex);
    }

    ServerSelectConversationChoice(ChoiceIndex);
    return true;
}

void USFNarrativeComponent::EndConversation(ESFDialogueExitReason ExitReason)
{
    if (!DialogueComponent || !DialogueComponent->IsConversationActive())
    {
        return;
    }

    if (HasAuthorityOverNarrative())
    {
        DialogueComponent->EndConversation();
        return;
    }

    ServerEndConversation(ExitReason);
}

bool USFNarrativeComponent::IsConversationActive() const
{
    return DialogueComponent && DialogueComponent->IsConversationActive();
}

USFConversationDataAsset* USFNarrativeComponent::GetActiveConversation() const
{
    return DialogueComponent && DialogueComponent->GetActiveConversation()
        ? DialogueComponent->GetActiveConversation()
        : CachedActiveConversation.Get();
}

AActor* USFNarrativeComponent::GetActiveConversationSourceActor() const
{
    return DialogueComponent && DialogueComponent->GetActiveSourceActor()
        ? DialogueComponent->GetActiveSourceActor()
        : CachedConversationSourceActor.Get();
}

FName USFNarrativeComponent::GetCurrentConversationNodeId() const
{
    return DialogueComponent && !DialogueComponent->GetCurrentNodeId().IsNone()
        ? DialogueComponent->GetCurrentNodeId()
        : CachedConversationNodeId;
}

bool USFNarrativeComponent::ApplyNarrativeDelta(const FSFNarrativeDelta& Delta, bool bReplicate)
{
    return ApplyNarrativeDeltas(TArray<FSFNarrativeDelta>{ Delta }, bReplicate);
}

bool USFNarrativeComponent::ApplyNarrativeDeltas(const TArray<FSFNarrativeDelta>& Deltas, bool bReplicate)
{
    if (!HasAuthorityOverNarrative())
    {
        return false;
    }

    USFNarrativeStateSubsystem* StateSubsystem = GetNarrativeStateSubsystem();
    if (!StateSubsystem || Deltas.Num() == 0)
    {
        return false;
    }

    FSFNarrativeChangeSet CombinedChangeSet;
    bool bAnyApplied = false;

    for (const FSFNarrativeDelta& Delta : Deltas)
    {
        if (USFNarrativeFunctionLibrary::IsDeltaNoOp(Delta))
        {
            continue;
        }

        FSFNarrativeChangeSet LocalChangeSet;
        if (StateSubsystem->ApplyDelta(Delta, LocalChangeSet))
        {
            bAnyApplied = true;
            USFNarrativeFunctionLibrary::MergeChangeSets(CombinedChangeSet, LocalChangeSet);

            if (bReplicate && Replicator)
            {
                Replicator->PushAuthoritativeDelta(Delta);
            }
        }
    }

    if (!bAnyApplied)
    {
        return false;
    }

    for (const FSFWorldFactChange& Change : CombinedChangeSet.WorldFactChanges)
    {
        OnWorldFactChanged.Broadcast(Change.Key.FactTag, Change.Key.ContextId, Change.NewValue);

        FSFNarrativeClientSyncEvent Event;
        Event.Category = ESFNarrativeClientChangeCategory::WorldFact;
        Event.Title = FText::FromString(TEXT("World state updated"));
        Event.bLogInHistory = true;
        EnqueueClientSyncEvent(Event);
    }

    for (const FSFFactionChange& Change : CombinedChangeSet.FactionChanges)
    {
        OnFactionStandingChanged.Broadcast(Change.FactionTag, Change.OldStanding.Trust, Change.NewStanding.Trust);

        FSFNarrativeClientSyncEvent Event;
        Event.Category = ESFNarrativeClientChangeCategory::Faction;
        Event.Title = FText::FromString(TEXT("Faction standing changed"));
        Event.OldValue = Change.OldStanding.Trust;
        Event.NewValue = Change.NewStanding.Trust;
        EnqueueClientSyncEvent(Event);
    }

    for (const FSFIdentityChange& Change : CombinedChangeSet.IdentityChanges)
    {
        OnIdentityAxisChanged.Broadcast(Change.NewValue.AxisTag, Change.OldValue.Value, Change.NewValue.Value);

        FSFNarrativeClientSyncEvent Event;
        Event.Category = ESFNarrativeClientChangeCategory::IdentityAxis;
        Event.Title = FText::FromString(TEXT("Identity updated"));
        Event.OldValue = Change.OldValue.Value;
        Event.NewValue = Change.NewValue.Value;
        EnqueueClientSyncEvent(Event);
    }

    for (const FSFEndingState& EndingState : CombinedChangeSet.EndingStatesChanged)
    {
        OnEndingStateChanged.Broadcast(EndingState.EndingTag, static_cast<uint8>(EndingState.Availability), EndingState.Score);

        FSFNarrativeClientSyncEvent Event;
        Event.Category = ESFNarrativeClientChangeCategory::Ending;
        Event.Urgency = ESFNarrativeClientChangeUrgency::Important;
        Event.Title = FText::FromString(TEXT("Ending state updated"));
        Event.NewValue = EndingState.Score;
        EnqueueClientSyncEvent(Event);
    }

    FlushClientSyncBatch();
    return true;
}

bool USFNarrativeComponent::CompleteNarrativeTask(FGameplayTag TaskTag, FName ContextId, int32 Quantity)
{
    if (!HasAuthorityOverNarrative() || !QuestRuntime || !TaskTag.IsValid() || Quantity <= 0)
    {
        return false;
    }

    const bool bApplied = QuestRuntime->ApplyTaskProgress(TaskTag, ContextId, Quantity, false);
    if (bApplied && EventHub)
    {
        EventHub->OnNarrativeTaskCompleted.Broadcast(TaskTag, ContextId, Quantity);
    }

    return bApplied;
}

bool USFNarrativeComponent::SetWorldFact(const FSFWorldFactKey& Key, const FSFWorldFactValue& Value, bool bReplicate)
{
    const FSFNarrativeDelta Delta = FSFNarrativeDelta::MakeSetWorldFact(0, Key.FactTag, Key.ContextId, Value);
    return ApplyNarrativeDelta(Delta, bReplicate);
}

bool USFNarrativeComponent::GetWorldFactValue(const FSFWorldFactKey& Key, FSFWorldFactValue& OutValue) const
{
    if (USFNarrativeStateSubsystem* StateSubsystem = GetNarrativeStateSubsystem())
    {
        return StateSubsystem->GetWorldFact(Key, OutValue);
    }

    return false;
}

bool USFNarrativeComponent::HasWorldFact(const FSFWorldFactKey& Key) const
{
    FSFWorldFactValue Dummy;
    return GetWorldFactValue(Key, Dummy);
}

bool USFNarrativeComponent::GetFactionScore(FGameplayTag FactionTag, float& OutScore) const
{
    if (!FactionTag.IsValid())
    {
        OutScore = 0.0f;
        return false;
    }

    if (USFNarrativeStateSubsystem* StateSubsystem = GetNarrativeStateSubsystem())
    {
        FSFFactionStandingValue Standing;
        if (StateSubsystem->GetFactionStanding(FactionTag, Standing))
        {
            OutScore = Standing.Trust; // Proxy old single-score access onto Trust in the canonical faction schema.
            return true;
        }
    }

    OutScore = 0.0f;
    return false;
}

bool USFNarrativeComponent::GetIdentityAxisValue(FGameplayTag AxisTag, float& OutValue) const
{
    bool bHasValue = false;
    OutValue = USFNarrativeStateQueryLibrary::GetIdentityAxisValue(this, AxisTag, bHasValue);
    return bHasValue;
}

bool USFNarrativeComponent::EvaluateConditionSet(const FSFNarrativeConditionSet& ConditionSet, const USFQuestDefinition* QuestDef, const USFQuestInstance* QuestInstance) const
{
    return USFNarrativeStateQueryLibrary::EvaluateConditionSet(this, ConditionSet, QuestDef, QuestInstance);
}

bool USFNarrativeComponent::SaveToSlot(const FString& SlotName, int32 SlotIndex)
{
    return HasAuthorityOverNarrative() && SaveService ? SaveService->SaveToSlot(SlotName, SlotIndex) : false;
}

bool USFNarrativeComponent::LoadFromSlot(const FString& SlotName, int32 SlotIndex)
{
    return HasAuthorityOverNarrative() && SaveService ? SaveService->LoadFromSlot(SlotName, SlotIndex) : false;
}

bool USFNarrativeComponent::DeleteSave(const FString& SlotName, int32 SlotIndex)
{
    return HasAuthorityOverNarrative() && SaveService ? SaveService->DeleteSlot(SlotName, SlotIndex) : false;
}

void USFNarrativeComponent::ServerAdvanceConversation_Implementation()
{
    AdvanceConversation();
}

void USFNarrativeComponent::ServerSelectConversationChoice_Implementation(int32 ChoiceIndex)
{
    SelectConversationChoice(ChoiceIndex);
}

void USFNarrativeComponent::ServerEndConversation_Implementation(ESFDialogueExitReason ExitReason)
{
    EndConversation(ExitReason);
}

bool USFNarrativeComponent::ApplyDialogueNarrativeHooks(bool bOnEnter, int32 ChoiceIndex)
{
    USFDialogueNarrativeMap* MapAsset = GetDialogueNarrativeMap();
    if (!MapAsset || !DialogueComponent)
    {
        return false;
    }

    USFConversationDataAsset* Conversation = GetActiveConversation();
    const FName NodeId = GetCurrentConversationNodeId();
    if (!Conversation || NodeId.IsNone())
    {
        return false;
    }

    FSFDialogueKey Key;
    Key.ConversationId = Conversation->GetFName();
    Key.NodeId = NodeId;
    Key.OptionIndex = ChoiceIndex;

    FSFDialogueNarrativeEntry Entry;
    if (!MapAsset->GetEntry(Key, Entry))
    {
        return false;
    }

    if (!EvaluateConditionSet(Entry.AvailabilityConditions, nullptr, nullptr))
    {
        return false;
    }

    const TArray<FSFNarrativeDelta>& DeltasToApply = bOnEnter ? Entry.OnEnterDeltas : Entry.OnExitDeltas;
    if (DeltasToApply.Num() > 0)
    {
        ApplyNarrativeDeltas(DeltasToApply, true);
    }

    if (Entry.bMarkAsSeenFact
        && Entry.SeenFactKey.FactTag.IsValid()
        && !Entry.SeenFactKey.ContextId.IsNone())
    {
        FSFNarrativeDelta SeenDelta = USFNarrativeFunctionLibrary::MakeBoolFactDelta(Entry.SeenFactKey, true);
        ApplyNarrativeDelta(SeenDelta, true);
    }

    if (!Entry.ConsequenceDescription.IsEmpty())
    {
        FSFNarrativeClientSyncEvent Event;
        Event.Category = ESFNarrativeClientChangeCategory::Dialogue;
        Event.Title = FText::FromString(TEXT("Dialogue consequence"));
        Event.Body = Entry.ConsequenceDescription;
        Event.bLogInHistory = true;
        EnqueueClientSyncEvent(Event);
        FlushClientSyncBatch();
    }

    return true;
}

void USFNarrativeComponent::EnqueueClientSyncEvent(const FSFNarrativeClientSyncEvent& Event)
{
    PendingClientSyncBatch.Events.Add(Event);
}

void USFNarrativeComponent::FlushClientSyncBatch()
{
    if (!PendingClientSyncBatch.IsEmpty())
    {
        OnClientSyncBatchReady.Broadcast(PendingClientSyncBatch);
        PendingClientSyncBatch.Reset();
    }
}

void USFNarrativeComponent::HandleReplicatedDeltaApplied(const FSFNarrativeDelta& Delta)
{
    FSFNarrativeClientSyncEvent Event;
    Event.Category = ESFNarrativeClientChangeCategory::Custom;
    Event.Title = FText::FromString(TEXT("Narrative state synchronized"));
    Event.Body = FText::FromString(TEXT("A replicated narrative update was applied."));
    Event.bLogInHistory = false;
    Event.ObjectiveOrStateId = Delta.SubjectId;

    EnqueueClientSyncEvent(Event);
    FlushClientSyncBatch();
}

void USFNarrativeComponent::HandleHubQuestStarted(USFQuestInstance* QuestInstance)
{
    OnQuestStarted.Broadcast(QuestInstance);

    FSFNarrativeClientSyncEvent Event;
    Event.Category = ESFNarrativeClientChangeCategory::QuestState;
    Event.Urgency = ESFNarrativeClientChangeUrgency::Important;
    Event.Title = FText::FromString(TEXT("Quest started"));
    Event.bLogInHistory = true;
    EnqueueClientSyncEvent(Event);
    FlushClientSyncBatch();
}

void USFNarrativeComponent::HandleHubQuestRestarted(USFQuestInstance* QuestInstance)
{
    OnQuestRestarted.Broadcast(QuestInstance);
}

void USFNarrativeComponent::HandleHubQuestAbandoned(USFQuestInstance* QuestInstance)
{
    OnQuestAbandoned.Broadcast(QuestInstance);
}

void USFNarrativeComponent::HandleHubQuestStateChanged(USFQuestInstance* QuestInstance, FName StateId)
{
    OnQuestStateChanged.Broadcast(QuestInstance, StateId);
}

void USFNarrativeComponent::HandleHubQuestTaskProgressed(USFQuestInstance* QuestInstance, FGameplayTag TaskTag, FName ContextId, int32 Quantity)
{
    OnQuestTaskProgressed.Broadcast(QuestInstance, TaskTag, ContextId, Quantity);
}

void USFNarrativeComponent::HandleHubNarrativeTaskCompleted(FGameplayTag TaskTag, FName ContextId, int32 Quantity)
{
    OnNarrativeTaskCompleted.Broadcast(TaskTag);

    FSFNarrativeClientSyncEvent Event;
    Event.Category = ESFNarrativeClientChangeCategory::LifetimeTask;
    Event.Title = FText::FromString(TEXT("Task progress updated"));
    Event.ObjectiveOrStateId = ContextId;
    Event.Tags.AddTag(TaskTag);
    Event.NumericValue = static_cast<float>(Quantity);
    Event.bHasNumericValue = true;
    EnqueueClientSyncEvent(Event);
    FlushClientSyncBatch();
}

void USFNarrativeComponent::HandleHubBeginSave(const FString& SlotName)
{
    OnBeginSave.Broadcast(SlotName);
}

void USFNarrativeComponent::HandleHubCompleteSave(const FString& SlotName)
{
    OnSaveComplete.Broadcast(SlotName);
}

void USFNarrativeComponent::HandleHubBeginLoad(const FString& SlotName)
{
    OnBeginLoad.Broadcast(SlotName);
}

void USFNarrativeComponent::HandleHubCompleteLoad(const FString& SlotName)
{
    OnLoadComplete.Broadcast(SlotName);
}

void USFNarrativeComponent::HandleDialogueConversationStarted(AActor* SourceActor)
{
    CachedConversationSourceActor = SourceActor;
    CachedActiveConversation = DialogueComponent && DialogueComponent->GetActiveConversation()
        ? DialogueComponent->GetActiveConversation()
        : CachedActiveConversation.Get();
    CachedConversationNodeId = DialogueComponent ? DialogueComponent->GetCurrentNodeId() : CachedConversationNodeId;

    ApplyDialogueNarrativeHooks(true, INDEX_NONE);
    OnConversationStarted.Broadcast(CachedActiveConversation.Get(), CachedConversationSourceActor.Get());
}

void USFNarrativeComponent::HandleDialogueConversationEnded()
{
    USFConversationDataAsset* EndedConversation = CachedActiveConversation.Get();
    OnConversationEnded.Broadcast(EndedConversation);

    CachedActiveConversation = nullptr;
    CachedConversationSourceActor = nullptr;
    CachedConversationNodeId = NAME_None;
}

void USFNarrativeComponent::HandleDialogueNodeUpdated(const FSFDialogueDisplayData& DisplayData)
{
    CachedActiveConversation = DialogueComponent && DialogueComponent->GetActiveConversation()
        ? DialogueComponent->GetActiveConversation()
        : CachedActiveConversation.Get();

    CachedConversationSourceActor = DialogueComponent && DialogueComponent->GetActiveSourceActor()
        ? DialogueComponent->GetActiveSourceActor()
        : CachedConversationSourceActor.Get();

    CachedConversationNodeId = DialogueComponent
        ? DialogueComponent->GetCurrentNodeId()
        : CachedConversationNodeId;

    ApplyDialogueNarrativeHooks(true, INDEX_NONE);
    OnConversationNodeUpdated.Broadcast(CachedActiveConversation.Get(), CachedConversationNodeId, DisplayData);
}

void USFNarrativeComponent::HandleDialogueEventTriggered(FGameplayTag EventTag)
{
    OnConversationEventTriggered.Broadcast(GetActiveConversation(), EventTag);

    if (bTreatDialogueEventTagsAsNarrativeTasks && EventTag.IsValid())
    {
        CompleteNarrativeTask(EventTag, GetCurrentConversationNodeId(), 1);
    }
}

// Copyright Fallen Signal Studios LLC. All Rights Reserved.
