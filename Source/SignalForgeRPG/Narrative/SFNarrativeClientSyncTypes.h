// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeSaveTypes.h"
#include "SFNarrativeReplicationTypes.h"
#include "SFNarrativeClientSyncTypes.generated.h"

/**
 * High-level category of a client-visible narrative change.
 * This is used purely for routing/UX (HUD vs codex vs log).
 */
UENUM(BlueprintType)
enum class ESFNarrativeClientChangeCategory : uint8
{
    None,

    WorldFact,
    Faction,
    IdentityAxis,
    Ending,

    QuestState,
    QuestObjective,
    LifetimeTask,

    Dialogue,
    Tutorial,

    Custom
};

/**
 * Severity / "volume" for UI presentation.
 */
UENUM(BlueprintType)
enum class ESFNarrativeClientChangeUrgency : uint8
{
    Quiet,      // Log-only, no pop-up.
    Normal,     // Standard toast / HUD update.
    Important,  // Emphasized toast.
    Critical    // Big banner, pausing UI, etc.
};

/**
 * A single, UI-facing narrative change produced on the client after
 * applying a replicated delta or local narrative event.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeClientSyncEvent
{
    GENERATED_BODY()

    /** High-level routing category (what kind of change this is). */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client")
    ESFNarrativeClientChangeCategory Category = ESFNarrativeClientChangeCategory::None;

    /** UI urgency / emphasis. */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client")
    ESFNarrativeClientChangeUrgency Urgency = ESFNarrativeClientChangeUrgency::Normal;

    /** Original replicated delta that caused this event (if any). */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client")
    FSFNarrativeReplicatedDelta SourceDelta;

    /** Optional quest asset id (for quest-related changes). */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client")
    FPrimaryAssetId QuestAssetId;

    /** Optional objective or state id. */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client")
    FName ObjectiveOrStateId = NAME_None;

    /** Generic descriptive tags for this event (used by UI filters). */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client")
    FGameplayTagContainer Tags;

    /** Human-readable title line for UI. */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client")
    FText Title;

    /** Optional body/description for UI. */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client")
    FText Body;

    /** Optional small numeric payload (e.g. new score, new count). */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client")
    float NumericValue = 0.0f;

    /** True if NumericValue is meaningful. */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client")
    bool bHasNumericValue = false;

    /** Optional before/after values (for e.g. faction/identity bars). */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client")
    float OldValue = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client")
    float NewValue = 0.0f;

    /** Optional small enum payload (e.g. quest completion state). */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client")
    uint8 ByteValue0 = 0;

    /** True if this event should be added to the in-game narrative log. */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client")
    bool bLogInHistory = true;

    /** Optional per-event unique id (for de-duplication in UI). */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client")
    FGuid EventId;

    FSFNarrativeClientSyncEvent()
    {
        EventId = FGuid::NewGuid();
    }
};

/**
 * Compact summary of a quest for HUD display.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeClientQuestSummary
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client|Quest")
    FPrimaryAssetId QuestAssetId;

    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client|Quest")
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client|Quest")
    ESFQuestCompletionState CompletionState = ESFQuestCompletionState::NotStarted;

    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client|Quest")
    FName CurrentStateId = NAME_None;

    /** Objectives the HUD should currently show (already localized). */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client|Quest")
    TArray<FText> ActiveObjectiveTexts;

    /** Whether this quest should be pinned in the HUD. */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client|Quest")
    bool bPinned = false;
};

/**
 * Snapshot of the current "HUD-facing" narrative state.
 * This is built on the client by reading from subsystems and
 * using whatever localization/formatting rules you like.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeClientHudState
{
    GENERATED_BODY()

    /** Quests visible in the HUD. */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client|HUD")
    TArray<FSFNarrativeClientQuestSummary> QuestSummaries;

    /** Factions that should show in the HUD / reputation screen. */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client|HUD")
    TArray<FSFFactionSnapshot> FactionSnapshots;

    /** Identity axes visible in the HUD (bars, graphs). */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client|HUD")
    TArray<FSFIdentityAxisValue> IdentityAxes;

    /** Optional currently active ending candidate, if any. */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client|HUD")
    FGameplayTag HighlightedEndingTag;
};

/**
 * A batch of client sync events produced in a single frame/tick.
 * Your narrative client sync subsystem can broadcast this to UI.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeClientSyncBatch
{
    GENERATED_BODY()

    /** All narrative events produced since the last sync. */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client")
    TArray<FSFNarrativeClientSyncEvent> Events;

    /** Optional full HUD snapshot (if something invalidated the view). */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client")
    bool bHasHudState = false;

    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Client", meta = (EditCondition = "bHasHudState"))
    FSFNarrativeClientHudState HudState;

    bool IsEmpty() const
    {
        return Events.Num() == 0 && !bHasHudState;
    }

    void Reset()
    {
        Events.Reset();
        bHasHudState = false;
        HudState = FSFNarrativeClientHudState();
    }
};
