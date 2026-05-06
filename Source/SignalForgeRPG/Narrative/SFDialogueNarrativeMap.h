// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "SFNarrativeTypes.h"
#include "SFNarrativeSaveTypes.h"
#include "SFDialogueNarrativeMap.generated.h"

/**
 * Identifies a specific dialogue “address” that your dialogue system
 * can refer to when asking for narrative conditions/consequences.
 *
 * This should match whatever your dialogue tool exports:
 *  - ConversationId  ? top-level file / graph / conversation id
 *  - NodeId          ? specific line/choice node id within that graph
 *  - OptionIndex     ? specific choice index when a node has multiple options
 */
USTRUCT(BlueprintType)
struct FSFDialogueKey
{
    GENERATED_BODY()

    /** Conversation identifier as seen by your dialogue system/tool. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    FName ConversationId = NAME_None;

    /** Node identifier within that conversation (line, hub, choice node). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    FName NodeId = NAME_None;

    /** Optional choice index when this entry refers to a specific option. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    int32 OptionIndex = INDEX_NONE;

    bool IsValid() const
    {
        return !ConversationId.IsNone() && !NodeId.IsNone();
    }

    friend bool operator==(const FSFDialogueKey& A, const FSFDialogueKey& B)
    {
        return A.ConversationId == B.ConversationId
            && A.NodeId == B.NodeId
            && A.OptionIndex == B.OptionIndex;
    }
};

FORCEINLINE uint32 GetTypeHash(const FSFDialogueKey& Key)
{
    uint32 Hash = GetTypeHash(Key.ConversationId);
    Hash = HashCombine(Hash, GetTypeHash(Key.NodeId));
    Hash = HashCombine(Hash, GetTypeHash(Key.OptionIndex));
    return Hash;
}

/**
 * Narrative integration for a single dialogue “address”.
 *
 * - Conditions gate whether the line/choice is available.
 * - Deltas fire when the node/choice is taken/finished.
 * - Tags let you drive higher-level patterns (e.g. “romance +1”).
 */
USTRUCT(BlueprintType)
struct FSFDialogueNarrativeEntry
{
    GENERATED_BODY()

    /** Dialogue address this entry applies to. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    FSFDialogueKey Key;

    /** Tags describing the narrative dimension of this line/choice. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    FGameplayTagContainer NarrativeTags;

    /** Conditions that must pass for this line/choice to be available. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    FSFNarrativeConditionSet AvailabilityConditions;

    /** Narrative deltas applied when this node is entered. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    TArray<FSFNarrativeDelta> OnEnterDeltas;

    /** Narrative deltas applied when this node is completed/exited. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    TArray<FSFNarrativeDelta> OnExitDeltas;

    /** Optional explicit “choice consequence” description for logs/UI. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    FText ConsequenceDescription;

    /** If true, the first time this fires it sets a fact to “seen once”. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    bool bMarkAsSeenFact = true;

    /** Fact key template for “seen this line/choice”. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue", meta = (EditCondition = "bMarkAsSeenFact"))
    FSFWorldFactKey SeenFactKey;
};

/**
 * Data asset mapping dialogue nodes/options to narrative conditions
 * and consequences.
 *
 * Your dialogue integration layer looks entries up by FSFDialogueKey
 * and:
 *  - asks if the availability conditions pass,
 *  - applies OnEnter/OnExit deltas at appropriate times.
 */
UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFDialogueNarrativeMap : public UDataAsset
{
    GENERATED_BODY()

public:
    /** All narrative hooks for dialogue. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Dialogue")
    TArray<FSFDialogueNarrativeEntry> Entries;

    //
    // Query helpers
    //

    /** Get all entries for a given conversation + node. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Dialogue")
    void GetEntriesForNode(
        FName ConversationId,
        FName NodeId,
        TArray<FSFDialogueNarrativeEntry>& OutEntries) const;

    /** Get a single entry for a specific conversation/node/option. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Dialogue")
    bool GetEntry(
        const FSFDialogueKey& Key,
        FSFDialogueNarrativeEntry& OutEntry) const;

    /** Convenience: get all entries tagged with a particular narrative tag. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Dialogue")
    void GetEntriesWithTag(
        FGameplayTag Tag,
        TArray<FSFDialogueNarrativeEntry>& OutEntries) const;
};
