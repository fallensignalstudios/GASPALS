// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "SFNarrativeBlueprintLibrary.h"

#include "SFNarrativeStateSubsystem.h"
#include "SFNarrativeFactSubsystem.h"
#include "SFNarrativeStateQueryLibrary.h"

USFNarrativeStateSubsystem* USFNarrativeBlueprintLibrary::GetNarrativeState(
    const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    UWorld* World = WorldContextObject->GetWorld();
    return World ? World->GetSubsystem<USFNarrativeStateSubsystem>() : nullptr;
}

USFNarrativeFactSubsystem* USFNarrativeBlueprintLibrary::GetNarrativeFacts(
    const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    UWorld* World = WorldContextObject->GetWorld();
    return World ? World->GetSubsystem<USFNarrativeFactSubsystem>() : nullptr;
}

bool USFNarrativeBlueprintLibrary::EvaluateNarrativeConditionSet(
    const UObject* WorldContextObject,
    const FSFNarrativeConditionSet& ConditionSet,
    const USFQuestDefinition* QuestDef,
    const USFQuestInstance* QuestInstance)
{
    return USFNarrativeStateQueryLibrary::EvaluateConditionSet(
        WorldContextObject,
        ConditionSet,
        QuestDef,
        QuestInstance);
}

bool USFNarrativeBlueprintLibrary::ApplyNarrativeDelta(
    const UObject* WorldContextObject,
    const FSFNarrativeDelta& Delta,
    FSFNarrativeChangeSet& OutChangeSet)
{
    OutChangeSet = FSFNarrativeChangeSet{};

    USFNarrativeStateSubsystem* State = GetNarrativeState(WorldContextObject);
    if (!State)
    {
        return false;
    }

    return State->ApplyDelta(Delta, OutChangeSet);
}

bool USFNarrativeBlueprintLibrary::ApplyNarrativeDeltas(
    const UObject* WorldContextObject,
    const TArray<FSFNarrativeDelta>& Deltas,
    FSFNarrativeChangeSet& OutCombinedChangeSet)
{
    OutCombinedChangeSet = FSFNarrativeChangeSet{};

    USFNarrativeStateSubsystem* State = GetNarrativeState(WorldContextObject);
    if (!State)
    {
        return false;
    }

    bool bAnyApplied = false;

    for (const FSFNarrativeDelta& Delta : Deltas)
    {
        FSFNarrativeChangeSet LocalChangeSet;
        if (State->ApplyDelta(Delta, LocalChangeSet))
        {
            bAnyApplied = true;

            // Merge LocalChangeSet into OutCombinedChangeSet.
            OutCombinedChangeSet.TaskResults.Append(LocalChangeSet.TaskResults);
            OutCombinedChangeSet.WorldFactChanges.Append(LocalChangeSet.WorldFactChanges);
            OutCombinedChangeSet.FactionChanges.Append(LocalChangeSet.FactionChanges);
            OutCombinedChangeSet.IdentityChanges.Append(LocalChangeSet.IdentityChanges);
            OutCombinedChangeSet.AppliedOutcomes.Append(LocalChangeSet.AppliedOutcomes);
            OutCombinedChangeSet.EndingStatesChanged.Append(LocalChangeSet.EndingStatesChanged);
            OutCombinedChangeSet.SourceTags.AppendTags(LocalChangeSet.SourceTags);
        }
    }

    return bAnyApplied;
}

void USFNarrativeBlueprintLibrary::SetBoolFact(
    const UObject* WorldContextObject,
    const FSFWorldFactKey& Key,
    bool bValue)
{
    USFNarrativeStateSubsystem* State = GetNarrativeState(WorldContextObject);
    if (!State)
    {
        return;
    }

    FSFWorldFactValue NewValue = FSFWorldFactValue::MakeBool(bValue);

    FSFWorldFactValue OldValue;
    State->SetWorldFact(Key, NewValue, OldValue);
}

bool USFNarrativeBlueprintLibrary::GetBoolFact(
    const UObject* WorldContextObject,
    const FSFWorldFactKey& Key,
    bool& bOutValue)
{
    bOutValue = false;

    USFNarrativeStateSubsystem* State = GetNarrativeState(WorldContextObject);
    if (!State)
    {
        return false;
    }

    FSFWorldFactValue Value;
    if (!State->GetWorldFact(Key, Value) || Value.ValueType != ESFNarrativeFactValueType::Bool)
    {
        return false;
    }

    bOutValue = Value.BoolValue;
    return true;
}

ESFFactionStandingBand USFNarrativeBlueprintLibrary::GetFactionBand(
    const UObject* WorldContextObject,
    FGameplayTag FactionTag)
{
    return USFNarrativeStateQueryLibrary::GetFactionStandingBand(
        WorldContextObject,
        FactionTag);
}

float USFNarrativeBlueprintLibrary::GetIdentityAxisValue(
    const UObject* WorldContextObject,
    FGameplayTag AxisTag,
    bool& bOutHasValue)
{
    return USFNarrativeStateQueryLibrary::GetIdentityAxisValue(
        WorldContextObject,
        AxisTag,
        bOutHasValue);
}
