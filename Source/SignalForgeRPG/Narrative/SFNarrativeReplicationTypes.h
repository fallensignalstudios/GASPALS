// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeSaveTypes.h"
#include "SFNarrativeReplicationTypes.generated.h"

/**
 * What kind of thing this delta modifies.
 */
UENUM(BlueprintType)
enum class ESFNarrativeReplicatedDeltaDomain : uint8
{
    None,

    WorldFact,
    Faction,
    IdentityAxis,
    Ending,

    QuestState,
    LifetimeTask,

    Custom
};

/**
 * How a numeric value should be applied.
 */
UENUM(BlueprintType)
enum class ESFNarrativeReplicatedNumericMode : uint8
{
    Set,
    Add
};

/**
 * A single, network-friendly narrative delta.
 *
 * This is the replicated form of FSFNarrativeDelta; it intentionally
 * carries only the data needed for clients to mirror the authoritative
 * state. Server code can convert between both types as needed.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeReplicatedDelta
{
    GENERATED_BODY()

    /** Domain of the change (fact, faction, quest, etc.). */
    UPROPERTY()
    ESFNarrativeReplicatedDeltaDomain Domain = ESFNarrativeReplicatedDeltaDomain::None;

    /** Generic tags / ids used per-domain. */
    UPROPERTY()
    FGameplayTag Tag0;          // e.g. FactTag, FactionTag, AxisTag, EndingTag, QuestTag
    UPROPERTY()
    FGameplayTag Tag1;          // optional secondary tag (e.g. TaskTag)
    UPROPERTY()
    FName Name0 = NAME_None;    // e.g. ContextId, StateId, TaskId
    UPROPERTY()
    FName Name1 = NAME_None;    // optional secondary name

    /** Numeric component. */
    UPROPERTY()
    ESFNarrativeReplicatedNumericMode NumericMode = ESFNarrativeReplicatedNumericMode::Set;

    UPROPERTY()
    float NumericValue = 0.0f;

    /** Bool component (for flags/facts/availability). */
    UPROPERTY()
    bool bBoolValue = false;

    /** Optional tags (e.g. outcome tags, tag facts). */
    UPROPERTY()
    FGameplayTagContainer TagContainerValue;

    /** Optional small integer payload (e.g. quest completion enum as uint8). */
    UPROPERTY()
    uint8 ByteValue0 = 0;

    /** True if this delta should also be recorded into local save state. */
    UPROPERTY()
    bool bSaveToHistory = true;
};

/**
 * Single replicated change entry used in a fast array.
 */
USTRUCT()
struct FSFNarrativeReplicatedDeltaItem : public FFastArraySerializerItem
{
    GENERATED_BODY()

    UPROPERTY()
    FSFNarrativeReplicatedDelta Delta;
};

/**
 * Array of narrative deltas for replication using FFastArraySerializer.
 *
 * The replicator component owns an instance of this and pushes deltas
 * into it; clients receive them and apply to their local state.
 */
USTRUCT()
struct FSFNarrativeReplicatedDeltaArray : public FFastArraySerializer
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FSFNarrativeReplicatedDeltaItem> Items;

    /** Owning actor/component, required by FFastArraySerializer. */
    UPROPERTY(NotReplicated)
    UObject* Owner = nullptr;

    FSFNarrativeReplicatedDeltaArray()
        : Owner(nullptr)
    {
    }

    FSFNarrativeReplicatedDeltaArray(UObject* InOwner)
        : Owner(InOwner)
    {
    }

    void AddDelta(const FSFNarrativeReplicatedDelta& InDelta)
    {
        FSFNarrativeReplicatedDeltaItem& Item = Items.Emplace_GetRef();
        Item.Delta = InDelta;

        // Mark for replication.
        MarkItemDirty(Item);
    }

    void ResetDeltas()
    {
        Items.Reset();
        MarkArrayDirty();
    }

    bool IsEmpty() const
    {
        return Items.Num() == 0;
    }

    // FFastArraySerializer overrides
    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FSFNarrativeReplicatedDeltaItem, FSFNarrativeReplicatedDeltaArray>(Items, DeltaParms, *this);
    }
};

template<>
struct TStructOpsTypeTraits<FSFNarrativeReplicatedDeltaArray> : public TStructOpsTypeTraitsBase2<FSFNarrativeReplicatedDeltaArray>
{
    enum
    {
        WithNetDeltaSerializer = true,
    };
};


/**
 * Minimal snapshot of world narrative state for initial replication
 * (e.g. when a client joins in-progress). This mirrors FSFNarrativeWorldState
 * but may be trimmed down if you only care about subsets for clients.
 */
USTRUCT()
struct FSFNarrativeReplicatedWorldSnapshot
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FSFWorldFactSnapshot> WorldFacts;

    UPROPERTY()
    TArray<FSFFactionSnapshot> FactionSnapshots;

    UPROPERTY()
    TArray<FSFIdentityAxisValue> IdentityAxes;

    UPROPERTY()
    TArray<FSFEndingState> EndingStates;

    bool IsEmpty() const
    {
        return WorldFacts.Num() == 0
            && FactionSnapshots.Num() == 0
            && IdentityAxes.Num() == 0
            && EndingStates.Num() == 0;
    }

    void Reset()
    {
        WorldFacts.Reset();
        FactionSnapshots.Reset();
        IdentityAxes.Reset();
        EndingStates.Reset();
    }
};

/**
 * Client-side change notification produced after applying a replicated delta.
 * Your replicator component can broadcast this to Blueprints/UI.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeClientChange
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Replication")
    FSFNarrativeReplicatedDelta AppliedDelta;

    /** Optional human-readable description, useful for UI/debug. */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Replication")
    FText DebugDescription;
};
