#include "SFNarrativeStateQueryLibrary.h"
#include "SFNarrativeFactSubsystem.h"
#include "SFNarrativeStateSubsystem.h"
#include "SFQuestConditionLibrary.h"
#include "SFQuestInstance.h"
#include "SFQuestDefinition.h"

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

    // No facts = treat as failure if there are any fact conditions.
    for (const FSFNarrativeCondition& Cond : ConditionSet.AllConditions)
    {
        if (Cond.Domain == ESFNarrativeConditionDomain::WorldFact)
        {
            return false;
        }
    }
    for (const FSFNarrativeCondition& Cond : ConditionSet.AnyConditions)
    {
        if (Cond.Domain == ESFNarrativeConditionDomain::WorldFact)
        {
            return false;
        }
    }
    for (const FSFNarrativeCondition& Cond : ConditionSet.NoneConditions)
    {
        if (Cond.Domain == ESFNarrativeConditionDomain::WorldFact)
        {
            return false;
        }
    }

    // No world-fact conditions at all; trivially true.
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
        // If there are no quest-state conditions at all, trivially true.
        bool bHasQuestConditions = false;
        for (const FSFNarrativeCondition& C : ConditionSet.AllConditions)
        {
            if (C.Domain == ESFNarrativeConditionDomain::QuestState) { bHasQuestConditions = true; break; }
        }
        if (!bHasQuestConditions)
        {
            for (const FSFNarrativeCondition& C : ConditionSet.AnyConditions)
            {
                if (C.Domain == ESFNarrativeConditionDomain::QuestState) { bHasQuestConditions = true; break; }
            }
        }
        if (!bHasQuestConditions)
        {
            for (const FSFNarrativeCondition& C : ConditionSet.NoneConditions)
            {
                if (C.Domain == ESFNarrativeConditionDomain::QuestState) { bHasQuestConditions = true; break; }
            }
        }

        return !bHasQuestConditions;
    }

    const FSFQuestSnapshot Snapshot = QuestInstance->BuildSnapshot();
    return USFQuestConditionLibrary::EvaluateQuestStateConditionsInSet(
        ConditionSet,
        QuestDef,
        Snapshot);
}

bool USFNarrativeStateQueryLibrary::EvaluateConditionSet(
    const UObject* WorldContextObject,
    const FSFNarrativeConditionSet& ConditionSet,
    const USFQuestDefinition* QuestDef,
    const USFQuestInstance* QuestInstance)
{
    //
    // 1) World facts
    //
    if (!EvaluateWorldFactConditionsInSet(WorldContextObject, ConditionSet))
    {
        return false;
    }

    //
    // 2) Quest state
    //
    if (!EvaluateQuestStateConditionsForInstance(ConditionSet, QuestDef, QuestInstance))
    {
        return false;
    }

    //
    // 3) Faction / Identity / Ending conditions
    //
    // These are represented inside FSFNarrativeCondition but evaluated locally.
    if (USFNarrativeStateSubsystem* State = GetStateSubsystem(WorldContextObject))
    {
        // AllConditions
        for (const FSFNarrativeCondition& C : ConditionSet.AllConditions)
        {
            bool bRelevant = false;
            bool bPass = true;

            switch (C.Domain)
            {
            case ESFNarrativeConditionDomain::Faction:
                bRelevant = true;
                {
                    const FSFNarrativeFactionCondition& F = C.Faction;
                    FSFFactionStandingValue Standing;
                    if (!State->GetFactionStanding(F.FactionTag, Standing))
                    {
                        bPass = false;
                    }
                    else
                    {
                        // Band requirement
                        if (F.RequiredBand != ESFFactionStandingBand::Unknown &&
                            Standing.StandingBand != F.RequiredBand)
                        {
                            bPass = false;
                        }

                        // Aggregate numeric condition
                        if (bPass && F.AggregateCondition.Op != ESFNarrativeNumericCompareOp::Equal) // heuristic: treat default as "unused"
                        {
                            FSFNarrativeNumericCondition Cond = F.AggregateCondition;
                            float Actual = Standing.AggregateScore;
                            const float A = Cond.ThresholdA;
                            const float B = Cond.ThresholdB;
                            const float Eps = Cond.Epsilon;

                            switch (Cond.Op)
                            {
                            case ESFNarrativeNumericCompareOp::Equal:            bPass = FMath::Abs(Actual - A) <= Eps; break;
                            case ESFNarrativeNumericCompareOp::NotEqual:         bPass = FMath::Abs(Actual - A) > Eps; break;
                            case ESFNarrativeNumericCompareOp::Greater:          bPass = Actual > A; break;
                            case ESFNarrativeNumericCompareOp::GreaterOrEqual:   bPass = Actual >= A; break;
                            case ESFNarrativeNumericCompareOp::Less:             bPass = Actual < A; break;
                            case ESFNarrativeNumericCompareOp::LessOrEqual:      bPass = Actual <= A; break;
                            case ESFNarrativeNumericCompareOp::BetweenInclusive: bPass = Actual >= FMath::Min(A, B) && Actual <= FMath::Max(A, B); break;
                            case ESFNarrativeNumericCompareOp::OutsideExclusive: bPass = Actual < FMath::Min(A, B) || Actual > FMath::Max(A, B); break;
                            default: break;
                            }
                        }
                    }
                }
                break;

            case ESFNarrativeConditionDomain::IdentityAxis:
                bRelevant = true;
                {
                    const FSFNarrativeIdentityAxisCondition& I = C.IdentityAxis;
                    float Actual = 0.0f;
                    if (!State->GetIdentityAxisValue(I.AxisTag, Actual))
                    {
                        bPass = false;
                    }
                    else
                    {
                        const FSFNarrativeNumericCondition& Cond = I.ValueCondition;
                        const float A = Cond.ThresholdA;
                        const float B = Cond.ThresholdB;
                        const float Eps = Cond.Epsilon;

                        switch (Cond.Op)
                        {
                        case ESFNarrativeNumericCompareOp::Equal:            bPass = FMath::Abs(Actual - A) <= Eps; break;
                        case ESFNarrativeNumericCompareOp::NotEqual:         bPass = FMath::Abs(Actual - A) > Eps; break;
                        case ESFNarrativeNumericCompareOp::Greater:          bPass = Actual > A; break;
                        case ESFNarrativeNumericCompareOp::GreaterOrEqual:   bPass = Actual >= A; break;
                        case ESFNarrativeNumericCompareOp::Less:             bPass = Actual < A; break;
                        case ESFNarrativeNumericCompareOp::LessOrEqual:      bPass = Actual <= A; break;
                        case ESFNarrativeNumericCompareOp::BetweenInclusive: bPass = Actual >= FMath::Min(A, B) && Actual <= FMath::Max(A, B); break;
                        case ESFNarrativeNumericCompareOp::OutsideExclusive: bPass = Actual < FMath::Min(A, B) || Actual > FMath::Max(A, B); break;
                        default: break;
                        }
                    }
                }
                break;

            case ESFNarrativeConditionDomain::Time:
                // Time conditions are game-specific; you can wire this to a clock/chapter system later.
                // For now we treat them as "not evaluated" (bRelevant = false).
                bRelevant = false;
                break;

            case ESFNarrativeConditionDomain::Custom:
                // Custom domain reserved for your own evaluators.
                bRelevant = false;
                break;

            default:
                break;
            }

            if (bRelevant)
            {
                if (C.bNegate ? bPass : !bPass)
                {
                    return false;
