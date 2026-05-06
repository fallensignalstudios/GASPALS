// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeTriggerTypes.h"
#include "SFQuestStepDefinition.h"        // For FSFQuestStepOutcomeDefinition (optional reuse)
#include "SFQuestObjectiveDefinition.generated.h"

/**
 * How this objective contributes to completion logic for its owning quest/state.
 */
UENUM(BlueprintType)
enum class ESFQuestObjectiveRole : uint8
{
    Primary,        // Required to progress / finish the quest or owning state.
    Optional,       // Nice-to-have; affects rewards or endings but not hard completion.
    HiddenOptional  // Optional and hidden until completed or revealed by other logic.
};

/**
 * A single authored objective inside a quest state.
 *
 * Objectives are higher-level than tasks:
 *  - They group one or more FSFQuestTaskDefinition entries.
 *  - They have their own activation/completion conditions.
 *  - They can be required/optional, have outcome tags, and control UI visibility.
 *
 * You can embed these directly into USFQuestDefinition, or manage them via
 * separate data assets.
 */
USTRUCT(BlueprintType)
struct FSFQuestObjectiveDefinition
{
    GENERATED_BODY()

    /** Author-facing identifier, unique within a quest. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    FName ObjectiveId = NAME_None;

    /** Display name shown in journals / HUD. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    FText DisplayName;

    /** Player-facing description text. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest", meta = (MultiLine = "true"))
    FText Description;

    /** Optional icon/style tag for UI (e.g. Narrative.Quest.Objective.Kill). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    FGameplayTag ObjectiveTag;

    /** Whether this objective is primary / optional / hidden-optional. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    ESFQuestObjectiveRole Role = ESFQuestObjectiveRole::Primary;

    /** Tasks that must be completed for this objective to count as complete. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    TArray<FSFQuestTaskDefinition> Tasks;

    /**
     * Conditions that must pass for this objective to become active / visible.
     * Uses the same condition primitives as steps: world facts, identity axes,
     * faction standing, quest states, time, etc.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    FSFNarrativeConditionSet ActivationConditions;

    /**
     * Additional conditions that must pass for this objective to be considered
     * fully completed, beyond raw task counters (e.g. faction band, identity).
     * If empty, completion is purely task driven.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    FSFNarrativeConditionSet CompletionConditions;

    /**
     * Optional outcomes which fire when this objective completes or fails.
     * Reuses the same outcome definition struct as steps for consistency.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    FSFQuestStepOutcomeDefinition Outcome;

    /** If true, this objective remains hidden until ActivationConditions pass. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    bool bHiddenUntilActive = false;

    /** If true, show this objective as struck-through / completed after finishing. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    bool bShowAfterCompletion = true;

    /** Ordering index for UI within a state; lower values appear earlier. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    int32 OrderIndex = 0;

    /** Optional state identifier this objective belongs to (editor- and tooling-friendly). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    FName OwningStateId = NAME_None;

    //
    // Convenience helpers
    //

    /** True if this objective has at least one valid task with a non-None TaskId. */
    bool HasValidTasks() const
    {
        for (const FSFQuestTaskDefinition& Task : Tasks)
        {
            if (Task.TaskId.IsNone())
            {
                return false;
            }
        }
        return Tasks.Num() > 0;
    }

    /** True if this objective has no tasks and no extra completion conditions (pure narrative beat). */
    bool IsPureNarrativeBeat() const
    {
        return Tasks.Num() == 0 && CompletionConditions.IsEmpty();
    }
};

/**
 * Optional library asset for sharing objectives across quests or states.
 * You can skip this if you prefer to embed objectives directly in USFQuestDefinition.
 */
UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFQuestObjectiveLibrary : public UDataAsset
{
    GENERATED_BODY()

public:
    /** All reusable objective definitions keyed by ObjectiveId. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    TArray<FSFQuestObjectiveDefinition> Objectives;
};
