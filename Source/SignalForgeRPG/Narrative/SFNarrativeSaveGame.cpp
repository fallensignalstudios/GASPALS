#include "SFNarrativeSaveGame.h"

USFNarrativeSaveGame::USFNarrativeSaveGame()
{
    SaveTimestamp = FDateTime::UtcNow();
}

void USFNarrativeSaveGame::ResetNarrative()
{
    WorldState.Reset();

    // FSFNarrativeSaveData is assumed to have simple arrays/maps you can Reset
    NarrativeData.WorldFacts.Reset();
    NarrativeData.FactionSnapshots.Reset();
    NarrativeData.IdentitySnapshots.Reset();
    NarrativeData.EndingSnapshots.Reset();
    NarrativeData.QuestSnapshots.Reset();
    NarrativeData.LifetimeTaskCounters.Reset();
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
    if (NarrativeData.IdentitySnapshots.Num() > 0)      return true;
    if (NarrativeData.EndingSnapshots.Num() > 0)        return true;
    if (NarrativeData.QuestSnapshots.Num() > 0)         return true;
    if (NarrativeData.LifetimeTaskCounters.Num() > 0)   return true;
    if (NarrativeData.CustomOwnerSnapshots.Num() > 0)   return true;

    return false;
}