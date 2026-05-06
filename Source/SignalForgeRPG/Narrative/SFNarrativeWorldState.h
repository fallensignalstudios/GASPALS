// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeFlagMap.h"
#include "SFNarrativeVariableMap.h"
#include "SFNarrativeWorldState.generated.h"

/**
 * Immutable snapshot of world-level narrative state at a point in time.
 *
 * This is designed as a pure data container that can be:
 *  - built from USFNarrativeStateSubsystem (and friends),
 *  - serialized inside FSFNarrativeSaveData or your save game class,
 *  - and later used to restore the world narrative state.
 *
 * It intentionally mirrors the structure of FSFNarrativeSaveData but is
 * scoped to "world" rather than "per-owner" (quests, dialogue, etc.).
 */
USTRUCT(BlueprintType)
struct FSFNarrativeWorldState
{
    GENERATED_BODY()

    /** World facts (global key/value pairs). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|WorldState")
    TArray<FSFWorldFactSnapshot> WorldFacts;

    /** Global faction standings. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|WorldState")
    TArray<FSFFactionSnapshot> FactionSnapshots;

    /** Global identity axis values. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|WorldState")
    TArray<FSFIdentityAxisValue> IdentityAxes;

    /** Global ending scores + availability. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|WorldState")
    TArray<FSFEndingState> EndingStates;

    /** Optional global boolean flags (lightweight structural state). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|WorldState")
    FSFNarrativeFlagMap GlobalFlags;

    /** Optional global scalar variables (ints/floats/bools/names/tags). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|WorldState")
    FSFNarrativeVariableMap GlobalVariables;

    //
    // Helpers
    //

    /** True if everything is empty / default-initialized. */
    bool IsEmpty() const
    {
        return WorldFacts.Num() == 0
            && FactionSnapshots.Num() == 0
            && IdentityAxes.Num() == 0
            && EndingStates.Num() == 0
            && GlobalFlags.Flags.Num() == 0
            && GlobalVariables.Variables.Num() == 0;
    }

    /** Clear all stored state. */
    void Reset()
    {
        WorldFacts.Reset();
        FactionSnapshots.Reset();
        IdentityAxes.Reset();
        EndingStates.Reset();
        GlobalFlags.Reset();
        GlobalVariables.Reset();
    }
};

/**
 * Utility functions for building/restoring FSFNarrativeWorldState from the
 * live subsystems. Kept header-only for convenience.
 */
UCLASS()
class SIGNALFORGERPG_API USFNarrativeWorldStateLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /** Build a world-state snapshot from the given world + optional global flags/vars. */
    UFUNCTION(BlueprintCallable, Category = "Narrative|WorldState", meta = (WorldContext = "WorldContextObject"))
    static FSFNarrativeWorldState BuildWorldState(
        const UObject* WorldContextObject,
        const FSFNarrativeFlagMap& GlobalFlags,
        const FSFNarrativeVariableMap& GlobalVariables);

    /** Apply a world-state snapshot back into the narrative state subsystem. */
    UFUNCTION(BlueprintCallable, Category = "Narrative|WorldState", meta = (WorldContext = "WorldContextObject"))
    static bool ApplyWorldState(
        const UObject* WorldContextObject,
        const FSFNarrativeWorldState& WorldState,
        FSFNarrativeFlagMap& OutGlobalFlags,
        FSFNarrativeVariableMap& OutGlobalVariables);
};
