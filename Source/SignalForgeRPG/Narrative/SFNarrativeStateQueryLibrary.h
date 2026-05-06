// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeTriggerTypes.h"
#include "SFQuestStateTypes.h"
#include "SFNarrativeStateQueryLibrary.generated.h"

class USFNarrativeFactSubsystem;
class USFNarrativeStateSubsystem;
class USFQuestDefinition;
class USFQuestInstance;

/**
 * High-level, read-only queries over the narrative state.
 *
 * These functions are intended for:
 *  - Blueprint usage in quests, dialogue, level scripts.
 *  - C++ helpers that want a single import point to "ask the narrative".
 *
 * They do NOT mutate state.
 */
UCLASS()
class SIGNALFORGERPG_API USFNarrativeStateQueryLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    //
    // Fact & world-state queries
    //

    /** Return the fact subsystem for convenience. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Query", meta = (WorldContext = "WorldContextObject"))
    static USFNarrativeFactSubsystem* GetFactSubsystem(const UObject* WorldContextObject);

    /** Return the core state subsystem. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Query", meta = (WorldContext = "WorldContextObject"))
    static USFNarrativeStateSubsystem* GetStateSubsystem(const UObject* WorldContextObject);

    /** Simple wrapper around "is this bool fact == Expected?". */
    UFUNCTION(BlueprintPure, Category = "Narrative|Query", meta = (WorldContext = "WorldContextObject"))
    static bool IsBoolFactEqual(
        const UObject* WorldContextObject,
        const FSFWorldFactKey& Key,
        bool bExpected);

    /** Numeric fact (int/float) vs. numeric condition. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Query", meta = (WorldContext = "WorldContextObject"))
    static bool DoesNumericFactPassCondition(
        const UObject* WorldContextObject,
        const FSFWorldFactKey& Key,
        const FSFNarrativeNumericCondition& Condition);

    /** Evaluate all WorldFact-domain conditions in a set. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Query", meta = (WorldContext = "WorldContextObject"))
    static bool EvaluateWorldFactConditionsInSet(
        const UObject* WorldContextObject,
        const FSFNarrativeConditionSet& ConditionSet);

    //
    // Faction & identity queries
    //

    /** Get a faction's standing band (or Unknown). */
    UFUNCTION(BlueprintPure, Category = "Narrative|Query", meta = (WorldContext = "WorldContextObject"))
    static ESFFactionStandingBand GetFactionStandingBand(
        const UObject* WorldContextObject,
        FGameplayTag FactionTag);

    /** True if the faction band is at least RequiredBand (using your enum ordering). */
    UFUNCTION(BlueprintPure, Category = "Narrative|Query", meta = (WorldContext = "WorldContextObject"))
    static bool IsFactionStandingAtLeast(
        const UObject* WorldContextObject,
        FGameplayTag FactionTag,
        ESFFactionStandingBand RequiredBand);

    /** Get numeric identity axis value (0 if missing). */
    UFUNCTION(BlueprintPure, Category = "Narrative|Query", meta = (WorldContext = "WorldContextObject"))
    static float GetIdentityAxisValue(
        const UObject* WorldContextObject,
        FGameplayTag AxisTag,
        bool& bOutHasValue);

    //
    // Ending queries
    //

    /** Get full ending state (score + availability). */
    UFUNCTION(BlueprintPure, Category = "Narrative|Query", meta = (WorldContext = "WorldContextObject"))
    static bool GetEndingState(
        const UObject* WorldContextObject,
        FGameplayTag EndingTag,
        FSFEndingState& OutState);

    /** True if an ending is at least Available (or whatever threshold you want). */
    UFUNCTION(BlueprintPure, Category = "Narrative|Query", meta = (WorldContext = "WorldContextObject"))
    static bool IsEndingAvailable(
        const UObject* WorldContextObject,
        FGameplayTag EndingTag,
        ESFEndingAvailability MinimumAvailability = ESFEndingAvailability::Available);

    //
    // Quest/condition helpers
    //

    /**
     * Evaluate quest-state domain conditions in a condition set against
     * a particular quest instance (definition + snapshot).
     */
    UFUNCTION(BlueprintPure, Category = "Narrative|Query")
    static bool EvaluateQuestStateConditionsForInstance(
        const FSFNarrativeConditionSet& ConditionSet,
        const USFQuestDefinition* QuestDef,
        const USFQuestInstance* QuestInstance);

    //
    // Top-level condition evaluation orchestration
    //

    /**
     * Evaluate a condition set across all supported domains:
     *  - WorldFact: via fact subsystem
     *  - QuestState: via provided quest definition/instance (optional)
     *  - Faction/Identity/Ending: via state subsystem
     *
     * Any domains you don't care about for this call can be omitted.
     */
    UFUNCTION(BlueprintPure, Category = "Narrative|Query", meta = (WorldContext = "WorldContextObject"))
    static bool EvaluateConditionSet(
        const UObject* WorldContextObject,
        const FSFNarrativeConditionSet& ConditionSet,
        const USFQuestDefinition* QuestDef = nullptr,
        const USFQuestInstance* QuestInstance = nullptr);
};
