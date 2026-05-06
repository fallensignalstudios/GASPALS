// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "SFNarrativeSaveTypes.h"
#include "SFNarrativeTypes.h"

namespace SFNarrativeSave
{
    bool IsEmpty(const FSFNarrativeSaveData& InData)
    {
        return InData.QuestSnapshots.Num() == 0
            && InData.LifetimeTaskCounters.Num() == 0
            && InData.WorldFacts.Num() == 0
            && InData.FactionSnapshots.Num() == 0
            && InData.IdentityAxes.Num() == 0
            && InData.AppliedOutcomes.Num() == 0
            && InData.EndingStates.Num() == 0
            && InData.CustomOwnerSnapshots.Num() == 0;
    }

    void Reset(FSFNarrativeSaveData& InOutData)
    {
        InOutData.SchemaVersion = 2;
        InOutData.QuestSnapshots.Reset();
        InOutData.LifetimeTaskCounters.Reset();
        InOutData.WorldFacts.Reset();
        InOutData.FactionSnapshots.Reset();
        InOutData.IdentityAxes.Reset();
        InOutData.AppliedOutcomes.Reset();
        InOutData.EndingStates.Reset();
        InOutData.CustomOwnerSnapshots.Reset();
    }
}
