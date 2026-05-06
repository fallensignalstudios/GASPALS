// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "SFNarrativeStateQueryLibrary.h"

#include "SFNarrativeFactSubsystem.h"
#include "SFNarrativeStateSubsystem.h"
#include "SFQuestConditionLibrary.h"
#include "SFQuestDefinition.h"
#include "SFQuestInstance.h"

namespace
{
    static bool IsDefaultNumericCondition(const FSFNarrativeNumericCondition& Condition)
    {
        return Condition.Op == ESFNarrativeNumericCompareOp::GreaterOrEqual
            && FMath::IsNearlyZero(Condition.ThresholdA)
            && FMath::IsNearlyZero(Condition.ThresholdB)
            && FMath::IsNearlyEqual(Condition.Epsilon, KINDA_SMALL_NUMBER);
    }

    static bool EvaluateNumericCondition(float Actual, const FSFNarrativeNumericCondition& Condition)
    {
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

    static bool EvaluateFactionCondition(
        const USFNarrativeStateSubsystem* State,
        const FSFNarrativeFactionCondition& Condition)
    {
        if (!State)
        {
            return false;
        }

        FSFFactionStandingValue Standing;
        if (!State->GetFactionStanding(Condition.FactionTag, Standing))
        {
            return false;
        }

        if (Condition.RequiredBand != ESFFactionStandingBand::Unknown
            && Standing.StandingBand != Condition.RequiredBand)
        {
            return false;
        }

        if (!IsDefaultNumericCondition(Condition.AggregateCondition))
        {
            if (EvaluateNumericCondition(Standing.Trust, Condition.AggregateCondition))
            {
                return true;
            }

            if (Condition.bPassIfAnyMetricHigh)
            {
                return EvaluateNumericCondition(Standing.Fear, Condition.AggregateCondition)
                    || EvaluateNumericCondition(Standing.Respect, Condition.AggregateCondition)
                    || EvaluateNumericCondition(Standing.Dependency, Condition.AggregateCondition)
                    || EvaluateNumericCondition(Standing.Alignment, Condition.AggregateCondition)
                    || EvaluateNumericCondition(Standing.Betrayal, Condition.AggregateCondition);
            }

            return false;
        }

        return true;
    }

    static bool EvaluateIdentityCondition(
        const USFNarrativeStateSubsystem* State,
        const FSFNarrativeIdentityAxisCondition& Condition)
    {
        if (!State)
        {
            return false;
        }

        float ActualValue = 0.0f;
        if (!State->GetIdentityAxisValue(Condition.AxisTag, ActualValue))
        {
            return false;
        }

        if (IsDefaultNumericCondition(Condition.ValueCondition))
        {
            return true;
        }

        return EvaluateNumericCondition(ActualValue, Condition.ValueCondition);
    }

    static bool TryEvaluateLocalCondition(
        const USFNarrativeStateSubsystem* State,
        const FSFNarrativeCondition& Condition,
        bool& bOutRelevant,
        bool& bOutPass)
    {
        bOutRelevant = true;
        bOutPass = false;

        switch (Condition.Domain)
        {
        case ESFNarrativeConditionDomain::Faction:
            bOutPass = EvaluateFactionCondition(State, Condition.Faction);
            return true;

        case ESFNarrativeConditionDomain::IdentityAxis:
            bOutPass = EvaluateIdentityCondition(State, Condition.IdentityAxis);
            return true;

        case ESFNarrativeConditionDomain::WorldFact:
        case ESFNarrativeConditionDomain::QuestState:
        case ESFNarrativeConditionDomain::TagSet:
        case ESFNarrativeConditionDomain::Dialogue:
        case ESFNarrativeConditionDomain::Time:
        case ESFNarrativeConditionDomain::Custom:
        default:
            bOutRelevant = false;
            bOutPass = true;
            return false;
        }
    }

    static bool HasLocalStateConditions(const FSFNarrativeConditionSet& ConditionSet)
    {
        auto HasRelevantCondition = [](const TArray<FSFNarrativeCondition>& Conditions)
        {
            for (const FSFNarrativeCondition& Condition : Conditions)
            {
                if (Condition.Domain == ESFNarrativeConditionDomain::Faction
                    || Condition.Domain == ESFNarrativeConditionDomain::IdentityAxis)
                {
                    return true;
                }
            }

            return false;
        };

        return HasRelevantCondition(ConditionSet.AllConditions)
            || HasRelevantCondition(ConditionSet.AnyConditions)
            || HasRelevantCondition(ConditionSet.NoneConditions);
    }
}

USFNarrativeFactSubsystem* USFNarrativeStateQueryLibrary::GetFactSubsystem(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    UWorld* World = WorldContextObject->GetWorld();
    return World ? World->GetSubsystem<USFNarrativeFactSubsystem>() : nullptr;
}

USFNarrativeStateSubsystem* USFNarrativeStateQueryLibrary::GetStateSubsystem(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    UWorld* World = WorldContextObject->GetWorld();
    return World ? World->GetSubsystem<USFNarrativeStateSubsystem>() : nullptr;
}

bool USFNarrativeStateQueryLibrary::IsBoolFactEqual(
    const UObject* WorldContextObject,
    const FSFWorldFactKey& Key,
    bool bExpected)
{
    if (USFNarrativeFactSubsystem* Facts = GetFactSubsystem(WorldContextObject))
    {
        return Facts->IsBoolFactEqual(Key, bExpected);
    }

    return false;
}

bool USFNarrativeStateQueryLibrary::DoesNumericFactPassCondition(
    const UObject* WorldContextObject,
    const FSFWorldFactKey& Key,
    const FSFNarrativeNumericCondition& Condition)
{
    if (USFNarrativeFactSubsystem* Facts = GetFactSubsystem(WorldContextObject))
    {
        return Facts->DoesNumericFactPassCondition(Key, Condition);
    }

    return false;
}

bool USFNarrativeStateQueryLibrary::EvaluateWorldFactConditionsInSet(
    const UObject* WorldContextObject,
    const FSFNarrativeConditionSet& ConditionSet)
{
    if (USFNarrativeFactSubsystem* Facts = GetFactSubsystem(WorldContextObject))
    {
        return Facts->EvaluateWorldFactConditionsInSet(ConditionSet);
    }

    for (const FSFNarrativeCondition& Condition : ConditionSet.AllConditions)
    {
        if (Condition.Domain == ESFNarrativeConditionDomain::WorldFact)
        {
            return false;
        }
    }

    for (const FSFNarrativeCondition& Condition : ConditionSet.AnyConditions)
    {
        if (Condition.Domain == ESFNarrativeConditionDomain::WorldFact)
        {
            return false;
        }
    }

    for (const FSFNarrativeCondition& Condition : ConditionSet.NoneConditions)
    {
        if (Condition.Domain == ESFNarrativeConditionDomain::WorldFact)
        {
            return false;
        }
    }

    return true;
}

ESFFactionStandingBand USFNarrativeStateQueryLibrary::GetFactionStandingBand(
    const UObject* WorldContextObject,
    FGameplayTag FactionTag)
{
    if (USFNarrativeStateSubsystem* State = GetStateSubsystem(WorldContextObject))
    {
        return State->GetFactionStandingBand(FactionTag);
    }

    return ESFFactionStandingBand::Unknown;
}

bool USFNarrativeStateQueryLibrary::IsFactionStandingAtLeast(
    const UObject* WorldContextObject,
    FGameplayTag FactionTag,
    ESFFactionStandingBand RequiredBand)
{
    const ESFFactionStandingBand Current = GetFactionStandingBand(WorldContextObject, FactionTag);
    return static_cast<uint8>(Current) >= static_cast<uint8>(RequiredBand);
}

float USFNarrativeStateQueryLibrary::GetIdentityAxisValue(
    const UObject* WorldContextObject,
    FGameplayTag AxisTag,
    bool& bOutHasValue)
{
    bOutHasValue = false;

    if (USFNarrativeStateSubsystem* State = GetStateSubsystem(WorldContextObject))
    {
        float Value = 0.0f;
        if (State->GetIdentityAxisValue(AxisTag, Value))
        {
            bOutHasValue = true;
            return Value;
        }
    }

    return 0.0f;
}

bool USFNarrativeStateQueryLibrary::GetEndingState(
    const UObject* WorldContextObject,
    FGameplayTag EndingTag,
    FSFEndingState& OutState)
{
    if (USFNarrativeStateSubsystem* State = GetStateSubsystem(WorldContextObject))
    {
        return State->GetEndingState(EndingTag, OutState);
    }

    OutState = FSFEndingState();
    return false;
}

bool USFNarrativeStateQueryLibrary::IsEndingAvailable(
    const UObject* WorldContextObject,
    FGameplayTag EndingTag,
    ESFEndingAvailability MinimumAvailability)
{
    FSFEndingState State;
    if (!GetEndingState(WorldContextObject, EndingTag, State))
    {
        return false;
    }

    return static_cast<uint8>(State.Availability) >= static_cast<uint8>(MinimumAvailability);
}

bool USFNarrativeStateQueryLibrary::EvaluateQuestStateConditionsForInstance(
    const FSFNarrativeConditionSet& ConditionSet,
    const USFQuestDefinition* QuestDef,
    const USFQuestInstance* QuestInstance)
{
    if (!QuestDef || !QuestInstance)
    {
        auto HasQuestConditions = [](const TArray<FSFNarrativeCondition>& Conditions)
        {
            for (const FSFNarrativeCondition& Condition : Conditions)
            {
                if (Condition.Domain == ESFNarrativeConditionDomain::QuestState)
                {
                    return true;
                }
            }

            return false;
        };

        return !(HasQuestConditions(ConditionSet.AllConditions)
            || HasQuestConditions(ConditionSet.AnyConditions)
            || HasQuestConditions(ConditionSet.NoneConditions));
    }

    return USFQuestConditionLibrary::EvaluateQuestStateConditionsInSet(
        ConditionSet,
        QuestDef,
        QuestInstance->BuildSnapshot());
}

bool USFNarrativeStateQueryLibrary::EvaluateConditionSet(
    const UObject* WorldContextObject,
    const FSFNarrativeConditionSet& ConditionSet,
    const USFQuestDefinition* QuestDef,
    const USFQuestInstance* QuestInstance)
{
    if (!EvaluateWorldFactConditionsInSet(WorldContextObject, ConditionSet))
    {
        return false;
    }

    if (!EvaluateQuestStateConditionsForInstance(ConditionSet, QuestDef, QuestInstance))
    {
        return false;
    }

    USFNarrativeStateSubsystem* State = GetStateSubsystem(WorldContextObject);
    if (!State && HasLocalStateConditions(ConditionSet))
    {
        return false;
    }

    for (const FSFNarrativeCondition& Condition : ConditionSet.AllConditions)
    {
        bool bRelevant = false;
        bool bPass = true;
        TryEvaluateLocalCondition(State, Condition, bRelevant, bPass);

        if (bRelevant && (Condition.bNegate ? bPass : !bPass))
        {
            return false;
        }
    }

    bool bHasRelevantAnyConditions = false;
    bool bAnyPassed = false;
    for (const FSFNarrativeCondition& Condition : ConditionSet.AnyConditions)
    {
        bool bRelevant = false;
        bool bPass = true;
        TryEvaluateLocalCondition(State, Condition, bRelevant, bPass);

        if (!bRelevant)
        {
            continue;
        }

        bHasRelevantAnyConditions = true;
        if (Condition.bNegate ? !bPass : bPass)
        {
            bAnyPassed = true;
            break;
        }
    }

    if (bHasRelevantAnyConditions && !bAnyPassed)
    {
        return false;
    }

    for (const FSFNarrativeCondition& Condition : ConditionSet.NoneConditions)
    {
        bool bRelevant = false;
        bool bPass = true;
        TryEvaluateLocalCondition(State, Condition, bRelevant, bPass);

        if (bRelevant && (Condition.bNegate ? !bPass : bPass))
        {
            return false;
        }
    }

    return true;
}
