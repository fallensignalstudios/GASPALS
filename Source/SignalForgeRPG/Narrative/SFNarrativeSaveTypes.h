#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/PrimaryAssetId.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeSaveTypes.generated.h"

/**
 * A single world fact at save time.
 * (Key + current value)
 */
USTRUCT(BlueprintType)
struct FSFWorldFactSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|World")
    FSFWorldFactKey Key;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|World")
    FSFWorldFactValue Value;
};

/**
 * Snapshot of a single faction's standing.
 */
USTRUCT(BlueprintType)
struct FSFFactionStandingSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|Faction")
    FGameplayTag FactionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|Faction")
    FSFFactionStandingValue Standing;
};

/**
 * Snapshot of a single identity axis.
 */
USTRUCT(BlueprintType)
struct FSFIdentityAxisSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|Identity")
    FGameplayTag AxisTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|Identity")
    float Value = 0.0f;
};

/**
 * Snapshot of a single ending's state.
 */
USTRUCT(BlueprintType)
struct FSFEndingSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|Ending")
    FGameplayTag EndingTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|Ending")
    FSFEndingState State;
};

/**
 * Snapshot of a quest instance for saving/loading.
 *
 * This is the type used by USFQuestInstance::BuildSnapshot/RestoreFromSnapshot.
 */
USTRUCT(BlueprintType)
struct FSFQuestSnapshot
{
    GENERATED_BODY()

    /** Which quest asset this snapshot belongs to. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|Quest")
    FPrimaryAssetId QuestAssetId;

    /** Current completion state. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|Quest")
    ESFQuestCompletionState CompletionState = ESFQuestCompletionState::NotStarted;

    /** Current state ID. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|Quest")
    FName CurrentStateId = NAME_None;

    /** All states that have been reached at least once. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|Quest")
    TArray<FName> ReachedStateIds;

    /** Per-task progress keyed by TaskId. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|Quest")
    TMap<FName, int32> TaskProgressByTaskId;

    /** Optional narrative outcome tags applied by this quest. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|Quest")
    FGameplayTagContainer OutcomeTags;
};

/**
 * Lifetime task counter key: identifies a specific task across all quests
 * for aggregate tracking (e.g. "killed 50 wolves ever").
 *
 * This is what USFQuestRuntime uses as a map key.
 */
USTRUCT(BlueprintType)
struct FSFTaskProgressKey
{
    GENERATED_BODY()

    /** Task tag, e.g. Narrative.Task.Kill.Wolf. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|Quest")
    FGameplayTag TaskTag;

    /** Optional context id (zone, NPC, quest, etc.). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|Quest")
    FName ContextId = NAME_None;

    friend bool operator==(const FSFTaskProgressKey& A, const FSFTaskProgressKey& B)
    {
        return A.TaskTag == B.TaskTag && A.ContextId == B.ContextId;
    }
};

FORCEINLINE uint32 GetTypeHash(const FSFTaskProgressKey& Key)
{
    return HashCombine(GetTypeHash(Key.TaskTag), GetTypeHash(Key.ContextId));
}

/**
 * Snapshot of a single lifetime task counter.
 */
USTRUCT(BlueprintType)
struct FSFTaskCounterSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|Quest")
    FSFTaskProgressKey Key;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|Quest")
    int32 Count = 0;
};

/**
 * Generic per-owner snapshot for narrative components or subsystems
 * that want to store additional serialized data.
 *
 * The concrete meaning of OwnerId / PayloadTags / PayloadData is up to
 * your game; this struct just gives you a standard container.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeOwnerSnapshot
{
    GENERATED_BODY()

    /** Identifier for the owner (player, character, subsystem, etc.). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|Owner")
    FName OwnerId = NAME_None;

    /** Tags describing what this payload contains (for routing / migration). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|Owner")
    FGameplayTagContainer PayloadTags;

    /** Opaque, versioned binary blob. Use your own serialization into this array. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|Owner")
    TArray<uint8> PayloadData;
};

/**
 * Aggregate narrative save data:
 *  - world-level facts / factions / identity / endings
 *  - quest instances and lifetime task counters
 *  - arbitrary per-owner snapshots
 *
 * This struct is used by:
 *  - USFQuestRuntime::BuildSaveData/LoadFromSaveData
 *  - USFNarrativeStateSubsystem::BuildSaveData/LoadFromSaveData
 *  - USFNarrativeSaveService and USFNarrativeSaveGame
 */
USTRUCT(BlueprintType)
struct FSFNarrativeSaveData
{
    GENERATED_BODY()

    //
    // World-level state
    //

    /** All world facts at time of save. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|World")
    TArray<FSFWorldFactSnapshot> WorldFacts;

    /** All faction standings. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|World")
    TArray<FSFFactionStandingSnapshot> FactionSnapshots;

    /** All identity axis values. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|World")
    TArray<FSFIdentityAxisSnapshot> IdentitySnapshots;

    /** All ending states. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|World")
    TArray<FSFEndingSnapshot> EndingSnapshots;

    //
    // Per-owner quest/task data
    //

    /** All active quest instances. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|Quest")
    TArray<FSFQuestSnapshot> QuestSnapshots;

    /** Aggregate lifetime task counters. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|Quest")
    TArray<FSFTaskCounterSnapshot> LifetimeTaskCounters;

    //
    // Extensibility hook
    //

    /** Arbitrary owner-scoped narrative payloads. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save|Owner")
    TArray<FSFNarrativeOwnerSnapshot> CustomOwnerSnapshots;

    /** True if there is nothing to serialize. */
    bool IsEmpty() const
    {
        return WorldFacts.Num() == 0
            && FactionSnapshots.Num() == 0
            && IdentitySnapshots.Num() == 0
            && EndingSnapshots.Num() == 0
            && QuestSnapshots.Num() == 0
            && LifetimeTaskCounters.Num() == 0
            && CustomOwnerSnapshots.Num() == 0;
    }

    /** Reset all arrays. */
    void Reset()
    {
        WorldFacts.Reset();
        FactionSnapshots.Reset();
        IdentitySnapshots.Reset();
        EndingSnapshots.Reset();
        QuestSnapshots.Reset();
        LifetimeTaskCounters.Reset();
        CustomOwnerSnapshots.Reset();
    }
};