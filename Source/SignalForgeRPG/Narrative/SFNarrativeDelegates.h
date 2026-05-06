// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeStructs.h"
#include "SFNarrativeDelegates.generated.h"

/**
 * Delegate signatures for the narrative system, grouped by domain.
 *
 * Bind these to UPROPERTY(BlueprintAssignable) members on your component/subsystem,
 * e.g.:
 *
 *   UPROPERTY(BlueprintAssignable, Category = "Narrative")
 *   FSFOnQuestSnapshotChanged OnQuestSnapshotChanged;
 */

 //
 // Quest delegates
 //

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FSFOnQuestSnapshotChanged,
    const FSFQuestSnapshot&, NewSnapshot);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FSFOnQuestCompletionStateChanged,
    FName, QuestId,
    ESFQuestCompletionState, OldState,
    ESFQuestCompletionState, NewState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FSFOnQuestStateEntered,
    FName, QuestId,
    FName, PreviousStateId,
    FName, NewStateId);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FSFOnQuestTaskProgressChanged,
    const FSFTaskProgressResult&, Result);

//
// Dialogue delegates
//

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
    FSFOnDialogueStarted,
    FName, DialogueId,
    const FSFDialogueStartParams&, StartParams,
    const FSFNarrativeInstigator&, Instigator,
    int32, Sequence);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
    FSFOnDialogueEnded,
    FName, DialogueId,
    ESFDialogueExitReason, Reason,
    const FSFNarrativeInstigator&, Instigator,
    int32, Sequence);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FSFOnDialogueMomentUpdated,
    FName, DialogueId,
    const FSFDialogueMoment&, Moment,
    int32, Sequence);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
    FSFOnDialogueOptionSelected,
    FName, DialogueId,
    const FSFDialogueOptionView&, Option,
    const FSFNarrativeInstigator&, Instigator,
    int32, Sequence);

//
// World fact delegates
//

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FSFOnWorldFactChanged,
    const FSFWorldFactChange&, Change);

//
// Faction delegates
//

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FSFOnFactionStandingChanged,
    const FSFFactionChange&, Change);

//
// Identity delegates
//

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FSFOnIdentityAxisChanged,
    const FSFIdentityChange&, Change);

//
// Outcome / ending delegates
//

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FSFOnOutcomeApplied,
    const FSFOutcomeApplication&, OutcomeApplication);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FSFOnEndingStateChanged,
    const FSFEndingState&, EndingState);

//
// Replication / system delegates
//

/** Low-level stream of narrative wire deltas (for replicators, recorders, analytics). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FSFOnNarrativeDeltaEmitted,
    const FSFNarrativeDelta&, Delta);

/** High-level change set emitted after an operation is fully applied. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FSFOnNarrativeChangeSetEmitted,
    const FSFNarrativeChangeSet&, ChangeSet);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FSFOnNarrativeCheckpointSaved,
    const FSFNarrativeSaveData&, SaveData,
    const FSFNarrativeRedirectTable&, Redirects);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FSFOnNarrativeStateResynced,
    const FSFNarrativeSaveData&, NewState,
    int32, SchemaVersion);

/** Catch-all native broadcast used by cheap observers that don't need typed payload. */
DECLARE_MULTICAST_DELEGATE(FSFOnNarrativeDirtyNative);

/** Single-cast native delegate for tight internal hookups (no Blueprint bind). */
DECLARE_DELEGATE_OneParam(FSFOnNarrativeDeltaNative, const FSFNarrativeDelta&);
