// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "SFNarrativeFunctionLibrary.h"

void USFNarrativeFunctionLibrary::MergeChangeSets(
    FSFNarrativeChangeSet& InOutA,
    const FSFNarrativeChangeSet& B)
{
    // World facts: B overwrites A for equal keys.
    for (const auto& Pair : B.WorldFactChanges)
    {
        InOutA.WorldFactChanges.Add(Pair.Key, Pair.Value);
    }

    // Faction changes: same key ? last writer wins (B overwrites).
    for (const auto& Pair : B.FactionChanges)
    {
        InOutA.FactionChanges.Add(Pair.Key, Pair.Value);
    }

    // Identity axes.
    for (const auto& Pair : B.IdentityAxisChanges)
    {
        InOutA.IdentityAxisChanges.Add(Pair.Key, Pair.Value);
    }

    // Endings.
    for (const auto& Pair : B.EndingChanges)
    {
        InOutA.EndingChanges.Add(Pair.Key, Pair.Value);
    }

    // Custom payloads (simply append, caller can dedupe if needed).
    InOutA.CustomChanges.Append(B.CustomChanges);
}

bool USFNarrativeFunctionLibrary::IsChangeSetEmpty(
    const FSFNarrativeChangeSet& ChangeSet)
{
    return ChangeSet.WorldFactChanges.Num() == 0 &&
        ChangeSet.FactionChanges.Num() == 0 &&
        ChangeSet.IdentityAxisChanges.Num() == 0 &&
        ChangeSet.EndingChanges.Num() == 0 &&
        ChangeSet.CustomChanges.Num() == 0;
}

FSFNarrativeDelta USFNarrativeFunctionLibrary::MakeBoolFactDelta(
    const FSFWorldFactKey& FactKey,
    bool bNewValue)
{
    FSFNarrativeDelta D;
    D.Domain = ESFNarrativeDeltaDomain::WorldFact;
    D.WorldFact.Key = FactKey;
    D.WorldFact.Operation = ESFWorldFactDeltaOp::Set;

    D.WorldFact.NewValue.Type = ESFNarrativeFactValueType::Bool;
    D.WorldFact.NewValue.BoolValue = bNewValue;

    return D;
}

FSFNarrativeDelta USFNarrativeFunctionLibrary::MakeFloatFactDelta(
    const FSFWorldFactKey& FactKey,
    float NewValue)
{
    FSFNarrativeDelta D;
    D.Domain = ESFNarrativeDeltaDomain::WorldFact;
    D.WorldFact.Key = FactKey;
    D.WorldFact.Operation = ESFWorldFactDeltaOp::Set;

    D.WorldFact.NewValue.Type = ESFNarrativeFactValueType::Float;
    D.WorldFact.NewValue.FloatValue = NewValue;

    return D;
}

FSFNarrativeDelta USFNarrativeFunctionLibrary::MakeIdentityAxisAddDelta(
    FGameplayTag AxisTag,
    float DeltaAmount)
{
    FSFNarrativeDelta D;
    D.Domain = ESFNarrativeDeltaDomain::IdentityAxis;
    D.IdentityAxis.AxisTag = AxisTag;
    D.IdentityAxis.Operation = ESFIdentityAxisDeltaOp::Add;
    D.IdentityAxis.Amount = DeltaAmount;
    return D;
}

FSFNarrativeDelta USFNarrativeFunctionLibrary::MakeFactionStandingAddDelta(
    FGameplayTag FactionTag,
    float DeltaAmount)
{
    FSFNarrativeDelta D;
    D.Domain = ESFNarrativeDeltaDomain::Faction;
    D.Faction.FactionTag = FactionTag;
    D.Faction.Operation = ESFFactionDeltaOp::AddScore;
    D.Faction.Amount = DeltaAmount;
    return D;
}

bool USFNarrativeFunctionLibrary::IsDeltaNoOp(
    const FSFNarrativeDelta& Delta)
{
    if (Delta.Domain == ESFNarrativeDeltaDomain::None)
    {
        return true;
    }

    switch (Delta.Domain)
    {
    case ESFNarrativeDeltaDomain::IdentityAxis:
        return !Delta.IdentityAxis.AxisTag.IsValid() ||
            FMath::IsNearlyZero(Delta.IdentityAxis.Amount);

    case ESFNarrativeDeltaDomain::Faction:
        return !Delta.Faction.FactionTag.IsValid() ||
            FMath::IsNearlyZero(Delta.Faction.Amount);

    case ESFNarrativeDeltaDomain::WorldFact:
        return !Delta.WorldFact.Key.FactTag.IsValid();

    default:
        break;
    }

    return false;
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
