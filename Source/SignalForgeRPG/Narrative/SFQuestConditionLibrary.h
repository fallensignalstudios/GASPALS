// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeTriggerTypes.h"
#include "SFQuestStateTypes.h"
#include "SFQuestObjectiveDefinition.h"
#include "SFQuestStepDefinition.h"
#include "SFQuestConditionLibrary.generated.h"

class USFQuestDefinition;
class USFQuestInstance;

/**
 * Helper library for evaluating quest-related conditions:
 *  - Are all tasks in a state complete?
 *  - Are objectives / steps complete?
 *  - Does a quest snapshot satisfy particular FSFNarrativeConditionSet entries
 *    that refer to quest state (FSFNarrativeQuestStateCondition)?
 *
 * It intentionally does NOT evaluate world facts / factions / identity; those
 * should be delegated to a narrative state subsystem and then composed with
 * the results here.
 */
UCLASS()
class SIGNALFORGERPG_API USFQuestConditionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    //
    // Raw task completion helpers
    //

    /** Returns true if all tasks in the given state definition are completed according to TaskProgressByTaskId. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Quest|Conditions")
    static bool AreAllStateTasksComplete(const FSFQuestStateDefinition& StateDef, const TMap<FName, int32>& TaskProgressByTaskId);

    /** Returns true if a single task is complete, based on RequiredQuantity and TaskProgressByTaskId. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Quest|Conditions")
    static bool IsTaskComplete(const FSFQuestTaskDefinition& TaskDef, const TMap<FName, int32>& TaskProgressByTaskId);

    //
    // Objective / step completion helpers
    //

    /** Returns true if all tasks for an objective are complete, ignoring additional CompletionConditions. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Quest|Conditions")
    static bool IsObjectiveTasksComplete(const FSFQuestObjectiveDefinition& ObjectiveDef, const TMap<FName, int32>& TaskProgressByTaskId);

    /** Returns true if all tasks for a step are complete, ignoring additional CompletionConditions. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Quest|Conditions")
    static bool IsStepTasksComplete(const FSFQuestStepDefinition& StepDef, const TMap<FName, int32>& TaskProgressByTaskId);

    /** Returns true if all primary objectives in the given array have their tasks completed. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Quest|Conditions")
    static bool ArePrimaryObjectivesComplete(const TArray<FSFQuestObjectiveDefinition>& Objectives, const TMap<FName, int32>& TaskProgressByTaskId);

    //
    // Quest state condition helpers (QuestState domain from FSFNarrativeCondition)
    //

    /**
     * Evaluate a single FSFNarrativeQuestStateCondition against the provided
     * quest snapshot + definition. This only considers quest-local state:
     *  - CompletionState
     *  - CurrentStateId
     *  - ReachedStateIds
     *  - OutcomeTags (from FSFQuestSnapshot)
     */
    UFUNCTION(BlueprintPure, Category = "Narrative|Quest|Conditions")
    static bool EvaluateQuestStateCondition(
        const FSFNarrativeQuestStateCondition& Condition,
        const USFQuestDefinition* QuestDef,
        const FSFQuestSnapshot& Snapshot);

    /**
     * Evaluate all QuestState-domain conditions inside a condition set against
     * the given quest. Non-QuestState domains are ignored here and should be
     * evaluated separately by your narrative state subsystem.
     *
     * This is useful when you are filtering dialogue options or outcomes
     * based on quest progress.
     */
    UFUNCTION(BlueprintPure, Category = "Narrative|Quest|Conditions")
    static bool EvaluateQuestStateConditionsInSet(
        const FSFNarrativeConditionSet& ConditionSet,
        const USFQuestDefinition* QuestDef,
        const FSFQuestSnapshot& Snapshot);

    //
    // Snapshots and views
    //

    /** Build a quick “is this state logically complete?” check from snapshot + state def. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Quest|Conditions")
    static bool IsStateLogicallyComplete(
        const FSFQuestStateDefinition& StateDef,
        const FSFQuestSnapshot& Snapshot);

    /** Determine a simple view status for a state based on snapshot and definition. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Quest|Conditions")
    static ESFQuestStateViewStatus ComputeStateViewStatus(
        const FSFQuestStateDefinition& StateDef,
        const FSFQuestSnapshot& Snapshot);
};
