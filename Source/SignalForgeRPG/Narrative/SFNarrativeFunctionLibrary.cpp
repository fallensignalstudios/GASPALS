// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "SFNarrativeFunctionLibrary.h"

void USFNarrativeFunctionLibrary::MergeChangeSets(
    FSFNarrativeChangeSet& InOutA,
    const FSFNarrativeChangeSet& B)
{
    InOutA.SourceTags.AppendTags(B.SourceTags);
    InOutA.TaskResults.Append(B.TaskResults);
    InOutA.WorldFactChanges.Append(B.WorldFactChanges);
    InOutA.FactionChanges.Append(B.FactionChanges);
    InOutA.IdentityChanges.Append(B.IdentityChanges);
    InOutA.AppliedOutcomes.Append(B.AppliedOutcomes);
    InOutA.EndingStatesChanged.Append(B.EndingStatesChanged);
}

bool USFNarrativeFunctionLibrary::IsChangeSetEmpty(
    const FSFNarrativeChangeSet& ChangeSet)
{
    return ChangeSet.IsEmpty();
}

FSFNarrativeDelta USFNarrativeFunctionLibrary::MakeBoolFactDelta(
    const FSFWorldFactKey& FactKey,
    bool bNewValue)
{
    return FSFNarrativeDelta::MakeSetWorldFact(
        0,
        FactKey.FactTag,
        FactKey.ContextId,
        FSFWorldFactValue::MakeBool(bNewValue));
}

FSFNarrativeDelta USFNarrativeFunctionLibrary::MakeFloatFactDelta(
    const FSFWorldFactKey& FactKey,
    float NewValue)
{
    return FSFNarrativeDelta::MakeSetWorldFact(
        0,
        FactKey.FactTag,
        FactKey.ContextId,
        FSFWorldFactValue::MakeFloat(NewValue));
}

FSFNarrativeDelta USFNarrativeFunctionLibrary::MakeIdentityAxisAddDelta(
    FGameplayTag AxisTag,
    float DeltaAmount)
{
    return FSFNarrativeDelta::MakeApplyIdentityDelta(0, AxisTag, DeltaAmount);
}

FSFNarrativeDelta USFNarrativeFunctionLibrary::MakeFactionStandingAddDelta(
    FGameplayTag FactionTag,
    float DeltaAmount)
{
    FSFFactionDelta Delta;
    Delta.FactionTag = FactionTag;
    Delta.TrustDelta = DeltaAmount; // Proxy old single-score usage onto Trust in the canonical faction schema.
    return FSFNarrativeDelta::MakeApplyFactionDelta(0, Delta);
}

bool USFNarrativeFunctionLibrary::IsDeltaNoOp(
    const FSFNarrativeDelta& Delta)
{
    switch (Delta.Type)
    {
    case ESFNarrativeDeltaType::None:
        return true;

    case ESFNarrativeDeltaType::SetWorldFact:
        return !Delta.Tag0.IsValid() || Delta.SubjectId.IsNone();

    case ESFNarrativeDeltaType::AddWorldFactTag:
    case ESFNarrativeDeltaType::RemoveWorldFactTag:
        return !Delta.Tag0.IsValid() || Delta.SubjectId.IsNone() || !Delta.Tag1.IsValid();

    case ESFNarrativeDeltaType::ApplyFactionDelta:
        return !FSFNarrativeDelta::UnpackFactionDelta(Delta).FactionTag.IsValid()
            || FSFNarrativeDelta::UnpackFactionDelta(Delta).IsZero();

    case ESFNarrativeDeltaType::SetFactionStandingBand:
        return !Delta.Tag0.IsValid();

    case ESFNarrativeDeltaType::ApplyIdentityDelta:
        return !Delta.Tag0.IsValid() || FMath::IsNearlyZero(Delta.Float0);

    case ESFNarrativeDeltaType::ApplyOutcome:
        return !FSFNarrativeDelta::UnpackOutcomeAssetId(Delta).IsValid();

    case ESFNarrativeDeltaType::SetEndingAvailability:
        return !Delta.Tag0.IsValid();

    case ESFNarrativeDeltaType::ApplyEndingScoreDelta:
        return !Delta.Tag0.IsValid() || FMath::IsNearlyZero(Delta.Float0);

    case ESFNarrativeDeltaType::BeginDialogue:
    case ESFNarrativeDeltaType::ExitDialogue:
    case ESFNarrativeDeltaType::AdvanceDialogue:
    case ESFNarrativeDeltaType::SelectDialogueOption:
    case ESFNarrativeDeltaType::TriggerDialogueEvent:
    case ESFNarrativeDeltaType::StartQuest:
    case ESFNarrativeDeltaType::RestartQuest:
    case ESFNarrativeDeltaType::AbandonQuest:
    case ESFNarrativeDeltaType::EnterQuestState:
    case ESFNarrativeDeltaType::CompleteQuestTask:
    case ESFNarrativeDeltaType::TaskProgress:
    case ESFNarrativeDeltaType::SaveCheckpointLoaded:
    case ESFNarrativeDeltaType::FullStateResync:
    default:
        return false;
    }
}

float USFNarrativeFunctionLibrary::NormalizeClamped(
    float Value, float Min, float Max)
{
    if (Min >= Max)
    {
        return 0.0f;
    }
    const float T = (Value - Min) / (Max - Min);
    return FMath::Clamp(T, 0.0f, 1.0f);
}
