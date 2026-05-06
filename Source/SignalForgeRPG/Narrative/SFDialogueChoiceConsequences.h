#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeTriggerTypes.h"
#include "SFQuestRewardDefinition.h"
#include "SFDialogueNarrativeHooks.h"
#include "SFDialogueChoiceConsequences.generated.h"

class USFQuestDefinition;

/**
 * Simple action for how this choice should influence quest state.
 * Actual behavior is implemented in your quest runtime using these hints.
 */
UENUM(BlueprintType)
enum class ESFDialogueQuestAction : uint8
{
    None,

    // Start the referenced quest if not already started.
    StartQuest,

    // Move the referenced quest into a specific state.
    EnterQuestState,

    // Mark the referenced quest as abandoned.
    AbandonQuest,

    // Mark the referenced quest as failed (e.g. branch fallout).
    FailQuest,

    // Mark the referenced quest as succeeded (e.g. final confirmation choice).
    SucceedQuest
};

/**
 * Represents what happens *specifically because* the player chose a given
 * dialogue option: quest changes, rewards, narrative effects, follow?up routing.
 *
 * This is data?only; execution is handled by:
 *  - USFDialogueComponent / bridge (for dialogue routing),
 *  - USFDialogueNarrativeHooks (for applying narrative effects),
 *  - your quest runtime / reward applier (for quests & rewards).
 */
USTRUCT(BlueprintType)
struct FSFDialogueChoiceConsequences
{
    GENERATED_BODY()

    /** Optional gate: conditions that must pass for these consequences to execute. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Consequences")
    FSFNarrativeConditionSet ExecutionConditions;

    //
    // Dialogue flow
    //

    /** Optional explicit next dialogue asset to jump to after this choice. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Consequences")
    TSoftObjectPtr<UObject> NextConversationAsset;

    /** Optional node ID within the current conversation to jump to after this choice. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Consequences")
    FName OverrideNextNodeId = NAME_None;

    /** Tags describing the branch this choice leads to (e.g. Branch.A, Branch.B). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Consequences")
    FGameplayTagContainer BranchTags;

    //
    // Quest interaction
    //

    /** High?level quest action suggested by this choice. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Consequences")
    ESFDialogueQuestAction QuestAction = ESFDialogueQuestAction::None;

    /** Quest definition to operate on when QuestAction is not None. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Consequences")
    TSoftObjectPtr<USFQuestDefinition> TargetQuest;

    /** Optional specific state to enter when QuestAction == EnterQuestState. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Consequences")
    FName TargetQuestStateId = NAME_None;

    //
    // Rewards
    //

    /** Rewards to grant immediately when this choice is selected. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Consequences")
    FSFQuestRewardSet ImmediateRewards;

    //
    // Narrative system effects
    //

    /** Structured narrative effects (world facts, factions, identity, outcomes, endings). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Consequences")
    FSFDialogueNarrativeEffect NarrativeEffect;

    /** Arbitrary additional tags that will be added to the narrative state when this choice is taken. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Consequences")
    FGameplayTagContainer NarrativeTagsToApply;

    //
    // Helper: simple “is there anything to do?” query.
    //

    bool IsEmpty() const
    {
        return QuestAction == ESFDialogueQuestAction::None
            && ImmediateRewards.IsEmpty()
            && NarrativeEffect.WorldFactsToSet.Num() == 0
            && NarrativeEffect.FactionDeltas.Num() == 0
            && NarrativeEffect.IdentityDeltas.Num() == 0
            && NarrativeEffect.OutcomesToApply.Num() == 0
            && NarrativeEffect.EndingScoreDeltas.Num() == 0
            && !NextConversationAsset.IsValid()
            && OverrideNextNodeId.IsNone()
            && NarrativeTagsToApply.IsEmpty();
    }
};

/**
 * Optional library asset for reusing choice consequences across multiple
 * dialogue assets (e.g. shared “accept quest” choice, “threaten NPC” choice).
 */
UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFDialogueChoiceConsequencesLibrary : public UDataAsset
{
    GENERATED_BODY()

public:
    /** Reusable consequence templates keyed by a stable name. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Consequences")
    TMap<FName, FSFDialogueChoiceConsequences> ConsequencesById;
};