#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"

#include "SFNarrativeSaveTypes.h"
#include "SFNarrativeSnapshot.h"
#include "SFNarrativeFlagMap.h"
#include "SFNarrativeVariableMap.h"
#include "SFNarrativeRestoreHelpers.generated.h"

class USFNarrativeStateSubsystem;
class USFQuestDefinition;
class USFQuestInstance;
class USFQuestRuntime;

/**
 * Utility helpers for restoring narrative state from save data.
 *
 * These are intentionally stateless and do not own any data; they just
 * translate FSFNarrativeSaveData / snapshots back into live objects.
 */
UCLASS()
class SIGNALFORGERPG_API USFNarrativeRestoreHelpers : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    //
    // World-level restore helpers
    //

    /** Apply the world portion of FSFNarrativeSaveData to the state subsystem. */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Restore", meta = (WorldContext = "WorldContextObject"))
    static bool RestoreWorldFromSaveData(
        const UObject* WorldContextObject,
        const FSFNarrativeSaveData& SaveData);

    //
    // Quest restore helpers
    //

    /** Restore a single quest instance from its snapshot. */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Restore")
    static bool RestoreQuestInstanceFromSnapshot(
        const FSFQuestSnapshot& Snapshot,
        const USFQuestDefinition* QuestDef,
        USFQuestInstance* QuestInstance);

    /** Restore all quest instances into a quest runtime from save data. */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Restore")
    static void RestoreQuestRuntimeFromSaveData(
        const FSFNarrativeSaveData& SaveData,
        USFQuestRuntime* QuestRuntime);

    //
    // Lifetime task counters
    //

    /** Rebuild the lifetime task counter map from snapshots. */
    template<typename TMapType>
    static void RestoreLifetimeTaskCounters(
        const TArray<FSFTaskCounterSnapshot>& Snapshots,
        TMapType& OutCounterMap)
    {
        OutCounterMap.Reset();
        for (const FSFTaskCounterSnapshot& Snapshot : Snapshots)
        {
            OutCounterMap.Add(Snapshot.Key, Snapshot.Count);
        }
    }

    //
    // Owner-snapshot helpers
    //

    /** Decode a structured FSFNarrativeSnapshot from an owner snapshot payload (if you choose to store it that way). */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Restore")
    static bool DecodeNarrativeSnapshotFromOwnerPayload(
        const FSFNarrativeOwnerSnapshot& OwnerSnapshot,
        FSFNarrativeSnapshot& OutSnapshot);

    /** Encode a structured snapshot into an owner snapshot payload. */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Restore")
    static void EncodeNarrativeSnapshotToOwnerPayload(
        const FSFNarrativeSnapshot& Snapshot,
        FSFNarrativeOwnerSnapshot& OutOwnerSnapshot);

    /** Convenience: restore flags + variables directly from a structured snapshot. */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Restore")
    static void RestoreFlagsAndVariablesFromSnapshot(
        const FSFNarrativeSnapshot& Snapshot,
        FSFNarrativeFlagMap& OutFlags,
        FSFNarrativeVariableMap& OutVariables);
};