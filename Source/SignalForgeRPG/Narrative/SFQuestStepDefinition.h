#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeTriggerTypes.h"
#include "SFQuestStepDefinition.generated.h"

class USFQuestDefinition;

/**
 * How this step contributes to quest completion/failure logic.
 */
UENUM(BlueprintType)
enum class ESFQuestStepRole : uint8
{
    Neutral,        // Step is informational or optional; it doesn’t directly gate completion.
    Required,       // Step must be completed for the quest (or owning state) to succeed.
    OptionalBonus,  // Step gives extra rewards, but is not required.
    FailureGate     // Failing or skipping this step may push the quest into failure.
};

/**
 * Outcome behavior for a single quest step.
 * This is intentionally generic and tag?driven so you can bind into your
 * outcome/ending systems later without hard dependencies.
 */
USTRUCT(BlueprintType)
struct FSFQuestStepOutcomeDefinition
{
    GENERATED_BODY()

    /** Tags applied when the step is first completed. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|Outcome")
    FGameplayTagContainer CompletionOutcomeTags;

    /** Tags applied if this step fails or expires (when you add that behavior). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|Outcome")
    FGameplayTagContainer FailureOutcomeTags;

    /**
     * Optional explicit outcome asset to apply on completion.
     * If invalid, you’re expected to use tags/other systems instead.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|Outcome")
    FPrimaryAssetId CompletionOutcomeAssetId;

    /**
     * Optional explicit outcome asset to apply on failure.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|Outcome")
    FPrimaryAssetId FailureOutcomeAssetId;
};

/**
 * A single authored step within a quest state.
 *
 * This wraps one or more FSFQuestTaskDefinition entries and adds:
 *  - metadata (name, description, role),
 *  - gating (conditions, required world/identity/faction state),
 *  - outcomes (tags / assets to fire when completed or failed),
 *  - and simple visibility controls for UI.
 *
 * At runtime you can choose to:
 *  - use only Tasks/RequiredTags/BlockedTags and ignore the rest, OR
 *  - interpret the full FSFNarrativeConditionSet via a state subsystem.
 */
USTRUCT(BlueprintType)
struct FSFQuestStepDefinition
{
    GENERATED_BODY()

    /** Author?facing identifier, unique within its owning quest. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    FName StepId = NAME_None;

    /** Human?readable title shown in quest journals. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    FText DisplayName;

    /** Player?facing description text. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest", meta = (MultiLine = "true"))
    FText Description;

    /** Optional icon or style tag for UI theming (e.g. “Kill”, “Explore”). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    FGameplayTag StepTag;

    /** Role this step plays in the owning state/quest. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    ESFQuestStepRole Role = ESFQuestStepRole::Required;

    /** Tasks that must be completed for this step to be considered complete. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    TArray<FSFQuestTaskDefinition> Tasks;

    /**
     * Gating conditions that must pass for this step to become active / visible.
     * You can evaluate this via your narrative state subsystem using world facts,
     * identity axes, faction bands, quest progress, etc.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    FSFNarrativeConditionSet ActivationConditions;

    /**
     * Conditions that must pass for this step to be considered successfully “completed”
     * beyond simple task counters (e.g. identity value, faction standing, or other quest states).
     * If empty, completion is purely task?driven.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    FSFNarrativeConditionSet CompletionConditions;

    /** Outcomes to apply when this step is completed or failed. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    FSFQuestStepOutcomeDefinition Outcome;

    /** If true, this step should be hidden from UI until ActivationConditions pass. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    bool bHiddenUntilActive = false;

    /** If true, this step remains visible as “completed” after it’s done; otherwise it may collapse from UI. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    bool bShowAfterCompletion = true;

    /** Optional ordering index for UI within a state; lower values appear earlier. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    int32 OrderIndex = 0;

    /** Optional state this step most directly belongs to (helps tooling & quick lookups). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    FName OwningStateId = NAME_None;

    /** Convenience: returns true if all Tasks array entries have valid TaskIds. */
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

    /** Whether this step is purely informational (no tasks, no completion conditions). */
    bool IsPureNarrativeBeat() const
    {
        return Tasks.Num() == 0 && CompletionConditions.IsEmpty();
    }
};

/**
 * Optional data asset wrapper for authoring large quests where steps are shared
 * across multiple states or quests. If you prefer to keep steps embedded directly
 * in USFQuestDefinition, you can skip using this asset.
 */
UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFQuestStepLibrary : public UDataAsset
{
    GENERATED_BODY()

public:
    /** All reusable step definitions keyed by StepId. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest")
    TArray<FSFQuestStepDefinition> Steps;
};