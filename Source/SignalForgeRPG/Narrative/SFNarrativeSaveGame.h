// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeWorldState.h"
#include "SFNarrativeSaveGame.generated.h"

/**
 * Top-level save game object for narrative state.
 *
 * This is a durable container for:
 *  - Global world narrative (facts, factions, identity, endings, flags, vars)
 *  - Per-owner narrative data (quests, lifetime task counts, dialogue, etc.)
 *  - High-level metadata (slot name, version, timestamps)
 *
 * All heavy lifting (building and applying FSFNarrativeSaveData) is done
 * by USFNarrativeSaveService and the various runtimes/subsystems.
 */
UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFNarrativeSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    USFNarrativeSaveGame();

    /** Arbitrary human-readable label for this save (e.g. location or chapter). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narrative|Save")
    FString FriendlyName;

    /** Slot name this save was last written to (for convenience). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narrative|Save")
    FString SlotName;

    /** User index this save is associated with. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narrative|Save")
    int32 UserIndex = 0;

    /** Narrative system version used to create this save (for migrations). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narrative|Save")
    int32 NarrativeVersion = 1;

    /** Engine time when the save was created. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narrative|Save")
    FDateTime SaveTimestamp;

    /** Total accumulated playtime in seconds at the time of save. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narrative|Save")
    float AccumulatedPlayTimeSeconds = 0.0f;

    /** Global world narrative snapshot (facts, factions, identity, endings, flags, vars). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narrative|Save")
    FSFNarrativeWorldState WorldState;

    /**
     * Aggregate narrative save data from all per-owner runtimes:
     *  - Quest snapshots
     *  - Lifetime task counters
     *  - Dialogue state snapshots
     *  - Any other narrative data you put into FSFNarrativeSaveData
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narrative|Save")
    FSFNarrativeSaveData NarrativeData;

    //
    // Helper methods
    //

    /** Clear narrative payloads but keep metadata. */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Save")
    void ResetNarrative();

    /** Returns true if the save has any narrative content at all. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Save")
    bool HasNarrativeContent() const;
};
