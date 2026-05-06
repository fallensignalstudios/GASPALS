#include "SFNarrativeEventRouter.h"
#include "SFNarrativeEventHub.h"
#include "SFNarrativeLog.h"
#include "SFNarrativeConstants.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerState.h"

void USFNarrativeEventRouter::Initialize(USFNarrativeEventHub* InEventHub, UObject* InOwnerContext)
{
    EventHub = InEventHub;
    OwnerContext = InOwnerContext;
}

void USFNarrativeEventRouter::RouteDelta(const FSFNarrativeDelta& Delta)
{
    if (!EventHub)
    {
        return;
    }

    switch (Delta.Type)
    {
    case ESFNarrativeDeltaType::StartQuest:
    case ESFNarrativeDeltaType::RestartQuest:
    case ESFNarrativeDeltaType::AbandonQuest:
    case ESFNarrativeDeltaType::EnterQuestState:
    case ESFNarrativeDeltaType::CompleteQuestTask:
    case ESFNarrativeDeltaType::TaskProgress:
        RouteQuestDelta(Delta);
        break;

    case ESFNarrativeDeltaType::BeginDialogue:
    case ESFNarrativeDeltaType::ExitDialogue:
    case ESFNarrativeDeltaType::AdvanceDialogue:
    case ESFNarrativeDeltaType::SelectDialogueOption:
    case ESFNarrativeDeltaType::TriggerDialogueEvent:
        RouteDialogueDelta(Delta);
        break;

    case ESFNarrativeDeltaType::SetWorldFact:
    case ESFNarrativeDeltaType::AddWorldFactTag:
    case ESFNarrativeDeltaType::RemoveWorldFactTag:
        RouteWorldFactDelta(Delta);
        break;

    case ESFNarrativeDeltaType::ApplyFactionDelta:
    case ESFNarrativeDeltaType::SetFactionStandingBand:
        RouteFactionDelta(Delta);
        break;

    case ESFNarrativeDeltaType::ApplyIdentityDelta:
        RouteIdentityDelta(Delta);
        break;

    case ESFNarrativeDeltaType::ApplyOutcome:
        RouteOutcomeDelta(Delta);
        break;

    case ESFNarrativeDeltaType::SetEndingAvailability:
    case ESFNarrativeDeltaType::ApplyEndingScoreDelta:
        RouteEndingDelta(Delta);
        break;

    case ESFNarrativeDeltaType::SaveCheckpointLoaded:
    case ESFNarrativeDeltaType::FullStateResync:
        RouteSystemDelta(Delta);
        break;

    default:
        break;
    }
}

void USFNarrativeEventRouter::RouteChangeSet(const FSFNarrativeChangeSet& ChangeSet)
{
    if (!EventHub)
    {
        return;
    }

    RouteTaskResults(ChangeSet.TaskResults);
    RouteWorldFactChanges(ChangeSet.WorldFactChanges);
    RouteFactionChanges(ChangeSet.FactionChanges);
    RouteIdentityChanges(ChangeSet.IdentityChanges);
    RouteAppliedOutcomes(ChangeSet.AppliedOutcomes);
    RouteEndingStates(ChangeSet.EndingStatesChanged);
}

//
// Quest
//

void USFNarrativeEventRouter::RouteQuestDelta(const FSFNarrativeDelta& Delta)
{
    if (!EventHub)
    {
        return;
    }

    const FName QuestId = Delta.SubjectId;

    USFQuestInstance* QuestInstance = ResolveQuestInstance(QuestId);

    switch (Delta.Type)
    {
    case ESFNarrativeDeltaType::StartQuest:
        EventHub->BroadcastQuestStarted(QuestInstance);
        break;

    case ESFNarrativeDeltaType::RestartQuest:
        EventHub->BroadcastQuestRestarted(QuestInstance);
        break;

    case ESFNarrativeDeltaType::AbandonQuest:
        EventHub->BroadcastQuestAbandoned(QuestInstance);
        break;

    case ESFNarrativeDeltaType::EnterQuestState:
        EventHub->BroadcastQuestStateChanged(QuestInstance, Delta.Arg0);
        break;

    case ESFNarrativeDeltaType::CompleteQuestTask:
    {
        // We assume Tag0 = TaskTag, Arg0 = ContextId, Int0 = Quantity.
        const FGameplayTag TaskTag = Delta.Tag0;
        const FName ContextId = Delta.Arg0;
        const int32 Quantity = Delta.Int0;
        EventHub->BroadcastQuestTaskProgressed(QuestInstance, TaskTag, ContextId, Quantity);
        EventHub->BroadcastNarrativeTaskCompleted(TaskTag, ContextId, Quantity);
        break;
    }

    case ESFNarrativeDeltaType::TaskProgress:
    {
        // For incremental task progress that might not complete.
        const FGameplayTag TaskTag = Delta.Tag0;
        const FName ContextId = Delta.Arg0;
        const int32 Quantity = Delta.Int0;
        EventHub->BroadcastQuestTaskProgressed(QuestInstance, TaskTag, ContextId, Quantity);
        break;
    }

    default:
        break;
    }
}

//
// Dialogue
//

void USFNarrativeEventRouter::RouteDialogueDelta(const FSFNarrativeDelta& Delta)
{
    if (!EventHub)
    {
        return;
    }

    // In your current hub, dialogue events are driven by USFConversationDataAsset*.
    // At the router level we only know FName IDs; typically you’ll keep the hub wiring
    // nearer to the dialogue component. Here we only reflect tag?style events that
    // don’t require the data asset.

    switch (Delta.Type)
    {
    case ESFNarrativeDeltaType::TriggerDialogueEvent:
        EventHub->BroadcastDialogueEventTriggered(Delta.Tag0);
        break;

    default:
        // Start / End / Advance / Choice events can stay where you currently fire them
        // from the dialogue / bridge layer using the existing hub APIs.
        break;
    }
}

//
// World facts
//

void USFNarrativeEventRouter::RouteWorldFactDelta(const FSFNarrativeDelta& Delta)
{
    if (!EventHub)
    {
        return;
    }

    const FGameplayTag FactTag = Delta.Tag0;
    const FName ContextId = Delta.Arg0;

    switch (Delta.Type)
    {
    case ESFNarrativeDeltaType::SetWorldFact:
    {
        FSFWorldFactValue Value;
        Value.ValueType = ESFNarrativeFactValueType::None;
        // The delta packs primitive fields into Bool0/Int0/Float0/Arg0/Tag1.
        Value.BoolValue = Delta.bBool0;
        Value.IntValue = Delta.Int0;
        Value.FloatValue = Delta.Float0;
        Value.NameValue = Delta.Arg0;
        Value.TagValue = Delta.Tag1;

        EventHub->BroadcastWorldFactChanged(FactTag, ContextId, Value);
        break;
    }

    case ESFNarrativeDeltaType::AddWorldFactTag:
    case ESFNarrativeDeltaType::RemoveWorldFactTag:
        // Treat these as structural updates; details are handled by state subsystem.
        // For now, we just broadcast a "removed" when appropriate.
        if (Delta.Type == ESFNarrativeDeltaType::RemoveWorldFactTag)
        {
            EventHub->BroadcastWorldFactRemoved(FactTag, ContextId);
        }
        break;

    default:
        break;
    }
}

//
// Factions
//

void USFNarrativeEventRouter::RouteFactionDelta(const FSFNarrativeDelta& Delta)
{
    if (!EventHub)
    {
        return;
    }

    const FGameplayTag FactionTag = Delta.Tag0;

    switch (Delta.Type)
    {
    case ESFNarrativeDeltaType::ApplyFactionDelta:
        // Detailed standing updates should come via FSFFactionChange in a ChangeSet.
        // Here we only know raw deltas, so we leave the heavy lifting to the state subsystem.
        break;

    case ESFNarrativeDeltaType::SetFactionStandingBand:
    {
        const ESFFactionStandingBand NewBand = static_cast<ESFFactionStandingBand>(Delta.Int0);
        EventHub->BroadcastFactionStandingBandChanged(FactionTag, NewBand);
        break;
    }

    default:
        break;
    }
}

//
// Identity
//

void USFNarrativeEventRouter::RouteIdentityDelta(const FSFNarrativeDelta& Delta)
{
    if (!EventHub)
    {
        return;
    }

    const FGameplayTag AxisTag = Delta.Tag0;
    const float DeltaValue = Delta.Float0;

    // The actual old/new values are tracked in state; from the raw delta alone we
    // can only infer direction. For richer events we rely on FSFIdentityChange via ChangeSet.
    ESFIdentityDriftDirection Drift = ESFIdentityDriftDirection::None;
    if (DeltaValue > FSFNarrativeConstants::IdentityPositiveThreshold)
    {
        Drift = ESFIdentityDriftDirection::Positive;
    }
    else if (DeltaValue < FSFNarrativeConstants::IdentityNegativeThreshold)
    {
        Drift = ESFIdentityDriftDirection::Negative;
    }

    if (Drift != ESFIdentityDriftDirection::None)
    {
        EventHub->BroadcastIdentityDriftDirectionChanged(AxisTag, Drift);
    }
}

//
// Outcomes
//

void USFNarrativeEventRouter::RouteOutcomeDelta(const FSFNarrativeDelta& Delta)
{
    if (!EventHub)
    {
        return;
    }

    const FName OutcomeId = Delta.SubjectId;
    const FName ContextId = Delta.Arg0;

    FPrimaryAssetId OutcomeAssetId;
    OutcomeAssetId.PrimaryAssetType = TEXT("Outcome");
    OutcomeAssetId.PrimaryAssetName = OutcomeId;

    EventHub->BroadcastOutcomeApplied(OutcomeAssetId, ContextId);
}

//
// Endings
//

void USFNarrativeEventRouter::RouteEndingDelta(const FSFNarrativeDelta& Delta)
{
    if (!EventHub)
    {
        return;
    }

    const FGameplayTag EndingTag = Delta.Tag0;

    switch (Delta.Type)
    {
    case ESFNarrativeDeltaType::SetEndingAvailability:
    {
        const ESFEndingAvailability Availability = static_cast<ESFEndingAvailability>(Delta.Int0);
        const float Score = Delta.Float0;
        EventHub->BroadcastEndingStateChanged(EndingTag, Availability, Score);

        if (Availability == ESFEndingAvailability::Locked)
        {
            EventHub->BroadcastEndingLocked(EndingTag);
        }
        break;
    }

    case ESFNarrativeDeltaType::ApplyEndingScoreDelta:
        // Final scores & availability should be reflected via FSFEndingState in a ChangeSet.
        break;

    default:
        break;
    }
}

//
// System / save
//

void USFNarrativeEventRouter::RouteSystemDelta(const FSFNarrativeDelta& Delta)
{
    if (!EventHub)
    {
        return;
    }

    switch (Delta.Type)
    {
    case ESFNarrativeDeltaType::SaveCheckpointLoaded:
        // Slot name typically exists elsewhere; here we just raise the point?of?no?return style notification
        // via a gameplay tag if you decide to encode checkpoint tags in Tag0 later.
        break;

    case ESFNarrativeDeltaType::FullStateResync:
        // A full resync is already exposed on the hub via save events; actual broadcasting
        // should happen near your save/load service when it knows the slot name.
        break;

    default:
        break;
    }
}

//
// ChangeSet routing – richer, stateful events
//

void USFNarrativeEventRouter::RouteTaskResults(const TArray<FSFTaskProgressResult>& TaskResults)
{
    if (!EventHub)
    {
        return;
    }

    for (const FSFTaskProgressResult& Result : TaskResults)
    {
        const FGameplayTag TaskTag = Result.Key.TaskTag;
        const FName ContextId = Result.Key.ContextId;
        const int32 Quantity = Result.NewCount;

        EventHub->BroadcastQuestTaskProgressed(nullptr /*QuestInstance*/, TaskTag, ContextId, Quantity);

        if (Result.bTaskCompleted)
        {
            EventHub->BroadcastNarrativeTaskCompleted(TaskTag, ContextId, Quantity);
        }
    }
}

void USFNarrativeEventRouter::RouteWorldFactChanges(const TArray<FSFWorldFactChange>& FactChanges)
{
    if (!EventHub)
    {
        return;
    }

    for (const FSFWorldFactChange& Change : FactChanges)
    {
        if (!Change.HasChanged())
        {
            continue;
        }

        EventHub->BroadcastWorldFactChanged(Change.Key.FactTag, Change.Key.ContextId, Change.NewValue);
    }
}

void USFNarrativeEventRouter::RouteFactionChanges(const TArray<FSFFactionChange>& FactionChanges)
{
    if (!EventHub)
    {
        return;
    }

    for (const FSFFactionChange& Change : FactionChanges)
    {
        EventHub->BroadcastFactionStandingChanged(Change.FactionTag, Change.OldStanding, Change.NewStanding);

        if (Change.DidStandingBandChange())
        {
            EventHub->BroadcastFactionStandingBandChanged(Change.FactionTag, Change.NewStanding.StandingBand);
        }

        CachedFactionStanding.FindOrAdd(Change.FactionTag) = Change.NewStanding;
    }
}

void USFNarrativeEventRouter::RouteIdentityChanges(const TArray<FSFIdentityChange>& IdentityChanges)
{
    if (!EventHub)
    {
        return;
    }

    for (const FSFIdentityChange& Change : IdentityChanges)
    {
        const FGameplayTag AxisTag = Change.NewValue.AxisTag;
        const float OldValue = Change.OldValue.Value;
        const float NewValue = Change.NewValue.Value;

        EventHub->BroadcastIdentityAxisChanged(AxisTag, OldValue, NewValue);
        EventHub->BroadcastIdentityDriftDirectionChanged(AxisTag, Change.DriftDirection);

        CachedIdentityValues.FindOrAdd(AxisTag) = NewValue;
    }
}

void USFNarrativeEventRouter::RouteAppliedOutcomes(const TArray<FSFOutcomeApplication>& Outcomes)
{
    if (!EventHub)
    {
        return;
    }

    for (const FSFOutcomeApplication& Application : Outcomes)
    {
        EventHub->BroadcastOutcomeApplied(Application.OutcomeAssetId, Application.ContextId);
    }
}

void USFNarrativeEventRouter::RouteEndingStates(const TArray<FSFEndingState>& EndingStates)
{
    if (!EventHub)
    {
        return;
    }

    for (const FSFEndingState& Ending : EndingStates)
    {
        EventHub->BroadcastEndingStateChanged(Ending.EndingTag, Ending.Availability, Ending.Score);

        if (Ending.Availability == ESFEndingAvailability::Locked)
        {
            EventHub->BroadcastEndingLocked(Ending.EndingTag);
        }
    }
}

//
// Quest instance resolution – placeholder
//

USFQuestInstance* USFNarrativeEventRouter::ResolveQuestInstance(FName QuestId) const
{
    // Intentionally left as a hook. Depending on your architecture, OwnerContext might be:
    //  - USFNarrativeComponent (which can look up quest instances by ID)
    //  - a world?level quest manager
    //  - or null (in tools/tests).
    //
    // You can safely return nullptr here; the hub already accepts a null QuestInstance
    // and many listeners will just rely on tags / IDs.

    return nullptr;
}