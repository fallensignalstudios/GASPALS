// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeTriggerTypes.h"
#include "SFQuestRewardDefinition.h"
#include "SFQuestObjectiveDefinition.h"
#include "SFQuestStepDefinition.h"
#include "SFQuestStateTypes.generated.h"

/**
 * High-level category for a quest state. Primarily for tooling and UI.
 */
UENUM(BlueprintType)
enum class ESFQuestStateCategory : uint8
{
    Normal,         // Regular progression state.
    Hub,            // Hub / interstitial state (turn-in, dialog hubs).
    BranchDecision, // Player choice/branch point.
    Failure,        // Explicit failure state.
    Success,        // Explicit success state.
    Epilogue        // Post-completion / epilogue.
};

/**
 * View-only status for how a state appears in quest UI, derived at runtime
 * from the underlying FSFQuestSnapshot and instance logic.
 */
UENUM(BlueprintType)
enum class ESFQuestStateViewStatus : uint8
{
    Locked,         // Not yet available and hidden or greyed out.
    Available,      // Can be entered next, but not yet active.
    Active,         // Current state.
    Completed,      // Past state that was successfully completed.
    Failed,         // Past state that was failed.
    Skipped         // Skipped due to branching, but not failed.
};

/**
 * Additional authored metadata for a quest state, layered on top of
 * FSFQuestStateDefinition from SFNarrativeTypes.h.
 *
 * This lets you keep your existing definition struct intact while adding
 * richer authoring info in a parallel struct.
 */
USTRUCT(BlueprintType)
struct FSFQuestStateMetadata
{
    GENERATED_BODY()

    /** High-level category for this state (normal, hub, success, failure, etc.). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|State")
    ESFQuestStateCategory Category = ESFQuestStateCategory::Normal;

    /** Optional tags describing this state (e.g. Narrative.Quest.State.Hub, Branch.A/B). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|State")
    FGameplayTagContainer StateTags;

    /** Optional cinematic / camera sequence tag to play upon entering this state. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|State")
    FGameplayTag EnterCinematicTag;

    /** Optional dialogue ID or tag to auto-start on entering this state. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|State")
    FName EnterDialogueId = NAME_None;

    /** Optional dialogue ID to consider required / expected to complete this state. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|State")
    FName RequiredDialogueId = NAME_None;

    /** If true, this state should never be shown in quest UI (used for structural states). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|State")
    bool bHiddenInUI = false;
};

/**
 * A richer description of a quest state, suitable for editor-only or
 * higher-level data structures. It references your existing
 * FSFQuestStateDefinition by StateId and adds objectives/steps/rewards/conditions.
 *
 * Runtime can still rely purely on FSFQuestStateDefinition + tasks if desired.
 */
USTRUCT(BlueprintType)
struct FSFQuestStateExtendedDefinition
{
    GENERATED_BODY()

    /** Must match an FSFQuestStateDefinition::StateId in the owning quest asset. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|State")
    FName StateId = NAME_None;

    /** Optional human-readable override for the state display name. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|State")
    FText DisplayNameOverride;

    /** Additional metadata about this state (category, tags, etc.). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|State")
    FSFQuestStateMetadata Metadata;

    /**
     * Objectives within this state. These can be higher-level wrappers
     * around the raw FSFQuestTaskDefinition entries defined in SFNarrativeTypes.h.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|State")
    TArray<FSFQuestObjectiveDefinition> Objectives;

    /**
     * Fine-grained step definitions, if you’re using the Step layer.
     * Steps can be referenced from Objectives (by StepId) or just be
     * used for internal organization.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|State")
    TArray<FSFQuestStepDefinition> Steps;

    /**
     * Conditions that must pass for the quest to be allowed to enter this state.
     * Use SFNarrativeTriggerTypes + your narrative state subsystem to evaluate.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|State")
    FSFNarrativeConditionSet EntryConditions;

    /**
     * Optional conditions that must be satisfied before we consider the state
     * “complete”, beyond raw task/objective completion.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|State")
    FSFNarrativeConditionSet CompletionConditions;

    /** Rewards granted when this state is first completed (e.g. a mid-quest milestone). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|State")
    FSFQuestRewardSet CompletionRewards;

    /** Rewards granted when this state represents or leads to failure. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|State")
    FSFQuestRewardSet FailureRewards;

    /** Optional narrative tags to apply when entering this state (e.g. outcomes, flags). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|State")
    FGameplayTagContainer EnterOutcomeTags;

    /** Optional narrative tags to apply when leaving this state. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|State")
    FGameplayTagContainer ExitOutcomeTags;

    /** True if this state should be treated as a terminal success state (mirrors bIsSuccessState). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|State")
    bool bIsTerminalSuccess = false;

    /** True if this state should be treated as a terminal failure state (mirrors bIsFailureState). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|State")
    bool bIsTerminalFailure = false;

    /** Convenience: returns true if this state has any objectives or steps. */
    bool HasAuthoredStructure() const
    {
        return Objectives.Num() > 0 || Steps.Num() > 0;
    }
};

/**
 * Lightweight view struct that can be built from FSFQuestSnapshot +
 * FSFQuestStateDefinition/FSFQuestStateExtendedDefinition and used by UI.
 */
USTRUCT(BlueprintType)
struct FSFQuestStateView
{
    GENERATED_BODY()

    /** State identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest|State")
    FName StateId = NAME_None;

    /** Localized name (from FSFQuestStateDefinition.DisplayName or override). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest|State")
    FText DisplayName;

    /** High-level category for UI (hub/normal/success/failure/etc.). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest|State")
    ESFQuestStateCategory Category = ESFQuestStateCategory::Normal;

    /** View-only status for this state (locked/available/active/completed/etc.). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest|State")
    ESFQuestStateViewStatus ViewStatus = ESFQuestStateViewStatus::Locked;

    /** All tasks present in this state (from FSFQuestStateDefinition.Tasks). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest|State")
    TArray<FSFQuestTaskDefinition> Tasks;

    /** Optional objective views (built from FSFQuestObjectiveDefinition + runtime progress). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest|State")
    TArray<FSFQuestObjectiveDefinition> Objectives;

    /** Optional tags from metadata (used for icons/filtering). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest|State")
    FGameplayTagContainer StateTags;

    /** Whether this state is currently the one active in the quest instance. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest|State")
    bool bIsCurrentState = false;

    /** Whether we have reached this state at least once (matches ReachedStateIds). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest|State")
    bool bHasBeenReached = false;
};
