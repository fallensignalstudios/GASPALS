// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "SFNarrativeSaveGame.h"

USFNarrativeSaveGame::USFNarrativeSaveGame()
{
    SaveTimestamp = FDateTime::UtcNow();
}

void USFNarrativeSaveGame::ResetNarrative()
{
    WorldState.Reset();

    // Reset the canonical FSFNarrativeSaveData payloads.
    NarrativeData.WorldFacts.Reset();
    NarrativeData.FactionSnapshots.Reset();
    NarrativeData.IdentityAxes.Reset();
    NarrativeData.EndingStates.Reset();
    NarrativeData.QuestSnapshots.Reset();
    NarrativeData.LifetimeTaskCounters.Reset();
    NarrativeData.AppliedOutcomes.Reset();
    NarrativeData.CustomOwnerSnapshots.Reset();
}

bool USFNarrativeSaveGame::HasNarrativeContent() const
{
    if (!WorldState.IsEmpty())
    {
        return true;
    }

    if (NarrativeData.WorldFacts.Num() > 0)             return true;
    if (NarrativeData.FactionSnapshots.Num() > 0)       return true;
    if (NarrativeData.IdentityAxes.Num() > 0)           return true;
    if (NarrativeData.EndingStates.Num() > 0)           return true;
    if (NarrativeData.QuestSnapshots.Num() > 0)         return true;
    if (NarrativeData.LifetimeTaskCounters.Num() > 0)   return true;
    if (NarrativeData.AppliedOutcomes.Num() > 0)        return true;
    if (NarrativeData.CustomOwnerSnapshots.Num() > 0)   return true;

    return false;
}
