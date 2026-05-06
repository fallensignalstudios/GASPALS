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
    OutChangeSet.Reset();

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
    OutCombinedChangeSet.Reset();

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
            OutCombinedChangeSet.WorldFactChanges.Append(LocalChangeSet.WorldFactChanges);
            OutCombinedChangeSet.FactionChanges.Append(LocalChangeSet.FactionChanges);
            OutCombinedChangeSet.IdentityAxisChanges.Append(LocalChangeSet.IdentityAxisChanges);
            OutCombinedChangeSet.EndingChanges.Append(LocalChangeSet.EndingChanges);
            OutCombinedChangeSet.CustomChanges.Append(LocalChangeSet.CustomChanges);
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

    FSFWorldFactValue NewValue;
    NewValue.Type = ESFNarrativeFactValueType::Bool;
    NewValue.BoolValue = bValue;

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
    if (!State->GetWorldFact(Key, Value) || Value.Type != ESFNarrativeFactValueType::Bool)
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