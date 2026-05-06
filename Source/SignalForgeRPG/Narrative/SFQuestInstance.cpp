// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "SFQuestInstance.h"

#include "SFNarrativeComponent.h"
#include "SFNarrativeEventHub.h"
#include "SFQuestDefinition.h"

bool USFQuestInstance::Initialize(USFNarrativeComponent* InOwner, const USFQuestDefinition* InDefinition, FName StartStateId)
{
    OwnerComponent = InOwner;
    Definition = InDefinition;
    CompletionState = ESFQuestCompletionState::NotStarted;
    CurrentStateId = NAME_None;
    ReachedStateIds.Reset();
    TaskProgressByTaskId.Reset();

    if (!OwnerComponent || !Definition)
    {
        return false;
    }

    const FName InitialStateId = !StartStateId.IsNone() ? StartStateId : Definition->InitialStateId;
    if (InitialStateId.IsNone())
    {
        return false;
    }

    return EnterState(InitialStateId, false);
}

bool USFQuestInstance::RestoreFromSnapshot(USFNarrativeComponent* InOwner, const USFQuestDefinition* InDefinition, const FSFQuestSnapshot& Snapshot)
{
    OwnerComponent = InOwner;
    Definition = InDefinition;

    if (!OwnerComponent || !Definition)
    {
        return false;
    }

    CompletionState = Snapshot.CompletionState;
    CurrentStateId = Snapshot.CurrentStateId;
    ReachedStateIds = Snapshot.ReachedStateIds;
    TaskProgressByTaskId = Snapshot.TaskProgressByTaskId;
    return true;
}

bool USFQuestInstance::EnterState(FName StateId, bool bFromReplication)
{
    const FSFQuestStateDefinition* StateDef = FindStateDefinition(StateId);
    if (!StateDef)
    {
        return false;
    }

    CurrentStateId = StateId;
    ReachedStateIds.AddUnique(StateId);

    if (CompletionState == ESFQuestCompletionState::NotStarted)
    {
        CompletionState = ESFQuestCompletionState::InProgress;
    }

    RefreshCompletionStateFromStateDefinition(*StateDef);

    if (OwnerComponent)
    {
        if (USFNarrativeEventHub* Hub = OwnerComponent->GetEventHub())
        {
            Hub->OnQuestStateChanged.Broadcast(this, StateId);
        }
    }

    return true;
}

bool USFQuestInstance::ApplyTaskProgress(FGameplayTag TaskTag, FName ContextId, int32 Quantity, bool bFromReplication)
{
    if (!Definition || !TaskTag.IsValid() || Quantity <= 0)
    {
        return false;
    }

    const FSFQuestStateDefinition* StateDef = FindStateDefinition(CurrentStateId);
    if (!StateDef)
    {
        return false;
    }

    bool bChanged = false;
    for (const FSFQuestTaskDefinition& TaskDef : StateDef->Tasks)
    {
        if (TaskDef.TaskTag != TaskTag)
        {
            continue;
        }

        if (!TaskDef.ExpectedContextId.IsNone() && TaskDef.ExpectedContextId != ContextId)
        {
            continue;
        }

        const int32 OldValue = TaskProgressByTaskId.FindRef(TaskDef.TaskId);
        const int32 NewValue = OldValue + Quantity;
        TaskProgressByTaskId.FindOrAdd(TaskDef.TaskId) = NewValue;
        bChanged = true;

        if (OwnerComponent)
        {
            if (USFNarrativeEventHub* Hub = OwnerComponent->GetEventHub())
            {
                Hub->OnQuestTaskProgressed.Broadcast(this, TaskDef.TaskTag, ContextId, NewValue);
            }
        }
    }

    if (bChanged && AreAllTasksCompleteInState(*StateDef))
    {
        RefreshCompletionStateFromStateDefinition(*StateDef);
    }

    return bChanged;
}

bool USFQuestInstance::ApplyReplicatedTaskProgressByTaskId(FName TaskId, int32 NewProgress, int32 DeltaAmount)
{
    if (!Definition || TaskId.IsNone() || NewProgress < 0)
    {
        return false;
    }

    const FSFQuestStateDefinition* StateDef = FindStateDefinition(CurrentStateId);
    if (!StateDef)
    {
        return false;
    }

    const FSFQuestTaskDefinition* TaskDef = StateDef->Tasks.FindByPredicate(
        [TaskId](const FSFQuestTaskDefinition& Candidate)
        {
            return Candidate.TaskId == TaskId;
        });

    if (!TaskDef)
    {
        return false;
    }

    TaskProgressByTaskId.FindOrAdd(TaskId) = NewProgress;

    if (OwnerComponent)
    {
        if (USFNarrativeEventHub* Hub = OwnerComponent->GetEventHub())
        {
            Hub->OnQuestTaskProgressed.Broadcast(this, TaskDef->TaskTag, TaskDef->ExpectedContextId, NewProgress);
        }
    }

    if (AreAllTasksCompleteInState(*StateDef))
    {
        RefreshCompletionStateFromStateDefinition(*StateDef);
    }

    return true;
}

void USFQuestInstance::MarkAbandoned()
{
    CompletionState = ESFQuestCompletionState::Abandoned;
}

bool USFQuestInstance::IsFinished() const
{
    return CompletionState == ESFQuestCompletionState::Succeeded
        || CompletionState == ESFQuestCompletionState::Failed
        || CompletionState == ESFQuestCompletionState::Abandoned;
}

bool USFQuestInstance::IsSucceeded() const
{
    return CompletionState == ESFQuestCompletionState::Succeeded;
}

bool USFQuestInstance::IsFailed() const
{
    return CompletionState == ESFQuestCompletionState::Failed;
}

ESFQuestCompletionState USFQuestInstance::GetCompletionState() const
{
    return CompletionState;
}

FName USFQuestInstance::GetCurrentStateId() const
{
    return CurrentStateId;
}

const USFQuestDefinition* USFQuestInstance::GetDefinition() const
{
    return Definition;
}

const TMap<FName, int32>& USFQuestInstance::GetTaskProgressByTaskId() const
{
    return TaskProgressByTaskId;
}

FSFQuestSnapshot USFQuestInstance::BuildSnapshot() const
{
    FSFQuestSnapshot Snapshot;
    if (Definition)
    {
        Snapshot.QuestAssetId = Definition->GetPrimaryAssetId();
    }
    Snapshot.CompletionState = CompletionState;
    Snapshot.CurrentStateId = CurrentStateId;
    Snapshot.ReachedStateIds = ReachedStateIds;
    Snapshot.TaskProgressByTaskId = TaskProgressByTaskId;
    return Snapshot;
}

const FSFQuestStateDefinition* USFQuestInstance::FindStateDefinition(FName StateId) const
{
    if (!Definition || StateId.IsNone())
    {
        return nullptr;
    }

    return Definition->States.FindByPredicate(
        [StateId](const FSFQuestStateDefinition& State)
        {
            return State.StateId == StateId;
        });
}

bool USFQuestInstance::AreAllTasksCompleteInState(const FSFQuestStateDefinition& StateDef) const
{
    for (const FSFQuestTaskDefinition& TaskDef : StateDef.Tasks)
    {
        if (GetTaskProgress(TaskDef.TaskId) < TaskDef.RequiredQuantity)
        {
            return false;
        }
    }

    return true;
}

void USFQuestInstance::RefreshCompletionStateFromStateDefinition(const FSFQuestStateDefinition& StateDef)
{
    if (StateDef.bIsSuccessState)
    {
        CompletionState = ESFQuestCompletionState::Succeeded;
    }
    else if (StateDef.bIsFailureState)
    {
        CompletionState = ESFQuestCompletionState::Failed;
    }
    else if (CompletionState == ESFQuestCompletionState::NotStarted)
    {
        CompletionState = ESFQuestCompletionState::InProgress;
    }
}

int32 USFQuestInstance::GetTaskProgress(FName TaskId) const
{
    return TaskProgressByTaskId.FindRef(TaskId);
}
