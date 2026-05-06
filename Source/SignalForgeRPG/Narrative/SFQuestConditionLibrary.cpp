#include "SFQuestConditionLibrary.h"
#include "SFQuestDefinition.h"

bool USFQuestConditionLibrary::AreAllStateTasksComplete(
    const FSFQuestStateDefinition& StateDef,
    const TMap<FName, int32>& TaskProgressByTaskId)
{
    for (const FSFQuestTaskDefinition& TaskDef : StateDef.Tasks)
    {
        if (!IsTaskComplete(TaskDef, TaskProgressByTaskId))
        {
            return false;
        }
    }

    return true;
}

bool USFQuestConditionLibrary::IsTaskComplete(
    const FSFQuestTaskDefinition& TaskDef,
    const TMap<FName, int32>& TaskProgressByTaskId)
{
    if (TaskDef.TaskId.IsNone())
    {
        return true; // Treat unnamed tasks as non-blocking.
    }

    const int32* FoundProgress = TaskProgressByTaskId.Find(TaskDef.TaskId);
    const int32 Progress = FoundProgress ? *FoundProgress : 0;

    return Progress >= TaskDef.RequiredQuantity;
}

bool USFQuestConditionLibrary::IsObjectiveTasksComplete(
    const FSFQuestObjectiveDefinition& ObjectiveDef,
    const TMap<FName, int32>& TaskProgressByTaskId)
{
    if (ObjectiveDef.Tasks.Num() == 0)
    {
        // A purely narrative objective is considered complete in task terms.
        return true;
    }

    for (const FSFQuestTaskDefinition& TaskDef : ObjectiveDef.Tasks)
    {
        if (!IsTaskComplete(TaskDef, TaskProgressByTaskId))
        {
            return false;
        }
    }

    return true;
}

bool USFQuestConditionLibrary::IsStepTasksComplete(
    const FSFQuestStepDefinition& StepDef,
    const TMap<FName, int32>& TaskProgressByTaskId)
{
    if (StepDef.Tasks.Num() == 0)
    {
        return true;
    }

    for (const FSFQuestTaskDefinition& TaskDef : StepDef.Tasks)
    {
        if (!IsTaskComplete(TaskDef, TaskProgressByTaskId))
        {
            return false;
        }
    }

    return true;
}

bool USFQuestConditionLibrary::ArePrimaryObjectivesComplete(
    const TArray<FSFQuestObjectiveDefinition>& Objectives,
    const TMap<FName, int32>& TaskProgressByTaskId)
{
    for (const FSFQuestObjectiveDefinition& Objective : Objectives)
    {
        if (Objective.Role != ESFQuestObjectiveRole::Primary)
        {
            continue;
        }

        if (!IsObjectiveTasksComplete(Objective, TaskProgressByTaskId))
        {
            return false;
        }
    }

    return true;
}

bool USFQuestConditionLibrary::EvaluateQuestStateCondition(
    const FSFNarrativeQuestStateCondition& Condition,
    const USFQuestDefinition* QuestDef,
    const FSFQuestSnapshot& Snapshot)
{
    if (!QuestDef)
    {
        return false;
    }

    // If a specific quest is referenced and it doesn't match, condition fails.
    if (Condition.QuestAssetId.IsValid())
    {
        if (Snapshot.QuestAssetId != Condition.QuestAssetId)
        {
            return false;
        }
    }

    if (Condition.QuestId != NAME_None)
    {
        const FPrimaryAssetId ExpectedId(TEXT("Quest"), Condition.QuestId);
        if (Snapshot.QuestAssetId != ExpectedId)
        {
            return false;
        }
    }

    // Completion states.
    if (Condition.AllowedCompletionStates.Num() > 0)
    {
        bool bAllowed = false;
        for (ESFQuestCompletionState Allowed : Condition.AllowedCompletionStates)
        {
            if (Snapshot.CompletionState == Allowed)
            {
                bAllowed = true;
                break;
            }
        }

        if (!bAllowed)
        {
            return false;
        }
    }

    // Current state requirement.
    if (Condition.RequiredCurrentStateId != NAME_None)
    {
        if (Snapshot.CurrentStateId != Condition.RequiredCurrentStateId)
        {
            return false;
        }
    }

    // Reached state requirement.
    if (Condition.RequiredReachedStateId != NAME_None)
    {
        if (!Snapshot.ReachedStateIds.Contains(Condition.RequiredReachedStateId))
        {
            return false;
        }
    }

    // Outcome tag requirement.
    if (Condition.RequiredOutcomeTag.IsValid())
    {
        if (!Snapshot.OutcomeTags.Contains(Condition.RequiredOutcomeTag))
        {
            return false;
        }
    }

    return true;
}

bool USFQuestConditionLibrary::EvaluateQuestStateConditionsInSet(
    const FSFNarrativeConditionSet& ConditionSet,
    const USFQuestDefinition* QuestDef,
    const FSFQuestSnapshot& Snapshot)
{
    // AllConditions (QuestState domain only).
    for (const FSFNarrativeCondition& Condition : ConditionSet.AllConditions)
    {
        if (Condition.Domain != ESFNarrativeConditionDomain::QuestState)
        {
            continue; // Defer non-quest conditions to the narrative subsystem.
        }

        const bool bPass = EvaluateQuestStateCondition(Condition.QuestState, QuestDef, Snapshot);
        if (Condition.bNegate ? bPass : !bPass)
        {
            return false;
        }
    }

    // AnyConditions: at least one QuestState-domain condition must pass, if any exist.
    bool bHasAnyQuestConditions = false;
    bool bAnyPassed = false;

    for (const FSFNarrativeCondition& Condition : ConditionSet.AnyConditions)
    {
        if (Condition.Domain != ESFNarrativeConditionDomain::QuestState)
        {
            continue;
        }

        bHasAnyQuestConditions = true;

        const bool bPass = EvaluateQuestStateCondition(Condition.QuestState, QuestDef, Snapshot);
        if (Condition.bNegate ? !bPass : bPass)
        {
            bAnyPassed = true;
            break;
        }
    }

    if (bHasAnyQuestConditions && !bAnyPassed)
    {
        return false;
    }

    // NoneConditions: all QuestState-domain conditions must fail.
    for (const FSFNarrativeCondition& Condition : ConditionSet.NoneConditions)
    {
        if (Condition.Domain != ESFNarrativeConditionDomain::QuestState)
        {
            continue;
        }

        const bool bPass = EvaluateQuestStateCondition(Condition.QuestState, QuestDef, Snapshot);
        if (Condition.bNegate ? !bPass : bPass)
        {
            return false;
        }
    }

    return true;
}

bool USFQuestConditionLibrary::IsStateLogicallyComplete(
    const FSFQuestStateDefinition& StateDef,
    const FSFQuestSnapshot& Snapshot)
{
    const int32* Found = Snapshot.TaskProgressByTaskId.Find(StateDef.StateId);
    // This assumes your task IDs are unique per state; if you instead key
    // by TaskId, you may want to use AreAllStateTasksComplete and pass
    // Snapshot.TaskProgressByTaskId directly. For now we default to that:
    return AreAllStateTasksComplete(StateDef, Snapshot.TaskProgressByTaskId);
}

ESFQuestStateViewStatus USFQuestConditionLibrary::ComputeStateViewStatus(
    const FSFQuestStateDefinition& StateDef,
    const FSFQuestSnapshot& Snapshot)
{
    const bool bIsCurrent = (Snapshot.CurrentStateId == StateDef.StateId);
    const bool bHasBeenReached = Snapshot.ReachedStateIds.Contains(StateDef.StateId);

    if (bIsCurrent)
    {
        return ESFQuestStateViewStatus::Active;
    }

    if (StateDef.bIsSuccessState)
    {
        return bHasBeenReached ? ESFQuestStateViewStatus::Completed : ESFQuestStateViewStatus::Available;
    }

    if (StateDef.bIsFailureState)
    {
        return bHasBeenReached ? ESFQuestStateViewStatus::Failed : ESFQuestStateViewStatus::Locked;
    }

    if (bHasBeenReached)
    {
        // A non-terminal state that has been visited.
        return ESFQuestStateViewStatus::Completed;
    }

    // Simple heuristic: if the snapshot’s current state is earlier in the
    // States array, the next few states are “Available”; farther ones are “Locked”.
    // For now we default to Locked/Available based purely on whether the quest
    // has started and we have any current state.
    if (Snapshot.CurrentStateId != NAME_None)
    {
        return ESFQuestStateViewStatus::Available;
    }

    return ESFQuestStateViewStatus::Locked;
}