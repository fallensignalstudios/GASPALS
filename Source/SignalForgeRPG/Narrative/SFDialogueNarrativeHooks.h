// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeStructs.h"
#include "SFDialogueNarrativeHooks.generated.h"

class USFNarrativeComponent;
class USFConversationDataAsset;

/**
 * Optional, author?facing metadata that can be attached to dialogue nodes or
 * choices to describe how they should interact with the narrative system.
 *
 * You can store this alongside your node/choice definitions in your
 * dialogue data assets and then feed it to Apply* functions below.
 */
USTRUCT(BlueprintType)
struct FSFDialogueNarrativeEffect
{
    GENERATED_BODY()

    /** World facts to set when this line/choice is executed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    TArray<FSFWorldFactSnapshot> WorldFactsToSet;

    /** Faction standing changes this line/choice should apply. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    TArray<FSFFactionDelta> FactionDeltas;

    /** Identity axis changes this line/choice should apply. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    TArray<FSFIdentityDelta> IdentityDeltas;

    /** Outcome assets to apply. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    TArray<FSFOutcomeApplication> OutcomesToApply;

    /** Ending tags to nudge with a score delta. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    TMap<FGameplayTag, float> EndingScoreDeltas;

    /** Arbitrary narrative tags to apply (e.g. “Player.BrokePromise”). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    FGameplayTagContainer NarrativeTagsToApply;
};

/**
 * Central place where dialogue interacts with the narrative system.
 *
 * High?level flow:
 *   - USFDialogueComponent (or the bridge) asks CanPlayNode/CanShowChoice.
 *   - When a node/choice fires, it calls ApplyNodeEffects/ApplyChoiceEffects
 *     with the authored FSFDialogueNarrativeEffect.
 *   - This library builds FSFNarrativeDelta instances and forwards them to
 *     USFNarrativeComponent for replication/save/event routing.
 */
UCLASS()
class SIGNALFORGERPG_API USFDialogueNarrativeHooks : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    //
    // Availability / gating helpers
    //

    /**
     * Returns true if a dialogue node should be considered available based on
     * the supplied narrative tags / world state.
     *
     * This is intended to be called from USFDialogueComponent::DoesNodePassTagChecks,
     * or from editor validation tools.
     *
     * @param OwnedTags     Dialogue/narrative tags for the current participant/view.
     * @param RequiredTags  Node-required tags (must all be present).
     * @param BlockedTags   Tags that block this node if any are present.
     */
    UFUNCTION(BlueprintPure, Category = "Narrative|Dialogue")
    static bool CanPlayDialogueNode(
        const FGameplayTagContainer& OwnedTags,
        const FGameplayTagContainer& RequiredTags,
        const FGameplayTagContainer& BlockedTags);

    /**
     * Returns true if a dialogue choice should be visible/available based on
     * the supplied narrative tags.
     */
    UFUNCTION(BlueprintPure, Category = "Narrative|Dialogue")
    static bool CanShowDialogueChoice(
        const FGameplayTagContainer& OwnedTags,
        const FGameplayTagContainer& RequiredTags,
        const FGameplayTagContainer& BlockedTags);

    //
    // Effect application helpers
    //

    /**
     * Apply narrative effects for a dialogue node (NPC line, event node, etc.).
     *
     * This function:
     *   - Builds FSFNarrativeDelta instances for world facts, factions, identity, outcomes, endings.
     *   - Forwards them to the narrative component.
     *   - Returns a ChangeSet summarizing the logical changes (for UI/logging).
     */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Dialogue")
    static FSFNarrativeChangeSet ApplyNodeNarrativeEffects(
        USFNarrativeComponent* NarrativeComponent,
        const FName DialogueId,
        const FName NodeId,
        const FSFDialogueNarrativeEffect& Effect);

    /**
     * Apply narrative effects for a dialogue choice selected by the player.
     * Same as ApplyNodeNarrativeEffects but uses a choice?specific context id.
     */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Dialogue")
    static FSFNarrativeChangeSet ApplyChoiceNarrativeEffects(
        USFNarrativeComponent* NarrativeComponent,
        const FName DialogueId,
        const FName ChoiceNodeId,
        const FSFDialogueNarrativeEffect& Effect,
        const FName SpeakerId);

private:
    static void EmitWorldFactDeltas(
        USFNarrativeComponent* NarrativeComponent,
        const FName SubjectId,
        const TArray<FSFWorldFactSnapshot>& Facts,
        int32& InOutSequence,
        FSFNarrativeChangeSet& OutChangeSet);

    static void EmitFactionDeltas(
        USFNarrativeComponent* NarrativeComponent,
        const TArray<FSFFactionDelta>& Deltas,
        int32& InOutSequence,
        FSFNarrativeChangeSet& OutChangeSet);

    static void EmitIdentityDeltas(
        USFNarrativeComponent* NarrativeComponent,
        const TArray<FSFIdentityDelta>& Deltas,
        int32& InOutSequence,
        FSFNarrativeChangeSet& OutChangeSet);

    static void EmitOutcomeDeltas(
        USFNarrativeComponent* NarrativeComponent,
        const FName SubjectId,
        const TArray<FSFOutcomeApplication>& Outcomes,
        int32& InOutSequence,
        FSFNarrativeChangeSet& OutChangeSet);

    static void EmitEndingDeltas(
        USFNarrativeComponent* NarrativeComponent,
        const TMap<FGameplayTag, float>& EndingScoreDeltas,
        int32& InOutSequence,
        FSFNarrativeChangeSet& OutChangeSet);
};
