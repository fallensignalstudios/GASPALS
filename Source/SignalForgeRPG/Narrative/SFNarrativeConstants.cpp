// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "SFNarrativeConstants.h"

const FName FSFNarrativeConstants::DefaultQuestStartStateId(TEXT("Start"));
const FName FSFNarrativeConstants::DefaultDialogueStartNodeId(TEXT("Start"));

ESFFactionStandingBand FSFNarrativeConstants::ClassifyFactionStanding(float StandingValue)
{
    if (StandingValue <= StandingBandHostileMax) { return ESFFactionStandingBand::Hostile; }
    if (StandingValue <= StandingBandAdversarialMax) { return ESFFactionStandingBand::Adversarial; }
    if (StandingValue <= StandingBandWaryMax) { return ESFFactionStandingBand::Wary; }
    if (StandingValue <= StandingBandNeutralMax) { return ESFFactionStandingBand::Neutral; }
    if (StandingValue <= StandingBandCooperativeMax) { return ESFFactionStandingBand::Cooperative; }
    if (StandingValue <= StandingBandAlliedMax) { return ESFFactionStandingBand::Allied; }
    return ESFFactionStandingBand::Bound;
}

float FSFNarrativeConstants::ClampFactionMetric(float Value)
{
    return FMath::Clamp(Value, FactionMetricMin, FactionMetricMax);
}

float FSFNarrativeConstants::ClampIdentityAxisValue(float Value)
{
    return FMath::Clamp(Value, IdentityAxisMin, IdentityAxisMax);
}

ESFIdentityDriftDirection FSFNarrativeConstants::ClassifyIdentityDrift(float Delta)
{
    if (FMath::Abs(Delta) <= IdentityDeltaEpsilon) { return ESFIdentityDriftDirection::None; }
    if (Delta > IdentityPositiveThreshold) { return ESFIdentityDriftDirection::Positive; }
    if (Delta < IdentityNegativeThreshold) { return ESFIdentityDriftDirection::Negative; }
    return ESFIdentityDriftDirection::None;
}
