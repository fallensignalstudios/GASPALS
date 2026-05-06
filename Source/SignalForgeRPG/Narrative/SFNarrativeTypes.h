// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/PrimaryAssetId.h"
#include "SFNarrativeTypes.generated.h"

class AActor;
class APlayerState;
class USFNarrativeComponent;

/**
 * High-level lifecycle state of a quest instance.
 */
UENUM(BlueprintType)
enum class ESFQuestCompletionState : uint8
{
    NotStarted,
    InProgress,
    Succeeded,
    Failed,
    Abandoned
};

/**
 * Why a dialogue session ended.
 */
UENUM(BlueprintType)
enum class ESFDialogueExitReason : uint8
{
    Completed,
    Cancelled,
    Interrupted,
    Replaced,
    NetworkDesync
};

/**
 * Mutation operator for narrative value writes (used by data-driven outcomes).
 */
UENUM(BlueprintType)
enum class ESFNarrativeValueOp : uint8
{
    None,
    Set,
    Add,
    Remove,
    Clear,
    Increment,
    Decrement,
    ClampMin,
    ClampMax
};

/**
 * Discriminator for the variant payload of FSFWorldFactValue.
 */
UENUM(BlueprintType)
enum class ESFNarrativeFactValueType : uint8
{
    None,
    Bool,
    Int,
    Float,
    Name,
    Tag
};

/**
 * Discrete bands that classify a faction's numeric standing.
 * Bands gate VO, mission availability, and AI behavior.
 */
UENUM(BlueprintType)
enum class ESFFactionStandingBand : uint8
{
    Unknown,
    Hostile,
    Adversarial,
    Wary,
    Neutral,
    Cooperative,
    Allied,
    Bound
};

/**
 * Direction an identity axis is moving after a recent delta.
 */
UENUM(BlueprintType)
enum class ESFIdentityDriftDirection : uint8
{
    None,
    Negative,
    Positive
};

/**
 * Public availability state of an ending.
 */
UENUM(BlueprintType)
enum class ESFEndingAvailability : uint8
{
    Unknown,
    Hidden,
    Available,
    Threatened,
    Locked,
    Dominant
};

/**
 * Discriminator for FSFNarrativeDelta. Keep this enum stable across save versions:
 * the wire format and replay logs depend on the ordinal values.
 *
 * When adding a new entry, append it to the end and bump CurrentSaveSchemaVersion
 * in FSFNarrativeConstants.
 */
UENUM(BlueprintType)
enum class ESFNarrativeDeltaType : uint8
{
    None,

    StartQuest,
    RestartQuest,
    AbandonQuest,
    EnterQuestState,
    CompleteQuestTask,
    TaskProgress,

    BeginDialogue,
    ExitDialogue,
    AdvanceDialogue,
    SelectDialogueOption,
    TriggerDialogueEvent,

    SetWorldFact,
    AddWorldFactTag,
    RemoveWorldFactTag,

    ApplyFactionDelta,
    SetFactionStandingBand,

    ApplyIdentityDelta,

    ApplyOutcome,

    SetEndingAvailability,
    ApplyEndingScoreDelta,

    SaveCheckpointLoaded,
    FullStateResync
};

/**
 * Parameters used to start a dialogue session. Keep this small; richer context
 * belongs on FSFNarrativeInstigator.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFDialogueStartParams
{
    GENERATED_BODY()

    /** Node to begin from. NAME_None means the dialogue's authored start node. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    FName StartNodeId = NAME_None;

    /** Higher values can interrupt lower-priority dialogue. INDEX_NONE means use default. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    int32 Priority = INDEX_NONE;

    /** Actor that anchors the dialogue (typically the speaker). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    TObjectPtr<AActor> ContextActor = nullptr;

    /** Whether starting this dialogue may interrupt a lower-priority active session. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    bool bAllowInterruptLowerPriorityDialogue = true;
};

/**
 * Authoring definition of a single quest task.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFQuestTaskDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    FName TaskId = NAME_None;

    /** Semantic verb (Kill, Collect, Interact, Travel, etc.). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    FGameplayTag TaskTag;

    /** Optional context (target id, location id, etc.) used to disambiguate progress. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    FName ExpectedContextId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest", meta = (ClampMin = "1"))
    int32 RequiredQuantity = 1;
};

/**
 * Authoring definition of a single quest state in the quest's state graph.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFQuestStateDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    FName StateId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    TArray<FSFQuestTaskDefinition> Tasks;

    /** Tags applied to the world / identity / factions on entering this state. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    TArray<FGameplayTag> OutcomeTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    bool bIsSuccessState = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    bool bIsFailureState = false;
};

/**
 * Authoring definition of a single dialogue node.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFDialogueNodeDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Dialogue")
    FName NodeId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Dialogue")
    FText Line;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Dialogue")
    FName SpeakerId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Dialogue")
    TArray<FName> NextNodeIds;

    /** Tags fired when this node is entered (drives systemic reactions). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Dialogue")
    TArray<FGameplayTag> EventTags;

    /** Tags written into NPC memory when this node is entered. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Dialogue")
    TArray<FGameplayTag> MemoryTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Dialogue")
    bool bPlayerResponse = false;
};

/**
 * Runtime grouping of nodes that should be presented together (a single "moment").
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFDialogueChunk
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Dialogue")
    TArray<FName> NPCNodeIds;

    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Dialogue")
    TArray<FName> PlayerReplyIds;

    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Dialogue")
    FName CurrentSpeakerId = NAME_None;
};

/**
 * Composite key for tracking task progress: { task verb, context }.
 * Two "kill ten rats" tasks with different ContextIds count separately.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFTaskProgressKey
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Tasks")
    FGameplayTag TaskTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Tasks")
    FName ContextId = NAME_None;

    bool operator==(const FSFTaskProgressKey& Other) const
    {
        return TaskTag == Other.TaskTag && ContextId == Other.ContextId;
    }

    bool operator!=(const FSFTaskProgressKey& Other) const { return !(*this == Other); }
};

USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFTaskCounterSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Tasks")
    FSFTaskProgressKey Key;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Tasks")
    int32 Count = 0;
};

USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFWorldFactKey
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|World")
    FGameplayTag FactTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|World")
    FName ContextId = NAME_None;

    bool operator==(const FSFWorldFactKey& Other) const
    {
        return FactTag == Other.FactTag && ContextId == Other.ContextId;
    }

    bool operator!=(const FSFWorldFactKey& Other) const { return !(*this == Other); }
};

/**
 * Variant value held by a world fact. Only the field that matches ValueType is meaningful.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFWorldFactValue
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|World")
    ESFNarrativeFactValueType ValueType = ESFNarrativeFactValueType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|World")
    bool BoolValue = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|World")
    int32 IntValue = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|World")
    float FloatValue = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|World")
    FName NameValue = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|World")
    FGameplayTag TagValue;

    static FSFWorldFactValue MakeBool(bool bInValue)
    {
        FSFWorldFactValue V; V.ValueType = ESFNarrativeFactValueType::Bool;  V.BoolValue = bInValue; return V;
    }
    static FSFWorldFactValue MakeInt(int32 InValue)
    {
        FSFWorldFactValue V; V.ValueType = ESFNarrativeFactValueType::Int;   V.IntValue = InValue;  return V;
    }
    static FSFWorldFactValue MakeFloat(float InValue)
    {
        FSFWorldFactValue V; V.ValueType = ESFNarrativeFactValueType::Float; V.FloatValue = InValue;  return V;
    }
    static FSFWorldFactValue MakeName(FName InValue)
    {
        FSFWorldFactValue V; V.ValueType = ESFNarrativeFactValueType::Name;  V.NameValue = InValue;  return V;
    }
    static FSFWorldFactValue MakeTag(FGameplayTag InValue)
    {
        FSFWorldFactValue V; V.ValueType = ESFNarrativeFactValueType::Tag;   V.TagValue = InValue;  return V;
    }

    /** Strict equality: same type AND same active payload. */
    bool operator==(const FSFWorldFactValue& Other) const
    {
        if (ValueType != Other.ValueType) { return false; }
        switch (ValueType)
        {
        case ESFNarrativeFactValueType::Bool:  return BoolValue == Other.BoolValue;
        case ESFNarrativeFactValueType::Int:   return IntValue == Other.IntValue;
        case ESFNarrativeFactValueType::Float: return FMath::IsNearlyEqual(FloatValue, Other.FloatValue);
        case ESFNarrativeFactValueType::Name:  return NameValue == Other.NameValue;
        case ESFNarrativeFactValueType::Tag:   return TagValue == Other.TagValue;
        case ESFNarrativeFactValueType::None:  return true;
        default:                               return false;
        }
    }

    bool operator!=(const FSFWorldFactValue& Other) const { return !(*this == Other); }

    bool IsSet() const { return ValueType != ESFNarrativeFactValueType::None; }
};

USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFWorldFactSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|World")
    FSFWorldFactKey Key;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|World")
    FSFWorldFactValue Value;
};

/**
 * Six-dimensional numeric standing toward a faction. Don't confuse with the
 * categorical band; the band is derived from these values via
 * FSFNarrativeConstants::ClassifyFactionStanding on the dominant metric (Trust).
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFFactionStandingValue
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Factions")
    float Trust = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Factions")
    float Fear = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Factions")
    float Respect = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Factions")
    float Dependency = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Factions")
    float Alignment = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Factions")
    float Betrayal = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Factions")
    ESFFactionStandingBand StandingBand = ESFFactionStandingBand::Unknown;
};

USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFFactionDelta
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Factions")
    FGameplayTag FactionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Factions")
    float TrustDelta = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Factions")
    float FearDelta = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Factions")
    float RespectDelta = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Factions")
    float DependencyDelta = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Factions")
    float AlignmentDelta = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Factions")
    float BetrayalDelta = 0.f;

    bool IsZero() const
    {
        return FMath::IsNearlyZero(TrustDelta)
            && FMath::IsNearlyZero(FearDelta)
            && FMath::IsNearlyZero(RespectDelta)
            && FMath::IsNearlyZero(DependencyDelta)
            && FMath::IsNearlyZero(AlignmentDelta)
            && FMath::IsNearlyZero(BetrayalDelta);
    }
};

USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFFactionSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Factions")
    FGameplayTag FactionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Factions")
    FSFFactionStandingValue Standing;
};

USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFIdentityAxisValue
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Identity")
    FGameplayTag AxisTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Identity")
    float Value = 0.f;
};

USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFIdentityDelta
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Identity")
    FGameplayTag AxisTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Identity")
    float Delta = 0.f;
};

USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFOutcomeApplication
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Outcomes")
    FPrimaryAssetId OutcomeAssetId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Outcomes")
    FName ContextId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Outcomes")
    TArray<FGameplayTag> AppliedTags;
};

USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFEndingState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Endings")
    FGameplayTag EndingTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Endings")
    ESFEndingAvailability Availability = ESFEndingAvailability::Hidden;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Endings")
    float Score = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Endings")
    TArray<FGameplayTag> LockedReasonTags;
};

USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFQuestSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Quest")
    FPrimaryAssetId QuestAssetId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Quest")
    ESFQuestCompletionState CompletionState = ESFQuestCompletionState::NotStarted;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Quest")
    FName CurrentStateId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Quest")
    TArray<FName> ReachedStateIds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Quest")
    TMap<FName, int32> TaskProgressByTaskId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Quest")
    TArray<FGameplayTag> OutcomeTags;
};

/**
 * Optional table used by the save service to remap stale ids/tags to current ones
 * when loading a save authored against an older content version.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFNarrativeRedirectTable
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Save")
    TMap<FName, FName> QuestIdRedirects;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Save")
    TMap<FName, FName> QuestStateRedirects;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Save")
    TMap<FGameplayTag, FGameplayTag> FactTagRedirects;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Save")
    TMap<FGameplayTag, FGameplayTag> FactionTagRedirects;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Save")
    TMap<FGameplayTag, FGameplayTag> IdentityAxisRedirects;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Save")
    TMap<FGameplayTag, FGameplayTag> EndingTagRedirects;
};

/**
 * Generic per-owner snapshot for narrative components or subsystems
 * that want to store additional serialized data alongside the canonical
 * narrative save payload. The concrete meaning of OwnerId / PayloadTags /
 * PayloadData is up to your game; this struct just gives you a standard
 * container.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFNarrativeOwnerSnapshot
{
    GENERATED_BODY()

    /** Identifier for the owner (player, character, subsystem, etc.). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Save|Owner")
    FName OwnerId = NAME_None;

    /** Tags describing what this payload contains (for routing / migration). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Save|Owner")
    FGameplayTagContainer PayloadTags;

    /** Opaque, versioned binary blob. Use your own serialization into this array. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Save|Owner")
    TArray<uint8> PayloadData;
};

/**
 * Full serialized payload for the narrative subsystem. Bump SchemaVersion
 * (and update FSFNarrativeConstants) whenever you change the layout.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFNarrativeSaveData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Save")
    int32 SchemaVersion = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Save")
    TArray<FSFQuestSnapshot> QuestSnapshots;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Save")
    TArray<FSFTaskCounterSnapshot> LifetimeTaskCounters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Save")
    TArray<FSFWorldFactSnapshot> WorldFacts;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Save")
    TArray<FSFFactionSnapshot> FactionSnapshots;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Save")
    TArray<FSFIdentityAxisValue> IdentityAxes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Save")
    TArray<FSFOutcomeApplication> AppliedOutcomes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Save")
    TArray<FSFEndingState> EndingStates;

    /**
     * Optional opaque per-owner payloads. Lets game-specific systems persist
     * additional data alongside the canonical narrative payload without
     * expanding this struct.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Narrative|Save")
    TArray<FSFNarrativeOwnerSnapshot> CustomOwnerSnapshots;
};

/**
 * Compact wire format for narrative state changes. Designed to fit a high
 * volume of small, ordered events into RPC bandwidth without per-type RPC bloat.
 *
 * Field semantics depend on Type. Use the Make* helpers; do not hand-pack.
 *
 * Wire layout (each FSFNarrativeDelta):
 *   2 FNames (SubjectId, Arg0), 1 extra FName (Arg1)
 *   2 FGameplayTags (Tag0, Tag1)
 *   2 int32, 2 float, 1 bool
 *   1 sequence number for ordering
 *
 * For deltas that need to carry more than fits, push the heavy payload into
 * world state via a setter call and use the delta only as a notification.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFNarrativeDelta
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Replication")
    ESFNarrativeDeltaType Type = ESFNarrativeDeltaType::None;

    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Replication")
    int32 Sequence = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Replication")
    FName SubjectId = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Replication")
    FName Arg0 = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Replication")
    FName Arg1 = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Replication")
    FGameplayTag Tag0;

    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Replication")
    FGameplayTag Tag1;

    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Replication")
    int32 Int0 = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Replication")
    int32 Int1 = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Replication")
    float Float0 = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Replication")
    float Float1 = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Replication")
    bool bBool0 = false;

    //
    // Quest factories
    //

    static FSFNarrativeDelta MakeStartQuest(int32 InSequence, FName InQuestId, FName InStartStateId)
    {
        FSFNarrativeDelta D; D.Type = ESFNarrativeDeltaType::StartQuest;
        D.Sequence = InSequence; D.SubjectId = InQuestId; D.Arg0 = InStartStateId; return D;
    }

    static FSFNarrativeDelta MakeRestartQuest(int32 InSequence, FName InQuestId, FName InStartStateId)
    {
        FSFNarrativeDelta D; D.Type = ESFNarrativeDeltaType::RestartQuest;
        D.Sequence = InSequence; D.SubjectId = InQuestId; D.Arg0 = InStartStateId; return D;
    }

    static FSFNarrativeDelta MakeAbandonQuest(int32 InSequence, FName InQuestId)
    {
        FSFNarrativeDelta D; D.Type = ESFNarrativeDeltaType::AbandonQuest;
        D.Sequence = InSequence; D.SubjectId = InQuestId; return D;
    }

    static FSFNarrativeDelta MakeEnterQuestState(int32 InSequence, FName InQuestId, FName InStateId)
    {
        FSFNarrativeDelta D; D.Type = ESFNarrativeDeltaType::EnterQuestState;
        D.Sequence = InSequence; D.SubjectId = InQuestId; D.Arg0 = InStateId; return D;
    }

    static FSFNarrativeDelta MakeCompleteQuestTask(int32 InSequence, FName InQuestId, FName InTaskId, int32 InFinalCount)
    {
        FSFNarrativeDelta D; D.Type = ESFNarrativeDeltaType::CompleteQuestTask;
        D.Sequence = InSequence; D.SubjectId = InQuestId; D.Arg0 = InTaskId; D.Int0 = InFinalCount; return D;
    }

    static FSFNarrativeDelta MakeTaskProgress(int32 InSequence, FName InQuestId, FName InTaskId, int32 InNewProgress, int32 InDeltaAmount)
    {
        FSFNarrativeDelta D; D.Type = ESFNarrativeDeltaType::TaskProgress;
        D.Sequence = InSequence; D.SubjectId = InQuestId; D.Arg0 = InTaskId;
        D.Int0 = InNewProgress; D.Int1 = InDeltaAmount; return D;
    }

    //
    // Dialogue factories
    //

    static FSFNarrativeDelta MakeBeginDialogue(int32 InSequence, FName InDialogueId, FName InStartNodeId, int32 InPriority)
    {
        FSFNarrativeDelta D; D.Type = ESFNarrativeDeltaType::BeginDialogue;
        D.Sequence = InSequence; D.SubjectId = InDialogueId; D.Arg0 = InStartNodeId; D.Int0 = InPriority; return D;
    }

    static FSFNarrativeDelta MakeExitDialogue(int32 InSequence, FName InDialogueId, ESFDialogueExitReason InReason)
    {
        FSFNarrativeDelta D; D.Type = ESFNarrativeDeltaType::ExitDialogue;
        D.Sequence = InSequence; D.SubjectId = InDialogueId; D.Int0 = static_cast<int32>(InReason); return D;
    }

    static FSFNarrativeDelta MakeAdvanceDialogue(int32 InSequence, FName InDialogueId, FName InCurrentNodeId, FName InSpeakerId)
    {
        FSFNarrativeDelta D; D.Type = ESFNarrativeDeltaType::AdvanceDialogue;
        D.Sequence = InSequence; D.SubjectId = InDialogueId; D.Arg0 = InCurrentNodeId; D.Arg1 = InSpeakerId; return D;
    }

    static FSFNarrativeDelta MakeSelectDialogueOption(int32 InSequence, FName InDialogueId, FName InOptionNodeId, FName InSpeakerId)
    {
        FSFNarrativeDelta D; D.Type = ESFNarrativeDeltaType::SelectDialogueOption;
        D.Sequence = InSequence; D.SubjectId = InDialogueId; D.Arg0 = InOptionNodeId; D.Arg1 = InSpeakerId; return D;
    }

    static FSFNarrativeDelta MakeTriggerDialogueEvent(int32 InSequence, FName InDialogueId, FName InNodeId, FGameplayTag InEventTag)
    {
        FSFNarrativeDelta D; D.Type = ESFNarrativeDeltaType::TriggerDialogueEvent;
        D.Sequence = InSequence; D.SubjectId = InDialogueId; D.Arg0 = InNodeId; D.Tag0 = InEventTag; return D;
    }

    //
    // World fact factories
    //

    static FSFNarrativeDelta MakeSetWorldFact(int32 InSequence, FGameplayTag InFactTag, FName InContextId, const FSFWorldFactValue& InValue)
    {
        FSFNarrativeDelta D; D.Type = ESFNarrativeDeltaType::SetWorldFact;
        D.Sequence = InSequence;
        D.Tag0 = InFactTag;
        D.SubjectId = InContextId;
        D.Int0 = static_cast<int32>(InValue.ValueType);
        D.bBool0 = InValue.BoolValue;
        D.Int1 = InValue.IntValue;
        D.Float0 = InValue.FloatValue;
        D.Arg0 = InValue.NameValue;
        D.Tag1 = InValue.TagValue;
        return D;
    }

    static FSFNarrativeDelta MakeAddWorldFactTag(int32 InSequence, FGameplayTag InFactTag, FName InContextId, FGameplayTag InAddedTag)
    {
        FSFNarrativeDelta D; D.Type = ESFNarrativeDeltaType::AddWorldFactTag;
        D.Sequence = InSequence; D.Tag0 = InFactTag; D.Tag1 = InAddedTag; D.SubjectId = InContextId; return D;
    }

    static FSFNarrativeDelta MakeRemoveWorldFactTag(int32 InSequence, FGameplayTag InFactTag, FName InContextId, FGameplayTag InRemovedTag)
    {
        FSFNarrativeDelta D; D.Type = ESFNarrativeDeltaType::RemoveWorldFactTag;
        D.Sequence = InSequence; D.Tag0 = InFactTag; D.Tag1 = InRemovedTag; D.SubjectId = InContextId; return D;
    }

    //
    // Faction factories
    //

    /**
     * Quantization factor for the secondary faction metrics packed into the delta wire.
     * Trust/Fear use the float fields directly; the other four metrics are quantized to
     * 1/1000 and packed into the int fields. Recover with UnpackFactionDelta.
     */
    static constexpr float FactionMetricFixedPointScale = 1000.f;

    static FSFNarrativeDelta MakeApplyFactionDelta(int32 InSequence, const FSFFactionDelta& InDelta)
    {
        FSFNarrativeDelta D; D.Type = ESFNarrativeDeltaType::ApplyFactionDelta;
        D.Sequence = InSequence;
        D.Tag0 = InDelta.FactionTag;
        D.Float0 = InDelta.TrustDelta;
        D.Float1 = InDelta.FearDelta;
        // Pack two quantized values per int32 (2x int16). Loses a tiny amount of precision
        // but covers the full +/-100.0 metric range with 0.001 resolution.
        const int32 RespQ = FMath::Clamp(FMath::RoundToInt(InDelta.RespectDelta * FactionMetricFixedPointScale), -32768, 32767);
        const int32 DepQ = FMath::Clamp(FMath::RoundToInt(InDelta.DependencyDelta * FactionMetricFixedPointScale), -32768, 32767);
        const int32 AlignQ = FMath::Clamp(FMath::RoundToInt(InDelta.AlignmentDelta * FactionMetricFixedPointScale), -32768, 32767);
        const int32 BetrayQ = FMath::Clamp(FMath::RoundToInt(InDelta.BetrayalDelta * FactionMetricFixedPointScale), -32768, 32767);
        D.Int0 = ((RespQ & 0xFFFF) << 16) | (DepQ & 0xFFFF);
        D.Int1 = ((AlignQ & 0xFFFF) << 16) | (BetrayQ & 0xFFFF);
        return D;
    }

    /** Inverse of MakeApplyFactionDelta. */
    static FSFFactionDelta UnpackFactionDelta(const FSFNarrativeDelta& InWire)
    {
        FSFFactionDelta Out;
        Out.FactionTag = InWire.Tag0;
        Out.TrustDelta = InWire.Float0;
        Out.FearDelta = InWire.Float1;

        // Sign-extend the 16-bit halves.
        auto Lo = [](int32 Packed) { return static_cast<int16>(Packed & 0xFFFF); };
        auto Hi = [](int32 Packed) { return static_cast<int16>((Packed >> 16) & 0xFFFF); };

        Out.RespectDelta = static_cast<float>(Hi(InWire.Int0)) / FactionMetricFixedPointScale;
        Out.DependencyDelta = static_cast<float>(Lo(InWire.Int0)) / FactionMetricFixedPointScale;
        Out.AlignmentDelta = static_cast<float>(Hi(InWire.Int1)) / FactionMetricFixedPointScale;
        Out.BetrayalDelta = static_cast<float>(Lo(InWire.Int1)) / FactionMetricFixedPointScale;
        return Out;
    }

    static FSFNarrativeDelta MakeSetFactionStandingBand(int32 InSequence, FGameplayTag InFactionTag, ESFFactionStandingBand InBand)
    {
        FSFNarrativeDelta D; D.Type = ESFNarrativeDeltaType::SetFactionStandingBand;
        D.Sequence = InSequence; D.Tag0 = InFactionTag; D.Int0 = static_cast<int32>(InBand); return D;
    }

    //
    // Identity factories
    //

    static FSFNarrativeDelta MakeApplyIdentityDelta(int32 InSequence, FGameplayTag InAxisTag, float InDelta)
    {
        FSFNarrativeDelta D; D.Type = ESFNarrativeDeltaType::ApplyIdentityDelta;
        D.Sequence = InSequence; D.Tag0 = InAxisTag; D.Float0 = InDelta; return D;
    }

    //
    // Outcome / ending factories
    //

    static FSFNarrativeDelta MakeApplyOutcome(int32 InSequence, const FPrimaryAssetId& InOutcomeAssetId, FName InContextId)
    {
        FSFNarrativeDelta D; D.Type = ESFNarrativeDeltaType::ApplyOutcome;
        D.Sequence = InSequence;
        // Store FPrimaryAssetId as { TypeName -> SubjectId, AssetName -> Arg0 } so it round-trips
        // through the FName-only wire fields.
        D.SubjectId = InOutcomeAssetId.PrimaryAssetType.GetName();
        D.Arg0 = InOutcomeAssetId.PrimaryAssetName;
        D.Arg1 = InContextId;
        return D;
    }

    /** Inverse of MakeApplyOutcome. */
    static FPrimaryAssetId UnpackOutcomeAssetId(const FSFNarrativeDelta& InWire)
    {
        return FPrimaryAssetId(FPrimaryAssetType(InWire.SubjectId), InWire.Arg0);
    }

    static FSFNarrativeDelta MakeSetEndingAvailability(int32 InSequence, FGameplayTag InEndingTag, ESFEndingAvailability InAvailability, float InScore)
    {
        FSFNarrativeDelta D; D.Type = ESFNarrativeDeltaType::SetEndingAvailability;
        D.Sequence = InSequence; D.Tag0 = InEndingTag;
        D.Int0 = static_cast<int32>(InAvailability); D.Float0 = InScore; return D;
    }

    static FSFNarrativeDelta MakeApplyEndingScoreDelta(int32 InSequence, FGameplayTag InEndingTag, float InScoreDelta)
    {
        FSFNarrativeDelta D; D.Type = ESFNarrativeDeltaType::ApplyEndingScoreDelta;
        D.Sequence = InSequence; D.Tag0 = InEndingTag; D.Float0 = InScoreDelta; return D;
    }

    //
    // System factories
    //

    static FSFNarrativeDelta MakeSaveCheckpointLoaded(int32 InSequence, int32 InSchemaVersion)
    {
        FSFNarrativeDelta D; D.Type = ESFNarrativeDeltaType::SaveCheckpointLoaded;
        D.Sequence = InSequence; D.Int0 = InSchemaVersion; return D;
    }

    static FSFNarrativeDelta MakeFullStateResync(int32 InSequence, int32 InSchemaVersion)
    {
        FSFNarrativeDelta D; D.Type = ESFNarrativeDeltaType::FullStateResync;
        D.Sequence = InSequence; D.Int0 = InSchemaVersion; return D;
    }
};

//
// Hash support
//

FORCEINLINE uint32 GetTypeHash(const FSFTaskProgressKey& Key)
{
    return HashCombine(GetTypeHash(Key.TaskTag), GetTypeHash(Key.ContextId));
}

FORCEINLINE uint32 GetTypeHash(const FSFWorldFactKey& Key)
{
    return HashCombine(GetTypeHash(Key.FactTag), GetTypeHash(Key.ContextId));
}
