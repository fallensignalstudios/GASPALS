// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Math/UnrealMathUtility.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeStructs.generated.h"

class APlayerState;
class USFNarrativeComponent;

/**
 * Who/what is driving a narrative change. Pass this to mutators on the runtime
 * components so they can attribute deltas, log, and run gating.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFNarrativeInstigator
{
    GENERATED_BODY()

    /** Player driving the change (if any). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Context")
    TObjectPtr<APlayerState> PlayerState = nullptr;

    /** Actor at the center of the interaction (speaker, quest giver, target). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Context")
    TObjectPtr<AActor> FocusActor = nullptr;

    /** Optional narrative component for the current subject. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Context")
    TObjectPtr<USFNarrativeComponent> NarrativeComponent = nullptr;

    /** Arbitrary context identifier (location id, encounter id, etc.). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Context")
    FName ContextId = NAME_None;

    /** World/system tags giving extra flavor (Night, InCombat, Stealth). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Context")
    FGameplayTagContainer ContextTags;
};

/**
 * { quest, state, task } address. Use instead of three loose FNames.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFQuestTaskAddress
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest")
    FName QuestId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest")
    FName StateId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest")
    FName TaskId = NAME_None;

    FSFQuestTaskAddress() = default;
    FSFQuestTaskAddress(FName InQuestId, FName InStateId, FName InTaskId)
        : QuestId(InQuestId), StateId(InStateId), TaskId(InTaskId) {
    }

    bool IsValid() const { return QuestId != NAME_None && TaskId != NAME_None; }

    bool operator==(const FSFQuestTaskAddress& Other) const
    {
        return QuestId == Other.QuestId && StateId == Other.StateId && TaskId == Other.TaskId;
    }
    bool operator!=(const FSFQuestTaskAddress& Other) const { return !(*this == Other); }
};

/**
 * { dialogue, node } address.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFDialogueNodeAddress
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    FName DialogueId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    FName NodeId = NAME_None;

    FSFDialogueNodeAddress() = default;
    FSFDialogueNodeAddress(FName InDialogueId, FName InNodeId)
        : DialogueId(InDialogueId), NodeId(InNodeId) {
    }

    bool IsValid() const { return DialogueId != NAME_None && NodeId != NAME_None; }

    bool operator==(const FSFDialogueNodeAddress& Other) const
    {
        return DialogueId == Other.DialogueId && NodeId == Other.NodeId;
    }
    bool operator!=(const FSFDialogueNodeAddress& Other) const { return !(*this == Other); }
};

/**
 * A single playable dialogue option built from FSFDialogueNodeDefinition plus runtime gating.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFDialogueOptionView
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    FSFDialogueNodeAddress NodeAddress;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    FText Line;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    FName SpeakerId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    bool bIsAvailable = true;

    /** Locked-state hint for UI (greyed-out reason). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    FText LockedReason;

    /** Outcome tags previewed in tooltips ("[Hostile]", "[Loyalty +]"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    FGameplayTagContainer OutcomeTags;
};

/**
 * The current dialogue "moment": active line + the immediate set of replies.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFDialogueMoment
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    FSFDialogueNodeDefinition CurrentNode;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    TArray<FSFDialogueOptionView> Options;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    FName CurrentSpeakerId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    bool bIsTerminal = false;
};

/**
 * Result of applying a single quest-task progress mutation.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFTaskProgressResult
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Tasks")
    FSFTaskProgressKey Key;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Tasks")
    int32 NewCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Tasks")
    int32 DeltaApplied = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Tasks")
    bool bTaskCompleted = false;
};

/**
 * Before/after pair for a single world-fact change.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFWorldFactChange
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|World")
    FSFWorldFactKey Key;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|World")
    FSFWorldFactValue OldValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|World")
    FSFWorldFactValue NewValue;

    bool HasChanged() const { return OldValue != NewValue; }
};

/**
 * Before/after pair for a single faction-standing change.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFFactionChange
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Factions")
    FGameplayTag FactionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Factions")
    FSFFactionStandingValue OldStanding;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Factions")
    FSFFactionStandingValue NewStanding;

    /** True if the categorical band crossed (drives big-moment VO/UI). */
    bool DidStandingBandChange() const { return OldStanding.StandingBand != NewStanding.StandingBand; }
};

/**
 * Before/after pair for a single identity-axis change.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFIdentityChange
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Identity")
    FSFIdentityAxisValue OldValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Identity")
    FSFIdentityAxisValue NewValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Identity")
    ESFIdentityDriftDirection DriftDirection = ESFIdentityDriftDirection::None;

    float GetDelta() const { return NewValue.Value - OldValue.Value; }
};

/**
 * Aggregated result of a single narrative operation (apply outcome, complete state, etc.).
 * Emitted to higher-level observers (UI, analytics) instead of fanning out individual deltas.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFNarrativeChangeSet
{
    GENERATED_BODY()

    /** Tags describing the source of the change (outcome id, dialogue id, etc.). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Changes")
    FGameplayTagContainer SourceTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Changes")
    TArray<FSFTaskProgressResult> TaskResults;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Changes")
    TArray<FSFWorldFactChange> WorldFactChanges;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Changes")
    TArray<FSFFactionChange> FactionChanges;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Changes")
    TArray<FSFIdentityChange> IdentityChanges;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Changes")
    TArray<FSFOutcomeApplication> AppliedOutcomes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Changes")
    TArray<FSFEndingState> EndingStatesChanged;

    bool IsEmpty() const
    {
        return TaskResults.Num() == 0
            && WorldFactChanges.Num() == 0
            && FactionChanges.Num() == 0
            && IdentityChanges.Num() == 0
            && AppliedOutcomes.Num() == 0
            && EndingStatesChanged.Num() == 0;
    }
};

/**
 * Per-instance behavior knobs for a narrative component / replicator.
 * Promote to a UDataAsset later if designers need to tune it without recompiling.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFNarrativeRuntimeConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Runtime")
    bool bReplicateNarrativeDeltas = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Runtime")
    bool bAutoSaveOnPlayerSave = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Runtime")
    bool bTreatAsLocalOnly = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Runtime", meta = (ClampMin = "0"))
    int32 MaxTrackedActiveQuests = 32;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Runtime", meta = (ClampMin = "1"))
    int32 MaxConcurrentDialogues = 1;
};

//
// Hash support
//

FORCEINLINE uint32 GetTypeHash(const FSFQuestTaskAddress& Address)
{
    uint32 Hash = GetTypeHash(Address.QuestId);
    Hash = HashCombine(Hash, GetTypeHash(Address.StateId));
    Hash = HashCombine(Hash, GetTypeHash(Address.TaskId));
    return Hash;
}

FORCEINLINE uint32 GetTypeHash(const FSFDialogueNodeAddress& Address)
{
    return HashCombine(GetTypeHash(Address.DialogueId), GetTypeHash(Address.NodeId));
}