#include "SFQuestRuntime.h"

#include "Engine/AssetManager.h"
#include "SFNarrativeComponent.h"
#include "SFNarrativeEventHub.h"
#include "SFQuestDefinition.h"
#include "SFQuestInstance.h"

bool USFQuestRuntime::Initialize(USFNarrativeComponent* InOwner)
{
    OwnerComponent = InOwner;
    QuestInstancesByAssetId.Reset();
    LifetimeTaskCounts.Reset();
    return OwnerComponent != nullptr;
}

//
// Primary, definition‑driven APIs
//

USFQuestInstance* USFQuestRuntime::StartQuestByDefinition(USFQuestDefinition* QuestDef, FName StartStateId)
{
    if (!OwnerComponent || !QuestDef)
    {
        return nullptr;
    }

    const FPrimaryAssetId QuestAssetId = GetQuestAssetId(QuestDef);
    if (!QuestAssetId.IsValid())
    {
        return nullptr;
    }

    if (USFQuestInstance* ExistingInstance = QuestInstancesByAssetId.FindRef(QuestAssetId))
    {
        return ExistingInstance;
    }

    USFQuestInstance* QuestInstance = NewObject<USFQuestInstance>(this);
    if (!QuestInstance || !QuestInstance->Initialize(OwnerComponent, QuestDef, StartStateId))
    {
        return nullptr;
    }

    QuestInstancesByAssetId.Add(QuestAssetId, QuestInstance);

    if (USFNarrativeEventHub* Hub = OwnerComponent->GetEventHub())
    {
        Hub->OnQuestStarted.Broadcast(QuestInstance);
    }

    return QuestInstance;
}

bool USFQuestRuntime::RestartQuestByDefinition(USFQuestDefinition* QuestDef, FName StartStateId)
{
    if (!OwnerComponent || !QuestDef)
    {
        return false;
    }

    const FPrimaryAssetId QuestAssetId = GetQuestAssetId(QuestDef);
    if (!QuestAssetId.IsValid())
    {
        return false;
    }

    USFQuestInstance* QuestInstance = NewObject<USFQuestInstance>(this);
    if (!QuestInstance || !QuestInstance->Initialize(OwnerComponent, QuestDef, StartStateId))
    {
        return false;
    }

    QuestInstancesByAssetId.Add(QuestAssetId, QuestInstance);

    if (USFNarrativeEventHub* Hub = OwnerComponent->GetEventHub())
    {
        Hub->OnQuestRestarted.Broadcast(QuestInstance);
    }

    return true;
}

bool USFQuestRuntime::AbandonQuestByDefinition(USFQuestDefinition* QuestDef)
{
    if (!OwnerComponent || !QuestDef)
    {
        return false;
    }

    const FPrimaryAssetId QuestAssetId = GetQuestAssetId(QuestDef);
    if (!QuestAssetId.IsValid())
    {
        return false;
    }

    USFQuestInstance* QuestInstance = QuestInstancesByAssetId.FindRef(QuestAssetId);
    if (!QuestInstance)
    {
        return false;
    }

    QuestInstance->MarkAbandoned();

    if (USFNarrativeEventHub* Hub = OwnerComponent->GetEventHub())
    {
        Hub->OnQuestAbandoned.Broadcast(QuestInstance);
    }

    return true;
}

bool USFQuestRuntime::EnterQuestStateByDefinition(USFQuestDefinition* QuestDef, FName StateId, bool bFromReplication)
{
    if (!QuestDef)
    {
        return false;
    }

    const FPrimaryAssetId QuestAssetId = GetQuestAssetId(QuestDef);
    if (!QuestAssetId.IsValid())
    {
        return false;
    }

    if (USFQuestInstance* QuestInstance = QuestInstancesByAssetId.FindRef(QuestAssetId))
    {
        return QuestInstance->EnterState(StateId, bFromReplication);
    }

    return false;
}

USFQuestInstance* USFQuestRuntime::GetQuestInstanceByAssetId(FPrimaryAssetId QuestAssetId) const
{
    return QuestInstancesByAssetId.FindRef(QuestAssetId);
}

//
// Class‑based wrappers
//

USFQuestInstance* USFQuestRuntime::StartQuest(TSubclassOf<USFQuestDefinition> QuestDefClass, FName StartStateId)
{
    return StartQuestByDefinition(ResolveQuestDefinitionFromClass(QuestDefClass), StartStateId);
}

bool USFQuestRuntime::RestartQuest(TSubclassOf<USFQuestDefinition> QuestDefClass, FName StartStateId)
{
    return RestartQuestByDefinition(ResolveQuestDefinitionFromClass(QuestDefClass), StartStateId);
}

bool USFQuestRuntime::AbandonQuest(TSubclassOf<USFQuestDefinition> QuestDefClass)
{
    return AbandonQuestByDefinition(ResolveQuestDefinitionFromClass(QuestDefClass));
}

bool USFQuestRuntime::EnterQuestState(TSubclassOf<USFQuestDefinition> QuestDefClass, FName StateId, bool bFromReplication)
{
    return EnterQuestStateByDefinition(ResolveQuestDefinitionFromClass(QuestDefClass), StateId, bFromReplication);
}

USFQuestInstance* USFQuestRuntime::GetQuestInstance(TSubclassOf<USFQuestDefinition> QuestDefClass) const
{
    const USFQuestDefinition* QuestDef = ResolveQuestDefinitionFromClass(QuestDefClass);
    if (!QuestDef)
    {
        return nullptr;
    }

    return QuestInstancesByAssetId.FindRef(GetQuestAssetId(QuestDef));
}

//
// Tasks
//

bool USFQuestRuntime::ApplyTaskProgress(FGameplayTag TaskTag, FName ContextId, int32 Quantity, bool bFromReplication)
{
    if (!TaskTag.IsValid() || Quantity <= 0)
    {
        return false;
    }

    bool bAnyApplied = false;

    for (TPair<FPrimaryAssetId, TObjectPtr<USFQuestInstance>>& Pair : QuestInstancesByAssetId)
    {
        if (USFQuestInstance* QuestInstance = Pair.Value)
        {
            bAnyApplied |= QuestInstance->ApplyTaskProgress(TaskTag, ContextId, Quantity, bFromReplication);
        }
    }

    if (bAnyApplied)
    {
        IncrementLifetimeTaskCount(TaskTag, ContextId, Quantity);
    }

    return bAnyApplied;
}

bool USFQuestRuntime::ApplyReplicatedTaskProgress(FName QuestId, FName TaskId, int32 NewProgress, int32 DeltaAmount)
{
    if (QuestId.IsNone() || TaskId.IsNone() || NewProgress < 0)
    {
        return false;
    }

    const FPrimaryAssetId QuestAssetId(FPrimaryAssetType(TEXT("Quest")), QuestId);
    if (USFQuestInstance* QuestInstance = GetQuestInstanceByAssetId(QuestAssetId))
    {
        return QuestInstance->ApplyReplicatedTaskProgressByTaskId(TaskId, NewProgress, DeltaAmount);
    }

    return false;
}

//
// Queries
//

TArray<USFQuestInstance*> USFQuestRuntime::GetAllQuestInstances() const
{
    TArray<USFQuestInstance*> Result;
    Result.Reserve(QuestInstancesByAssetId.Num());

    for (const TPair<FPrimaryAssetId, TObjectPtr<USFQuestInstance>>& Pair : QuestInstancesByAssetId)
    {
        Result.Add(Pair.Value.Get());
    }

    return Result;
}

int32 USFQuestRuntime::GetLifetimeTaskCount(FGameplayTag TaskTag, FName ContextId) const
{
    return LifetimeTaskCounts.FindRef(FSFTaskProgressKey{ TaskTag, ContextId });
}

//
// Save/load
//

FSFNarrativeSaveData USFQuestRuntime::BuildSaveData() const
{
    FSFNarrativeSaveData SaveData;

    for (const TPair<FPrimaryAssetId, TObjectPtr<USFQuestInstance>>& Pair : QuestInstancesByAssetId)
    {
        if (const USFQuestInstance* QuestInstance = Pair.Value)
        {
            SaveData.QuestSnapshots.Add(QuestInstance->BuildSnapshot());
        }
    }

    for (const TPair<FSFTaskProgressKey, int32>& Pair : LifetimeTaskCounts)
    {
        FSFTaskCounterSnapshot CounterSnapshot;
        CounterSnapshot.Key = Pair.Key;
        CounterSnapshot.Count = Pair.Value;
        SaveData.LifetimeTaskCounters.Add(CounterSnapshot);
    }

    return SaveData;
}

bool USFQuestRuntime::LoadFromSaveData(const FSFNarrativeSaveData& SaveData)
{
    if (!OwnerComponent)
    {
        return false;
    }

    QuestInstancesByAssetId.Reset();
    LifetimeTaskCounts.Reset();

    for (const FSFTaskCounterSnapshot& CounterSnapshot : SaveData.LifetimeTaskCounters)
    {
        LifetimeTaskCounts.Add(CounterSnapshot.Key, CounterSnapshot.Count);
    }

    for (const FSFQuestSnapshot& QuestSnapshot : SaveData.QuestSnapshots)
    {
        USFQuestDefinition* QuestDef = ResolveQuestDefinitionByAssetId(QuestSnapshot.QuestAssetId);
        if (!QuestDef)
        {
            continue;
        }

        USFQuestInstance* QuestInstance = NewObject<USFQuestInstance>(this);
        if (!QuestInstance || !QuestInstance->RestoreFromSnapshot(OwnerComponent, QuestDef, QuestSnapshot))
        {
            continue;
        }

        QuestInstancesByAssetId.Add(QuestSnapshot.QuestAssetId, QuestInstance);
    }

    return true;
}

//
// Helpers
//

USFQuestDefinition* USFQuestRuntime::ResolveQuestDefinitionFromClass(TSubclassOf<USFQuestDefinition> QuestDefClass) const
{
    return QuestDefClass ? QuestDefClass.GetDefaultObject() : nullptr;
}

USFQuestDefinition* USFQuestRuntime::ResolveQuestDefinitionByAssetId(FPrimaryAssetId QuestAssetId) const
{
    if (!QuestAssetId.IsValid())
    {
        return nullptr;
    }

    return Cast<USFQuestDefinition>(UAssetManager::Get().GetPrimaryAssetObject(QuestAssetId));
}

FPrimaryAssetId USFQuestRuntime::GetQuestAssetId(const USFQuestDefinition* QuestDef) const
{
    return QuestDef ? QuestDef->GetPrimaryAssetId() : FPrimaryAssetId();
}

void USFQuestRuntime::IncrementLifetimeTaskCount(FGameplayTag TaskTag, FName ContextId, int32 Quantity)
{
    if (!TaskTag.IsValid() || Quantity <= 0)
    {
        return;
    }

    FSFTaskProgressKey Key;
    Key.TaskTag = TaskTag;
    Key.ContextId = ContextId;
    LifetimeTaskCounts.FindOrAdd(Key) += Quantity;
}