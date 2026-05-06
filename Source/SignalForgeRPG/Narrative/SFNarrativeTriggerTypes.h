#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeTriggerTypes.generated.h"

class AActor;
class USFNarrativeComponent;

/**
 * Result of evaluating a narrative trigger condition.
 */
UENUM(BlueprintType)
enum class ESFNarrativeTriggerResult : uint8
{
    Failed,         // Explicitly did not pass.
    Passed,         // All required conditions passed.
    Deferred        // Not enough information yet (async / remote dependency).
};

/**
 * How we compare numeric values when evaluating conditions.
 */
UENUM(BlueprintType)
enum class ESFNarrativeNumericCompareOp : uint8
{
    Equal,
    NotEqual,
    Greater,
    GreaterOrEqual,
    Less,
    LessOrEqual,
    BetweenInclusive,   // Min/Max range.
    OutsideExclusive    // Outside Min/Max, strictly.
};

/**
 * High?level category of a condition. Primarily useful for tooling/inspection.
 */
UENUM(BlueprintType)
enum class ESFNarrativeConditionDomain : uint8
{
    WorldFact,
    TagSet,
    Faction,
    IdentityAxis,
    QuestState,
    Dialogue,
    Time,
    Custom
};

/**
 * Simple numeric comparison description reused by faction/identity/world?fact conditions.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeNumericCondition
{
    GENERATED_BODY()

    /** Operation applied when comparing Actual vs Thresholds. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Condition")
    ESFNarrativeNumericCompareOp Op = ESFNarrativeNumericCompareOp::GreaterOrEqual;

    /** Primary threshold or left bound. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Condition")
    float ThresholdA = 0.0f;

    /** Secondary threshold or right bound (for Between/Outside). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Condition")
    float ThresholdB = 0.0f;

    /** Optional tolerance for Equal / NotEqual comparisons. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Condition", meta = (ClampMin = "0.0"))
    float Epsilon = KINDA_SMALL_NUMBER;
};

/**
 * Condition that inspects a single world fact.
 * Uses FSFWorldFactKey + FSFWorldFactValue semantics.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeWorldFactCondition
{
    GENERATED_BODY()

    /** Which fact we care about (tag + optional context). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|World")
    FSFWorldFactKey Key;

    /** Required value type; if None, we only care about existence. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|World")
    ESFNarrativeFactValueType ExpectedValueType = ESFNarrativeFactValueType::None;

    /** Required bool value when ExpectedValueType == Bool. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|World")
    bool bExpectedBool = true;

    /** Numeric comparison when ExpectedValueType is Int or Float. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|World")
    FSFNarrativeNumericCondition NumericCondition;

    /** Required name when ExpectedValueType == Name. None means “any non?None name”. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|World")
    FName ExpectedName = NAME_None;

    /** Required tag when ExpectedValueType == Tag. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|World")
    FGameplayTag ExpectedTag;
};

/**
 * Condition that checks tags on the narrative owner and/or global tag sets.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeTagCondition
{
    GENERATED_BODY()

    /** Tags that must all be present on the evaluated context. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Tags")
    FGameplayTagContainer RequiredTags;

    /** If any of these are present, the condition fails. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Tags")
    FGameplayTagContainer BlockedTags;

    /** Optional: at least one of these must be present (logical OR). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Tags")
    FGameplayTagContainer AnyTags;
};

/**
 * Condition that inspects faction standing for a single faction.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeFactionCondition
{
    GENERATED_BODY()

    /** Which faction we care about. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Factions")
    FGameplayTag FactionTag;

    /** Optional band requirement; Unknown means “don’t care”. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Factions")
    ESFFactionStandingBand RequiredBand = ESFFactionStandingBand::Unknown;

    /** Optional numeric test against a single aggregate value (e.g. Alignment). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Factions")
    FSFNarrativeNumericCondition AggregateCondition;

    /** Whether to also allow the condition to pass if any metric exceeds a high watermark (e.g. Trust). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Factions")
    bool bPassIfAnyMetricHigh = false;
};

/**
 * Condition that inspects a single identity axis.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeIdentityAxisCondition
{
    GENERATED_BODY()

    /** Axis we care about (matches FSFIdentityAxisValue::AxisTag). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Identity")
    FGameplayTag AxisTag;

    /** Numeric test on the axis value. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Identity")
    FSFNarrativeNumericCondition ValueCondition;
};

/**
 * Condition that inspects quest state for a given quest asset/ID.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeQuestStateCondition
{
    GENERATED_BODY()

    /** Optional asset?level identifier (if you track quests as primary assets). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest")
    FPrimaryAssetId QuestAssetId;

    /** Local quest ID / short name if you address instances this way. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest")
    FName QuestId = NAME_None;

    /** Acceptable completion states. If empty, any state passes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest")
    TArray<ESFQuestCompletionState> AllowedCompletionStates;

    /** Optional requirement on being in a specific current state. None means “any”. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest")
    FName RequiredCurrentStateId = NAME_None;

    /** Optional requirement that a particular state has been reached at least once. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest")
    FName RequiredReachedStateId = NAME_None;

    /** Whether the quest must have emitted a specific outcome tag. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest")
    FGameplayTag RequiredOutcomeTag;
};

/**
 * Simple time?of?day / chapter / checkpoint style condition.
 * Concrete evaluation lives in your state subsystem (clock, chapter manager, etc.).
 */
USTRUCT(BlueprintType)
struct FSFNarrativeTimeCondition
{
    GENERATED_BODY()

    /** Arbitrary “phase” tag (e.g. Narrative.Time.Day, Narrative.Time.Night, Chapter tags, etc.). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Time")
    FGameplayTag RequiredPhaseTag;

    /** Optional range, in game hours, for when this condition is valid. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Time")
    FSFNarrativeNumericCondition GameTimeHoursCondition;
};

/**
 * A single atomic condition. Complex triggers are built by combining these in
 * FSFNarrativeTriggerConditionSet below.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeCondition
{
    GENERATED_BODY()

    /** For debugging and tools; not used at runtime beyond inspection. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Condition")
    ESFNarrativeConditionDomain Domain = ESFNarrativeConditionDomain::WorldFact;

    /** Optional human?readable label to show in tools/debug UI. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Condition")
    FString DebugLabel;

    /** World?fact based condition. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Condition")
    FSFNarrativeWorldFactCondition WorldFact;

    /** Tag?set based condition. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Condition")
    FSFNarrativeTagCondition TagCondition;

    /** Faction standing condition. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Condition")
    FSFNarrativeFactionCondition Faction;

    /** Identity axis condition. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Condition")
    FSFNarrativeIdentityAxisCondition IdentityAxis;

    /** Quest state condition. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Condition")
    FSFNarrativeQuestStateCondition QuestState;

    /** Time/phase condition. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Condition")
    FSFNarrativeTimeCondition TimeCondition;

    /** If true, the final evaluation is inverted (NOT). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Condition")
    bool bNegate = false;
};

/**
 * Logical grouping of conditions. This forms the basis of most “requirements”
 * on quests, dialogue nodes, outcomes, etc.
 *
 * Evaluation semantics:
 *   - AllConditions:     ALL of these must pass (AND).
 *   - AnyConditions:     At least one of these must pass (OR).
 *   - NoneConditions:    ALL of these must fail (NOT).
 */
USTRUCT(BlueprintType)
struct FSFNarrativeConditionSet
{
    GENERATED_BODY()

    /** All conditions that must pass for this set to be considered passing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Condition")
    TArray<FSFNarrativeCondition> AllConditions;

    /** At least one of these must pass; if empty, this group is ignored. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Condition")
    TArray<FSFNarrativeCondition> AnyConditions;

    /** All of these must fail; if any pass, the set fails. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Condition")
    TArray<FSFNarrativeCondition> NoneConditions;

    /** Optional additional tags that describe / categorize this condition set. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Condition")
    FGameplayTagContainer Tags;

    bool IsEmpty() const
    {
        return AllConditions.Num() == 0
            && AnyConditions.Num() == 0
            && NoneConditions.Num() == 0;
    }
};

/**
 * A complete trigger specification: conditions + bookkeeping on how/when it can fire.
 *
 * You can embed this into quest steps, dialogue nodes, world events, etc.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeTriggerSpec
{
    GENERATED_BODY()

    /** Unique identifier within its owning asset. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Trigger")
    FName TriggerId = NAME_None;

    /** Conditions that must be satisfied for this trigger to fire. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Trigger")
    FSFNarrativeConditionSet Conditions;

    /** Optional tags describing the trigger (used for lookup, analytics, UI). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Trigger")
    FGameplayTagContainer TriggerTags;

    /** If true, this trigger may only fire once per save / lifetime. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Trigger")
    bool bFireOnce = false;

    /** If true, this trigger may only fire once per owner (player, narrative component). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Trigger")
    bool bFireOncePerOwner = false;

    /** If greater than zero, the trigger will only fire when at least this many seconds have passed since the last time it fired. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Trigger", meta = (ClampMin = "0.0"))
    float CooldownSeconds = 0.0f;
};