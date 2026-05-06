#include "SFDialogueNarrativeHooks.h"
#include "SFNarrativeComponent.h"
#include "SFNarrativeLog.h"

bool USFDialogueNarrativeHooks::CanPlayDialogueNode(
    const FGameplayTagContainer& OwnedTags,
    const FGameplayTagContainer& RequiredTags,
    const FGameplayTagContainer& BlockedTags)
{
    if (OwnedTags.HasAny(BlockedTags))
    {
        return false;
    }

    if (!RequiredTags.IsEmpty() && !OwnedTags.HasAll(RequiredTags))
    {
        return false;
    }

    return true;
}

bool USFDialogueNarrativeHooks::CanShowDialogueChoice(
    const FGameplayTagContainer& OwnedTags,
    const FGameplayTagContainer& RequiredTags,
    const FGameplayTagContainer& BlockedTags)
{
    return CanPlayDialogueNode(OwnedTags, RequiredTags, BlockedTags);
}

FSFNarrativeChangeSet USFDialogueNarrativeHooks::ApplyNodeNarrativeEffects(
    USFNarrativeComponent* NarrativeComponent,
    const FName DialogueId,
    const FName NodeId,
    const FSFDialogueNarrativeEffect& Effect)
{
    FSFNarrativeChangeSet ChangeSet;
    ChangeSet.SourceTags.AddTagFast(FGameplayTag::RequestGameplayTag(TEXT("Narrative.Source.Dialogue.Node")));
    ChangeSet.SourceTags.AddTagFast(FGameplayTag::RequestGameplayTag(TEXT("Narrative.Source.Dialogue")));

    if (!NarrativeComponent)
    {
        NARR_LOG_WARNING(TEXT("ApplyNodeNarrativeEffects called with null NarrativeComponent for DialogueId=%s NodeId=%s"),
            *DialogueId.ToString(), *NodeId.ToString());
        return ChangeSet;
    }

    int32 Sequence = 0;

    EmitWorldFactDeltas(NarrativeComponent, DialogueId, Effect.WorldFactsToSet, Sequence, ChangeSet);
    EmitFactionDeltas(NarrativeComponent, Effect.FactionDeltas, Sequence, ChangeSet);
    EmitIdentityDeltas(NarrativeComponent, Effect.IdentityDeltas, Sequence, ChangeSet);
    EmitOutcomeDeltas(NarrativeComponent, DialogueId, Effect.OutcomesToApply, Sequence, ChangeSet);
    EmitEndingDeltas(NarrativeComponent, Effect.EndingScoreDeltas, Sequence, ChangeSet);

    // If you maintain global narrative tags on the component, you can apply NarrativeTagsToApply here.
    if (!Effect.NarrativeTagsToApply.IsEmpty())
    {
        // Example: NarrativeComponent->AddNarrativeTags(Effect.NarrativeTagsToApply);
    }

    return ChangeSet;
}

FSFNarrativeChangeSet USFDialogueNarrativeHooks::ApplyChoiceNarrativeEffects(
    USFNarrativeComponent* NarrativeComponent,
    const FName DialogueId,
    const FName ChoiceNodeId,
    const FSFDialogueNarrativeEffect& Effect,
    const FName SpeakerId)
{
    FSFNarrativeChangeSet ChangeSet;
    ChangeSet.SourceTags.AddTagFast(FGameplayTag::RequestGameplayTag(TEXT("Narrative.Source.Dialogue.Choice")));
    ChangeSet.SourceTags.AddTagFast(FGameplayTag::RequestGameplayTag(TEXT("Narrative.Source.Dialogue")));

    if (!NarrativeComponent)
    {
        NARR_LOG_WARNING(TEXT("ApplyChoiceNarrativeEffects called with null NarrativeComponent for DialogueId=%s NodeId=%s"),
            *DialogueId.ToString(), *ChoiceNodeId.ToString());
        return ChangeSet;
    }

    int32 Sequence = 0;

    EmitWorldFactDeltas(NarrativeComponent, DialogueId, Effect.WorldFactsToSet, Sequence, ChangeSet);
    EmitFactionDeltas(NarrativeComponent, Effect.FactionDeltas, Sequence, ChangeSet);
    EmitIdentityDeltas(NarrativeComponent, Effect.IdentityDeltas, Sequence, ChangeSet);
    EmitOutcomeDeltas(NarrativeComponent, DialogueId, Effect.OutcomesToApply, Sequence, ChangeSet);
    EmitEndingDeltas(NarrativeComponent, Effect.EndingScoreDeltas, Sequence, ChangeSet);

    if (!Effect.NarrativeTagsToApply.IsEmpty())
    {
        // Example: NarrativeComponent->AddNarrativeTags(Effect.NarrativeTagsToApply);
    }

    return ChangeSet;
}

void USFDialogueNarrativeHooks::EmitWorldFactDeltas(
    USFNarrativeComponent* NarrativeComponent,
    const FName SubjectId,
    const TArray<FSFWorldFactSnapshot>& Facts,
    int32& InOutSequence,
    FSFNarrativeChangeSet& OutChangeSet)
{
    for (const FSFWorldFactSnapshot& Snapshot : Facts)
    {
        FSFNarrativeDelta Delta = FSFNarrativeDelta::MakeSetWorldFact(
            ++InOutSequence,
            Snapshot.Key.FactTag,
            Snapshot.Value);

        NarrativeComponent->HandleExternalNarrativeDelta(Delta);

        FSFWorldFactChange Change;
        Change.Key = Snapshot.Key;
        Change.OldValue = FSFWorldFactValue(); // If you want old value, fetch from state subsystem first.
        Change.NewValue = Snapshot.Value;

        OutChangeSet.WorldFactChanges.Add(Change);
    }
}

void USFDialogueNarrativeHooks::EmitFactionDeltas(
    USFNarrativeComponent* NarrativeComponent,
    const TArray<FSFFactionDelta>& Deltas,
    int32& InOutSequence,
    FSFNarrativeChangeSet& OutChangeSet)
{
    for (const FSFFactionDelta& FactionDelta : Deltas)
    {
        FSFNarrativeDelta Delta = FSFNarrativeDelta::MakeApplyFactionDelta(
            ++InOutSequence,
            FactionDelta);

        NarrativeComponent->HandleExternalNarrativeDelta(Delta);

        // The actual FSFFactionChange (Old/New) should be built by your faction state subsystem
        // and then reported via a ChangeSet. Here we just record that something happened,
        // leaving detailed values to the subsystem.
    }
}

void USFDialogueNarrativeHooks::EmitIdentityDeltas(
    USFNarrativeComponent* NarrativeComponent,
    const TArray<FSFIdentityDelta>& Deltas,
    int32& InOutSequence,
    FSFNarrativeChangeSet& OutChangeSet)
{
    for (const FSFIdentityDelta& IdentityDelta : Deltas)
    {
        FSFNarrativeDelta Delta = FSFNarrativeDelta::MakeApplyIdentityDelta(
            ++InOutSequence,
            IdentityDelta.AxisTag,
            IdentityDelta.Delta);

        NarrativeComponent->HandleExternalNarrativeDelta(Delta);

        // Detailed FSFIdentityChange (Old/New/DriftDir) comes from identity subsystem later.
    }
}

void USFDialogueNarrativeHooks::EmitOutcomeDeltas(
    USFNarrativeComponent* NarrativeComponent,
    const FName SubjectId,
    const TArray<FSFOutcomeApplication>& Outcomes,
    int32& InOutSequence,
    FSFNarrativeChangeSet& OutChangeSet)
{
    for (const FSFOutcomeApplication& Application : Outcomes)
    {
        FSFNarrativeDelta Delta = FSFNarrativeDelta::MakeApplyOutcome(
            ++InOutSequence,
            Application.OutcomeAssetId.PrimaryAssetName,
            Application.ContextId);

        NarrativeComponent->HandleExternalNarrativeDelta(Delta);
        OutChangeSet.AppliedOutcomes.Add(Application);
    }
}

void USFDialogueNarrativeHooks::EmitEndingDeltas(
    USFNarrativeComponent* NarrativeComponent,
    const TMap<FGameplayTag, float>& EndingScoreDeltas,
    int32& InOutSequence,
    FSFNarrativeChangeSet& OutChangeSet)
{
    for (const TPair<FGameplayTag, float>& Pair : EndingScoreDeltas)
    {
        const FGameplayTag& EndingTag = Pair.Key;
        const float ScoreDelta = Pair.Value;

        // For now we just emit an ApplyEndingScoreDelta-style delta; the subsystem
        // is responsible for computing the new score / availability and then
        // emitting SetEndingAvailability.
        FSFNarrativeDelta Delta;
        Delta.Type = ESFNarrativeDeltaType::ApplyEndingScoreDelta;
        Delta.Sequence = ++InOutSequence;
        Delta.Tag0 = EndingTag;
        Delta.Float0 = ScoreDelta;

        NarrativeComponent->HandleExternalNarrativeDelta(Delta);
    }
}