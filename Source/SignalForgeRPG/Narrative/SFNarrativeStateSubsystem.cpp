#include "SFNarrativeStateSubsystem.h"
#include "SFNarrativeLog.h"

void USFNarrativeStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    WorldFacts.Reset();
    FactionStandingByTag.Reset();
    IdentityAxisValues.Reset();
    EndingStateByTag.Reset();
}

void USFNarrativeStateSubsystem::Deinitialize()
{
    WorldFacts.Reset();
    FactionStandingByTag.Reset();
    IdentityAxisValues.Reset();
    EndingStateByTag.Reset();

    Super::Deinitialize();
}

//
// World facts
//

bool USFNarrativeStateSubsystem::SetWorldFact(
    const FSFWorldFactKey& Key,
    const FSFWorldFactValue& NewValue,
    FSFWorldFactValue& OutOldValue)
{
    FSFWorldFactValue* Existing = WorldFacts.Find(Key);
    if (Existing)
    {
        OutOldValue = *Existing;
        *Existing = NewValue;
    }
    else
    {
        OutOldValue = FSFWorldFactValue();
        WorldFacts.Add(Key, NewValue);
    }

    return true;
}

bool USFNarrativeStateSubsystem::RemoveWorldFact(
    const FSFWorldFactKey& Key,
    FSFWorldFactValue& OutOldValue)
{
    if (FSFWorldFactValue* Existing = WorldFacts.Find(Key))
    {
        OutOldValue = *Existing;
        WorldFacts.Remove(Key);
        return true;
    }

    OutOldValue = FSFWorldFactValue();
    return false;
}

bool USFNarrativeStateSubsystem::GetWorldFact(
    const FSFWorldFactKey& Key,
    FSFWorldFactValue& OutValue) const
{
    if (const FSFWorldFactValue* Existing = WorldFacts.Find(Key))
    {
        OutValue = *Existing;
        return true;
    }

    OutValue = FSFWorldFactValue();
    return false;
}

bool USFNarrativeStateSubsystem::HasWorldFact(const FSFWorldFactKey& Key) const
{
    return WorldFacts.Contains(Key);
}

void USFNarrativeStateSubsystem::GetAllWorldFacts(TArray<FSFWorldFactSnapshot>& OutFacts) const
{
    OutFacts.Reset();
    OutFacts.Reserve(WorldFacts.Num());

    for (const TPair<FSFWorldFactKey, FSFWorldFactValue>& Pair : WorldFacts)
    {
        FSFWorldFactSnapshot Snapshot;
        Snapshot.Key = Pair.Key;
        Snapshot.Value = Pair.Value;
        OutFacts.Add(MoveTemp(Snapshot));
    }
}

//
// Factions
//

FSFFactionStandingValue USFNarrativeStateSubsystem::ApplyFactionDelta(
    const FSFFactionDelta& Delta,
    FSFFactionStandingValue& OutOldValue)
{
    FSFFactionStandingValue* Existing = FactionStandingByTag.Find(Delta.FactionTag);
    FSFFactionStandingValue Result;

    if (Existing)
    {
        OutOldValue = *Existing;
        Result = *Existing;
    }
    else
    {
        OutOldValue = FSFFactionStandingValue();
        Result = FSFFactionStandingValue();
    }

    Result.ApplyDelta(Delta);
    FactionStandingByTag.Add(Delta.FactionTag, Result);
    return Result;
}

bool USFNarrativeStateSubsystem::GetFactionStanding(
    const FGameplayTag FactionTag,
    FSFFactionStandingValue& OutValue) const
{
    if (const FSFFactionStandingValue* Existing = FactionStandingByTag.Find(FactionTag))
    {
        OutValue = *Existing;
        return true;
    }

    OutValue = FSFFactionStandingValue();
    return false;
}

ESFFactionStandingBand USFNarrativeStateSubsystem::GetFactionStandingBand(
    const FGameplayTag FactionTag) const
{
    FSFFactionStandingValue Standing;
    if (GetFactionStanding(FactionTag, Standing))
    {
        return Standing.StandingBand;
    }

    return ESFFactionStandingBand::Unknown;
}

//
// Identity axes
//

float USFNarrativeStateSubsystem::ApplyIdentityDelta(
    const FGameplayTag AxisTag,
    float Delta,
    float& OutOldValue,
    ESFIdentityDriftDirection& OutDriftDirection)
{
    float* Existing = IdentityAxisValues.Find(AxisTag);
    const float OldValue = Existing ? *Existing : 0.0f;
    const float NewValue = OldValue + Delta;

    OutOldValue = OldValue;

    IdentityAxisValues.Add(AxisTag, NewValue);

    if (FMath::IsNearlyZero(Delta))
    {
        OutDriftDirection = ESFIdentityDriftDirection::None;
    }
    else
    {
        OutDriftDirection = (Delta > 0.0f)
            ? ESFIdentityDriftDirection::Positive
            : ESFIdentityDriftDirection::Negative;
    }

    return NewValue;
}

bool USFNarrativeStateSubsystem::GetIdentityAxisValue(
    const FGameplayTag AxisTag,
    float& OutValue) const
{
    if (const float* Existing = IdentityAxisValues.Find(AxisTag))
    {
        OutValue = *Existing;
        return true;
    }

    OutValue = 0.0f;
    return false;
}

//
// Endings
//

float USFNarrativeStateSubsystem::ApplyEndingScoreDelta(
    const FGameplayTag EndingTag,
    float Delta,
    float& OutOldScore)
{
    FSFEndingState* Existing = EndingStateByTag.Find(EndingTag);
    FSFEndingState State;

    if (Existing)
    {
        State = *Existing;
    }

    OutOldScore = State.Score;
    State.Score += Delta;
    EndingStateByTag.Add(EndingTag, State);

    return State.Score;
}

bool USFNarrativeStateSubsystem::SetEndingAvailability(
    const FGameplayTag EndingTag,
    ESFEndingAvailability NewAvailability,
    ESFEndingAvailability& OutOldAvailability)
{
    FSFEndingState* Existing = EndingStateByTag.Find(EndingTag);
    FSFEndingState State;

    if (Existing)
    {
        State = *Existing;
    }

    OutOldAvailability = State.Availability;
    State.Availability = NewAvailability;

    EndingStateByTag.Add(EndingTag, State);
    return true;
}

bool USFNarrativeStateSubsystem::GetEndingState(
    const FGameplayTag EndingTag,
    FSFEndingState& OutState) const
{
    if (const FSFEndingState* Existing = EndingStateByTag.Find(EndingTag))
    {
        OutState = *Existing;
        return true;
    }

    OutState = FSFEndingState();
    return false;
}

//
// Delta application
//

bool USFNarrativeStateSubsystem::ApplyDelta(
    const FSFNarrativeDelta& Delta,
    FSFNarrativeChangeSet& OutChangeSet)
{
    switch (Delta.Type)
    {
    case ESFNarrativeDeltaType::SetWorldFact:
    {
        FSFWorldFactKey Key;
        Key.FactTag = Delta.Tag0;
        Key.ContextId = Delta.Name0;

        FSFWorldFactValue NewValue = Delta.WorldFactValue;
        FSFWorldFactValue OldValue;

        if (SetWorldFact(Key, NewValue, OldValue))
        {
            FSFWorldFactChange Change;
            Change.Key = Key;
            Change.OldValue = OldValue;
            Change.NewValue = NewValue;
            OutChangeSet.WorldFactChanges.Add(MoveTemp(Change));
        }
        break;
    }

    case ESFNarrativeDeltaType::RemoveWorldFact:
    {
        FSFWorldFactKey Key;
        Key.FactTag = Delta.Tag0;
        Key.ContextId = Delta.Name0;

        FSFWorldFactValue OldValue;
        if (RemoveWorldFact(Key, OldValue))
        {
            FSFWorldFactChange Change;
            Change.Key = Key;
            Change.OldValue = OldValue;
            Change.NewValue = FSFWorldFactValue();
            OutChangeSet.WorldFactChanges.Add(MoveTemp(Change));
        }
        break;
    }

    case ESFNarrativeDeltaType::ApplyFactionDelta:
    {
        FSFFactionDelta FactionDelta = Delta.FactionDelta;
        FSFFactionStandingValue OldValue;
        FSFFactionStandingValue NewValue = ApplyFactionDelta(FactionDelta, OldValue);

        FSFFactionChange Change;
        Change.FactionTag = FactionDelta.FactionTag;
        Change.OldStanding = OldValue;
        Change.NewStanding = NewValue;
        OutChangeSet.FactionChanges.Add(MoveTemp(Change));
        break;
    }

    case ESFNarrativeDeltaType::ApplyIdentityDelta:
    {
        float OldValue = 0.0f;
        ESFIdentityDriftDirection DriftDir = ESFIdentityDriftDirection::None;
        const float NewValue = ApplyIdentityDelta(Delta.Tag0, Delta.Float0, OldValue, DriftDir);

        FSFIdentityChange Change;
        Change.AxisTag = Delta.Tag0;
        Change.OldValue = OldValue;
        Change.NewValue = NewValue;
        Change.DriftDirection = DriftDir;
        OutChangeSet.IdentityChanges.Add(MoveTemp(Change));
        break;
    }

    case ESFNarrativeDeltaType::ApplyEndingScoreDelta:
    {
        float OldScore = 0.0f;
        const float NewScore = ApplyEndingScoreDelta(Delta.Tag0, Delta.Float0, OldScore);

        FSFEndingChange Change;
        Change.EndingTag = Delta.Tag0;
        Change.OldState.Score = OldScore;
        Change.NewState.Score = NewScore;
        // Availability is left unchanged here; may be set by follow-up logic.
        OutChangeSet.EndingChanges.Add(MoveTemp(Change));
        break;
    }

    case ESFNarrativeDeltaType::SetEndingAvailability:
    {
        FSFEndingState OldState;
        GetEndingState(Delta.Tag0, OldState);

        ESFEndingAvailability OldAvailability = OldState.Availability;
        ESFEndingAvailability NewAvailability = static_cast<ESFEndingAvailability>(Delta.Int0);

        SetEndingAvailability(Delta.Tag0, NewAvailability, OldAvailability);

        FSFEndingState NewState;
        GetEndingState(Delta.Tag0, NewState);

        FSFEndingChange Change;
        Change.EndingTag = Delta.Tag0;
        Change.OldState = OldState;
        Change.NewState = NewState;
        OutChangeSet.EndingChanges.Add(MoveTemp(Change));
        break;
    }

    default:
        NARR_LOG_WARNING(TEXT("USFNarrativeStateSubsystem::ApplyDelta – unsupported delta type %d"), static_cast<int32>(Delta.Type));
        return false;
    }

    return true;
}

//
// Save/load
//

FSFNarrativeSaveData USFNarrativeStateSubsystem::BuildSaveData() const
{
    FSFNarrativeSaveData SaveData;

    // World facts.
    for (const TPair<FSFWorldFactKey, FSFWorldFactValue>& Pair : WorldFacts)
    {
        FSFWorldFactSnapshot Snapshot;
        Snapshot.Key = Pair.Key;
        Snapshot.Value = Pair.Value;
        SaveData.WorldFacts.Add(MoveTemp(Snapshot));
    }

    // Factions.
    for (const TPair<FGameplayTag, FSFFactionStandingValue>& Pair : FactionStandingByTag)
    {
        FSFFactionStandingSnapshot Snapshot;
        Snapshot.FactionTag = Pair.Key;
        Snapshot.Standing = Pair.Value;
        SaveData.FactionSnapshots.Add(MoveTemp(Snapshot));
    }

    // Identity.
    for (const TPair<FGameplayTag, float>& Pair : IdentityAxisValues)
    {
        FSFIdentityAxisSnapshot Snapshot;
        Snapshot.AxisTag = Pair.Key;
        Snapshot.Value = Pair.Value;
        SaveData.IdentitySnapshots.Add(MoveTemp(Snapshot));
    }

    // Endings.
    for (const TPair<FGameplayTag, FSFEndingState>& Pair : EndingStateByTag)
    {
        FSFEndingSnapshot Snapshot;
        Snapshot.EndingTag = Pair.Key;
        Snapshot.State = Pair.Value;
        SaveData.EndingSnapshots.Add(MoveTemp(Snapshot));
    }

    return SaveData;
}

bool USFNarrativeStateSubsystem::LoadFromSaveData(const FSFNarrativeSaveData& SaveData)
{
    WorldFacts.Reset();
    FactionStandingByTag.Reset();
    IdentityAxisValues.Reset();
    EndingStateByTag.Reset();

    for (const FSFWorldFactSnapshot& Snapshot : SaveData.WorldFacts)
    {
        WorldFacts.Add(Snapshot.Key, Snapshot.Value);
    }

    for (const FSFFactionStandingSnapshot& Snapshot : SaveData.FactionSnapshots)
    {
        FactionStandingByTag.Add(Snapshot.FactionTag, Snapshot.Standing);
    }

    for (const FSFIdentityAxisSnapshot& Snapshot : SaveData.IdentitySnapshots)
    {
        IdentityAxisValues.Add(Snapshot.AxisTag, Snapshot.Value);
    }

    for (const FSFEndingSnapshot& Snapshot : SaveData.EndingSnapshots)
    {
        EndingStateByTag.Add(Snapshot.EndingTag, Snapshot.State);
    }

    return true;
}