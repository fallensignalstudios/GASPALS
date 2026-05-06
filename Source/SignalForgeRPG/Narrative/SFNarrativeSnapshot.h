#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SFNarrativeFlagMap.h"
#include "SFNarrativeVariableMap.h"
#include "SFNarrativeSaveTypes.h"
#include "SFNarrativeSnapshot.generated.h"

/**
 * Scoped narrative snapshot for a single owner (component, subsystem, session).
 *
 * This is a structured alternative to FSFNarrativeOwnerSnapshot::PayloadData:
 *  - Flags      ? FSFNarrativeFlagMap
 *  - Variables  ? FSFNarrativeVariableMap
 *  - LocalFacts ? world-fact-style snapshots scoped to this owner
 *
 * You can either:
 *  - embed FSFNarrativeSnapshot directly into other save structs, or
 *  - serialize it into FSFNarrativeOwnerSnapshot::PayloadData if you want a
 *    versioned binary format later.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeSnapshot
{
    GENERATED_BODY()

    /** Identifier for the owner this snapshot belongs to (component, actor, system). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Snapshot")
    FName OwnerId = NAME_None;

    /** Optional classification tags (e.g. Narrative.Owner.Player, Narrative.Owner.Dialogue). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Snapshot")
    FGameplayTagContainer OwnerTags;

    /** Local boolean flags (e.g. per-owner gates, seen-events). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Snapshot")
    FSFNarrativeFlagMap LocalFlags;

    /** Local scalar variables (ints/floats/bools/names/tags). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Snapshot")
    FSFNarrativeVariableMap LocalVariables;

    /**
     * Optional local facts associated with this owner. These use the same
     * shape as world facts, but you can scope them with a dedicated FactTag
     * and ContextId that includes OwnerId.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Snapshot")
    TArray<FSFWorldFactSnapshot> LocalFacts;

    //
    // Helpers
    //

    /** True if there is no local state to restore. */
    bool IsEmpty() const
    {
        return LocalFlags.Flags.Num() == 0
            && LocalVariables.Variables.Num() == 0
            && LocalFacts.Num() == 0;
    }

    /** Clear all local state. */
    void Reset()
    {
        LocalFlags.Reset();
        LocalVariables.Reset();
        LocalFacts.Reset();
    }

    /** Build fact snapshots from flags + variables using a base tag/context. */
    void BuildLocalFactsFromFlagsAndVariables(
        const FGameplayTag& FlagFactTag,
        const FGameplayTag& VariableFactTag,
        FName ContextIdBase)
    {
        LocalFacts.Reset();

        // Flags ? facts
        if (FlagFactTag.IsValid())
        {
            TArray<FSFWorldFactSnapshot> FlagSnapshots;
            LocalFlags.BuildSnapshot(FlagSnapshots, FlagFactTag, ContextIdBase);
            LocalFacts.Append(FlagSnapshots);
        }

        // Variables ? facts
        if (VariableFactTag.IsValid())
        {
            TArray<FSFWorldFactSnapshot> VarSnapshots;
            LocalVariables.BuildSnapshots(VarSnapshots, VariableFactTag, ContextIdBase);
            LocalFacts.Append(VarSnapshots);
        }
    }
};