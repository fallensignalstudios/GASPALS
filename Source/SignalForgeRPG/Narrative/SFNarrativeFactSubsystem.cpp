// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "SFNarrativeFactSubsystem.h"
#include "SFNarrativeStateSubsystem.h"

void USFNarrativeFactSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (UWorld* World = GetWorld())
    {
        CachedStateSubsystem = World->GetSubsystem<USFNarrativeStateSubsystem>();
    }
}

void USFNarrativeFactSubsystem::Deinitialize()
{
    CachedStateSubsystem = nullptr;
    Super::Deinitialize();
}

USFNarrativeStateSubsystem* USFNarrativeFactSubsystem::GetStateSubsystem() const
{
    return CachedStateSubsystem;
}

//
// Direct fact access
//

bool USFNarrativeFactSubsystem::GetFact(const FSFWorldFactKey& Key, FSFWorldFactValue& OutValue) const
{
    if (!CachedStateSubsystem)
    {
        OutValue = FSFWorldFactValue();
        return false;
    }

    return CachedStateSubsystem->GetWorldFact(Key, OutValue);
}

bool USFNarrativeFactSubsystem::HasFact(const FSFWorldFactKey& Key) const
{
    if (!CachedStateSubsystem)
    {
        return false;
    }

    return CachedStateSubsystem->HasWorldFact(Key);
}

bool USFNarrativeFactSubsystem::HasAnyFactWithTag(FGameplayTag FactTag) const
{
    if (!CachedStateSubsystem || !FactTag.IsValid())
    {
        return false;
    }

    TArray<FSFWorldFactSnapshot> AllFacts;
    CachedStateSubsystem->GetAllWorldFacts(AllFacts);

    for (const FSFWorldFactSnapshot& Snapshot : AllFacts)
    {
        if (Snapshot.Key.FactTag == FactTag)
        {
            return true;
        }
    }

    return false;
}

void USFNarrativeFactSubsystem::GetFactsByTag(
    FGameplayTag FactTag,
    TArray<FSFWorldFactSnapshot>& OutFacts) const
{
    OutFacts.Reset();

    if (!CachedStateSubsystem || !FactTag.IsValid())
    {
        return;
    }

    TArray<FSFWorldFactSnapshot> AllFacts;
    CachedStateSubsystem->GetAllWorldFacts(AllFacts);

    for (const FSFWorldFactSnapshot& Snapshot : AllFacts)
    {
        if (Snapshot.Key.FactTag == FactTag)
        {
            OutFacts.Add(Snapshot);
        }
    }
}

void USFNarrativeFactSubsystem::GetFactsByContext(
    FName ContextId,
    TArray<FSFWorldFactSnapshot>& OutFacts) const
{
    OutFacts.Reset();

    if (!CachedStateSubsystem || ContextId.IsNone())
    {
        return;
    }

    TArray<FSFWorldFactSnapshot> AllFacts;
    CachedStateSubsystem->GetAllWorldFacts(AllFacts);

    for (const FSFWorldFactSnapshot& Snapshot : AllFacts)
    {
        if (Snapshot.Key.ContextId == ContextId)
        {
            OutFacts.Add(Snapshot);
        }
    }
}

void USFNarrativeFactSubsystem::GetAllFacts(TArray<FSFWorldFactSnapshot>& OutFacts) const
{
    OutFacts.Reset();

    if (!CachedStateSubsystem)
    {
        return;
    }

    CachedStateSubsystem->GetAllWorldFacts(OutFacts);
}

//
// Typed convenience queries
//

bool USFNarrativeFactSubsystem::IsBoolFactEqual(
    const FSFWorldFactKey& Key,
    bool bExpected) const
{
    FSFWorldFactValue Value;
    if (!GetFact(Key, Value) || Value.ValueType != ESFNarrativeFactValueType::Bool)
    {
        return false;
    }

    return Value.BoolValue == bExpected;
}

bool USFNarrativeFactSubsystem::IsNameFactEqual(
    const FSFWorldFactKey& Key,
    FName ExpectedName) const
{
    FSFWorldFactValue Value;
    if (!GetFact(Key, Value) || Value.ValueType != ESFNarrativeFactValueType::Name)
    {
        return false;
    }

    return Value.NameValue == ExpectedName;
}

bool USFNarrativeFactSubsystem::DoesTagFactContain(
    const FSFWorldFactKey& Key,
    FGameplayTag RequiredTag) const
{
    FSFWorldFactValue Value;
    if (!GetFact(Key, Value) || Value.ValueType != ESFNarrativeFactValueType::Tag)
    {
        return false;
    }

    return Value.TagValue == RequiredTag;
}

bool USFNarrativeFactSubsystem::DoesNumericFactPassCondition(
    const FSFWorldFactKey& Key,
    const FSFNarrativeNumericCondition& Condition) const
{
    FSFWorldFactValue Value;
    if (!GetFact(Key, Value))
    {
        return false;
    }

    float Actual = 0.0f;

    switch (Value.ValueType)
    {
    case ESFNarrativeFactValueType::Int:
        Actual = static_cast<float>(Value.IntValue);
        break;
    case ESFNarrativeFactValueType::Float:
        Actual = Value.FloatValue;
        break;
    default:
        return false;
    }

    const float A = Condition.ThresholdA;
    const float B = Condition.ThresholdB;
    const float Eps = Condition.Epsilon;

    switch (Condition.Op)
    {
    case ESFNarrativeNumericCompareOp::Equal:
        return FMath::Abs(Actual - A) <= Eps;
    case ESFNarrativeNumericCompareOp::NotEqual:
        return FMath::Abs(Actual - A) > Eps;
    case ESFNarrativeNumericCompareOp::Greater:
        return Actual > A;
    case ESFNarrativeNumericCompareOp::GreaterOrEqual:
        return Actual >= A;
    case ESFNarrativeNumericCompareOp::Less:
        return Actual < A;
    case ESFNarrativeNumericCompareOp::LessOrEqual:
        return Actual <= A;
    case ESFNarrativeNumericCompareOp::BetweenInclusive:
        return Actual >= FMath::Min(A, B) && Actual <= FMath::Max(A, B);
    case ESFNarrativeNumericCompareOp::OutsideExclusive:
        return Actual < FMath::Min(A, B) || Actual > FMath::Max(A, B);
    default:
        return false;
    }
}

//
// Condition?set evaluation (WorldFact domain)
//

bool USFNarrativeFactSubsystem::EvaluateWorldFactCondition(
    const FSFNarrativeWorldFactCondition& Condition) const
{
    // Existence + type gate first.
    FSFWorldFactValue Value;
    const bool bHasFact = GetFact(Condition.Key, Value);

    if (!bHasFact)
    {
        // If the condition only cares about existence, fail when missing.
        if (Condition.ExpectedValueType == ESFNarrativeFactValueType::None)
        {
            return false;
        }
        return false;
    }

    if (Condition.ExpectedValueType != ESFNarrativeFactValueType::None &&
        Value.ValueType != Condition.ExpectedValueType)
    {
        return false;
    }

    switch (Condition.ExpectedValueType)
    {
    case ESFNarrativeFactValueType::None:
        // Just existence.
        return true;

    case ESFNarrativeFactValueType::Bool:
        return Value.BoolValue == Condition.bExpectedBool;

    case ESFNarrativeFactValueType::Int:
    case ESFNarrativeFactValueType::Float:
        return DoesNumericFactPassCondition(Condition.Key, Condition.NumericCondition);

    case ESFNarrativeFactValueType::Name:
        if (Condition.ExpectedName.IsNone())
        {
            return !Value.NameValue.IsNone();
        }
        return Value.NameValue == Condition.ExpectedName;

    case ESFNarrativeFactValueType::Tag:
        if (!Condition.ExpectedTag.IsValid())
        {
            return Value.TagValue.IsValid();
        }
        return Value.TagValue == Condition.ExpectedTag;

    default:
        break;
    }

    return false;
}

bool USFNarrativeFactSubsystem::EvaluateWorldFactConditionsInSet(
    const FSFNarrativeConditionSet& ConditionSet) const
{
    // AllConditions: all must pass.
    for (const FSFNarrativeCondition& Condition : ConditionSet.AllConditions)
    {
        if (Condition.Domain != ESFNarrativeConditionDomain::WorldFact)
        {
            continue;
        }

        const bool bPass = EvaluateWorldFactCondition(Condition.WorldFact);
        if (Condition.bNegate ? bPass : !bPass)
        {
            return false;
        }
    }

    // AnyConditions: at least one must pass if any exist.
    bool bHasAny = false;
    bool bAnyPassed = false;

    for (const FSFNarrativeCondition& Condition : ConditionSet.AnyConditions)
    {
        if (Condition.Domain != ESFNarrativeConditionDomain::WorldFact)
        {
            continue;
        }

        bHasAny = true;
        const bool bPass = EvaluateWorldFactCondition(Condition.WorldFact);

        if (Condition.bNegate ? !bPass : bPass)
        {
            bAnyPassed = true;
            break;
        }
    }

    if (bHasAny && !bAnyPassed)
    {
        return false;
    }

    // NoneConditions: all must fail.
    for (const FSFNarrativeCondition& Condition : ConditionSet.NoneConditions)
    {
        if (Condition.Domain != ESFNarrativeConditionDomain::WorldFact)
        {
            continue;
        }

        const bool bPass = EvaluateWorldFactCondition(Condition.WorldFact);
        if (Condition.bNegate ? !bPass : bPass)
        {
            return false;
        }
    }

    return true;
}
