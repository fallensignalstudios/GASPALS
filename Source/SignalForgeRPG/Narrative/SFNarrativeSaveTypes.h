// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SFNarrativeTypes.h"

/**
 * Save-time-only helpers and convenience functions for FSFNarrativeSaveData.
 *
 * The canonical save-data structs (FSFWorldFactSnapshot, FSFQuestSnapshot,
 * FSFTaskProgressKey, FSFTaskCounterSnapshot, FSFFactionSnapshot,
 * FSFIdentityAxisValue, FSFOutcomeApplication, FSFEndingState,
 * FSFNarrativeOwnerSnapshot, and FSFNarrativeSaveData itself) all live in
 * SFNarrativeTypes.h so that UHT can reflect them without circular include
 * gymnastics.
 *
 * This header exists for free helper functions that operate on those types.
 */
namespace SFNarrativeSave
{
    /** True if there is nothing to serialize. */
    SIGNALFORGERPG_API bool IsEmpty(const FSFNarrativeSaveData& InData);

    /** Reset all arrays and the schema version. */
    SIGNALFORGERPG_API void Reset(FSFNarrativeSaveData& InOutData);
}
