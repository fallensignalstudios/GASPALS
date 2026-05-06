#include "SFNarrativeWorldState.h"
#include "SFNarrativeStateSubsystem.h"

FSFNarrativeWorldState USFNarrativeWorldStateLibrary::BuildWorldState(
    const UObject* WorldContextObject,
    const FSFNarrativeFlagMap& GlobalFlags,
    const FSFNarrativeVariableMap& GlobalVariables)
{
    FSFNarrativeWorldState Result;

    if (!WorldContextObject)
    {
        return Result;
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return Result;
    }

    if (USFNarrativeStateSubsystem* StateSubsystem = World->GetSubsystem<USFNarrativeStateSubsystem>())
    {
        // Reuse the subsystem's save builder for core world state.
        const FSFNarrativeSaveData SaveData = StateSubsystem->BuildSaveData();

        Result.WorldFacts = SaveData.WorldFacts;
        Result.FactionSnapshots = SaveData.FactionSnapshots;
        Result.IdentitySnapshots = SaveData.IdentitySnapshots;
        Result.EndingSnapshots = SaveData.EndingSnapshots;
    }

    // Mirror flags/variables into the snapshot directly; caller decides
    // whether/how to map them into world facts.
    Result.GlobalFlags = GlobalFlags;
    Result.GlobalVariables = GlobalVariables;

    return Result;
}

bool USFNarrativeWorldStateLibrary::ApplyWorldState(
    const UObject* WorldContextObject,
    const FSFNarrativeWorldState& WorldState,
    FSFNarrativeFlagMap& OutGlobalFlags,
    FSFNarrativeVariableMap& OutGlobalVariables)
{
    if (!WorldContextObject)
    {
        return false;
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return false;
    }

    if (USFNarrativeStateSubsystem* StateSubsystem = World->GetSubsystem<USFNarrativeStateSubsystem>())
    {
        FSFNarrativeSaveData SaveData;
        SaveData.WorldFacts = WorldState.WorldFacts;
        SaveData.FactionSnapshots = WorldState.FactionSnapshots;
        SaveData.IdentitySnapshots = WorldState.IdentitySnapshots;
        SaveData.EndingSnapshots = WorldState.EndingSnapshots;

        StateSubsystem->LoadFromSaveData(SaveData);
    }

    OutGlobalFlags = WorldState.GlobalFlags;
    OutGlobalVariables = WorldState.GlobalVariables;

    return true;
}