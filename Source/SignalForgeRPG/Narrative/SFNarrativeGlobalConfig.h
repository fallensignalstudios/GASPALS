#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SFNarrativeGlobalConfig.generated.h"

/**
 * Lightweight global config struct for narrative runtime tuning.
 *
 * This is typically owned by a high-level narrative subsystem or
 * stored as a config variable on your game instance / game mode.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeGlobalConfig
{
    GENERATED_BODY()

    //
    // Save / persistence
    //

    /** Base slot name prefix for narrative saves (actual slot can append profile id). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Config|Save")
    FString DefaultSaveSlotBase = TEXT("Narrative");

    /** Default user index for narrative saves (0 for single-player). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Config|Save", meta = (ClampMin = "0"))
    int32 DefaultUserIndex = 0;

    /** If true, narrative system auto-saves after important beats (quest complete, ending locked in). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Config|Save")
    bool bAutoSaveOnMajorBeats = true;

    /** Cooldown (seconds) between auto-save attempts. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Config|Save", meta = (ClampMin = "0.0"))
    float AutoSaveCooldownSeconds = 60.0f;

    //
    // Faction / identity tuning
    //

    /** Default min/max clamp for identity axes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Config|Identity")
    float IdentityAxisMin = -100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Config|Identity")
    float IdentityAxisMax = 100.0f;

    /** Default neutral band threshold around zero for identity axes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Config|Identity")
    float IdentityNeutralEpsilon = 5.0f;

    /** Minimum absolute magnitude for an identity change to be considered “meaningful” for UI. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Config|Identity")
    float IdentityDeltaVisualThreshold = 1.0f;

    /** Default clamps for faction standing aggregate scores. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Config|Faction")
    float FactionScoreMin = -100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Config|Faction")
    float FactionScoreMax = 100.0f;

    //
    // Replication / networking
    //

    /** Maximum number of narrative deltas sent to a client per net update. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Config|Net", meta = (ClampMin = "1"))
    int32 MaxReplicatedDeltasPerTick = 16;

    /** If true, world facts marked as “debug-only” are never replicated to clients. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Config|Net")
    bool bSkipDebugFactsForClients = true;

    //
    // Events / pacing
    //

    /** Global cooldown between firing *random* narrative events (seconds). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Config|Events", meta = (ClampMin = "0.0"))
    float GlobalRandomEventCooldownSeconds = 30.0f;

    /** Maximum number of narrative events allowed to fire in a single frame/tick. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Config|Events", meta = (ClampMin = "1"))
    int32 MaxEventsPerTick = 4;

    //
    // Client sync / UI
    //

    /** Maximum number of client sync events buffered before older ones are dropped. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Config|Client")
    int32 MaxClientEventBuffer = 64;

    /** If true, low-urgency client events are collapsed when many similar events arrive. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Config|Client")
    bool bCollapseQuietEvents = true;

    //
    // Core narrative tags (roots)
    //

    /** Root tag for all faction tags (e.g. Narrative.Faction). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Config|Tags")
    FGameplayTag FactionRootTag;

    /** Root tag for all identity axis tags (e.g. Narrative.Identity). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Config|Tags")
    FGameplayTag IdentityRootTag;

    /** Root tag for all ending tags (e.g. Narrative.Ending). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Config|Tags")
    FGameplayTag EndingRootTag;

    /** Root tag for world facts (e.g. Narrative.Fact). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Config|Tags")
    FGameplayTag WorldFactRootTag;

    /** Root tag for quest-related tasks (e.g. Narrative.Task). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Config|Tags")
    FGameplayTag QuestTaskRootTag;

    //
    // Utility
    //

    /** Clamp a value to identity axis bounds. */
    float ClampIdentityAxis(float Value) const
    {
        return FMath::Clamp(Value, IdentityAxisMin, IdentityAxisMax);
    }

    /** Clamp a value to faction aggregate score bounds. */
    float ClampFactionScore(float Value) const
    {
        return FMath::Clamp(Value, FactionScoreMin, FactionScoreMax);
    }
};