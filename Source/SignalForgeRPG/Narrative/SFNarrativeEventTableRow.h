// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"

#include "SFNarrativeTypes.h"
#include "SFNarrativeSaveTypes.h"
#include "SFNarrativeEventTableRow.generated.h"

/**
 * Data-table row describing a single narrative event.
 *
 * This is used by your event scheduler / trigger system to:
 *  - decide WHEN an event can fire (conditions, cooldown, priority),
 *  - decide WHAT happens when it fires (narrative deltas, broadcasts).
 */
USTRUCT(BlueprintType)
struct FSFNarrativeEventTableRow : public FTableRowBase
{
    GENERATED_BODY()

    /** Unique id for this event (usually matches the row name). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Event")
    FName EventId = NAME_None;

    /** Category tag for routing (e.g. Narrative.Event.MainPlot, .Ambient, .Faction). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Event")
    FGameplayTag EventCategory;

    /** Optional sub-tags for finer routing / analytics. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Event")
    FGameplayTagContainer EventTags;

    /** Human-readable display name, for tools/debug/log. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Event")
    FText DisplayName;

    /** Optional longer description, for tools or codex/log UI. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Event")
    FText Description;

    //
    // Triggering
    //

    /**
     * Conditions that must be true for this event to be ELIGIBLE.
     * These are evaluated via USFNarrativeStateQueryLibrary::EvaluateConditionSet.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Event")
    FSFNarrativeConditionSet EligibilityConditions;

    /** Minimum in-game narrative time / chapter index before this can trigger. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Event")
    int32 MinNarrativeChapter = 0;

    /** Optional world fact that, once set, permanently disables this event. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Event")
    FSFWorldFactKey DisableIfFactSet;

    /** Base weight when selecting among eligible events (0 = cannot be randomly picked). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Event")
    float SelectionWeight = 1.0f;

    /** Cooldown in in-game seconds before this event can fire again (0 = one-shot unless explicitly reset). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Event")
    float CooldownSeconds = 0.0f;

    /** If true, this event can fire multiple times (respecting CooldownSeconds). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Event")
    bool bRepeatable = false;

    //
    // Payload
    //

    /** Narrative deltas to apply when the event actually fires. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Event")
    TArray<FSFNarrativeDelta> OnFireDeltas;

    /** Optional world fact set when the event first fires (e.g. “Event.X.Fired”). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Event")
    FSFWorldFactKey FiredFactKey;

    /** Value used for FiredFactKey when the event fires (bool/name/int/etc.), matching your FSFWorldFactValue usage. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Event")
    FSFWorldFactValue FiredFactValue;

    /** Optional tags for the client sync system to format HUD/log entries. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Event")
    FGameplayTagContainer ClientSyncTags;

    /** Suggested client-facing title/body when this event is surfaced to the player. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Event")
    FText ClientTitle;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Event")
    FText ClientBody;
};
