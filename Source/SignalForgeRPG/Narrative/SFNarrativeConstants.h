// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SFNarrativeTypes.h"

/**
 * Compile-time and load-time constants for the narrative subsystem.
 *
 * Values that need to be designer-tunable should be moved to a USFNarrativeConfigAsset
 * (TODO: introduce when content needs per-difficulty tuning); for now, everything is
 * a stable defualt the runtime can rely on.
 */
class SIGNALFORGERPG_API FSFNarrativeConstants
{
public:
    //
    // Save schema & migration
    //

    /** Current expected schema version for FSFNarrativeSaveData. Bump when the layout changes. */
    static constexpr int32 CurrentSaveSchemaVersion = 2;

    /** Oldest schema version we accept without hard-failing the load. */
    static constexpr int32 MinSupportedSaveSchemaVersion = 1;

    //
    // Quest system defaults
    //

    /** Fallback start state when a quest definition omits one. */
    static const FName DefaultQuestStartStateId;

    /** Maximum number of state ids we keep in FSFQuestSnapshot::ReachedStateIds. */
    static constexpr int32 MaxQuestStateHistory = 64;

    /** Hard cap on a single task's progress count. */
    static constexpr int32 MaxTaskProgressPerTask = 9999;

    /** Minimum task progress delta before we emit a TaskProgress wire delta. */
    static constexpr int32 MinReplicatedTaskDelta = 1;

    //
    // Dialogue system defaults
    //

    /** Default dialogue start node id when none is specified. */
    static const FName DefaultDialogueStartNodeId;

    /** Default dialogue priority when none is specified (higher wins). */
    static constexpr int32 DefaultDialoguePriority = 0;

    /** Whether higher-priority sessions may interrupt lower-priority ones by default. */
    static constexpr bool bAllowInterruptLowerPriorityDialogueByDefault = true;

    /** Per-session debug history depth. */
    static constexpr int32 MaxDialogueHistoryEntries = 128;

    //
    // Faction system defaults
    //

    /** Default value for any faction metric on first contact. */
    static constexpr float DefaultFactionMetricValue = 0.f;

    /** Threshold above which a metric is considered "high" (used by AI/VO selectors). */
    static constexpr float HighFactionMetricThreshold = 50.f;

    /** Threshold below which a metric is considered "low". */
    static constexpr float LowFactionMetricThreshold = -50.f;

    /** Clamp range for all faction metrics. */
    static constexpr float FactionMetricMin = -100.f;
    static constexpr float FactionMetricMax = 100.f;

    /** Standing band thresholds (applied on the dominant metric, e.g. Trust). */
    static constexpr float StandingBandHostileMax = -75.f; // <= -> Hostile
    static constexpr float StandingBandAdversarialMax = -25.f; // <= -> Adversarial
    static constexpr float StandingBandWaryMax = -5.f; // <= -> Wary
    static constexpr float StandingBandNeutralMax = 5.f; // <= -> Neutral
    static constexpr float StandingBandCooperativeMax = 50.f; // <= -> Cooperative
    static constexpr float StandingBandAlliedMax = 90.f; // <= -> Allied
    // > StandingBandAlliedMax -> Bound

    //
    // Identity / alignment defaults
    //

    /** Clamp range for identity axis values. */
    static constexpr float IdentityAxisMin = -100.f;
    static constexpr float IdentityAxisMax = 100.f;

    /** Absolute delta below this is treated as no meaningful change. */
    static constexpr float IdentityDeltaEpsilon = 0.01f;

    /** Thresholds for classifying drift direction. */
    static constexpr float IdentityPositiveThreshold = 0.1f;
    static constexpr float IdentityNegativeThreshold = -0.1f;

    //
    // Ending / outcome defaults
    //

    static constexpr float DefaultEndingScore = 0.f;

    /** Score threshold above which an ending is considered "dominant". */
    static constexpr float EndingDominantScoreThreshold = 100.f;

    /** Score below which an ending is effectively unavailable. */
    static constexpr float EndingUnavailableScoreThreshold = -100.f;

    //
    // Replication / delta stream
    //

    /** Maximum buffered deltas per client before we force a full resync. */
    static constexpr int32 MaxBufferedDeltasPerClient = 256;

    /** Maximum age (seconds) a delta may be replayed before requesting a resync. */
    static constexpr float MaxDeltaAgeSeconds = 10.f;

    //
    // Helpers
    //

    /** Map a raw standing float to the closest band using the thresholds above. */
    static ESFFactionStandingBand ClassifyFactionStanding(float StandingValue);

    /** Clamp any single faction metric to the allowed range. */
    static float ClampFactionMetric(float Value);

    /** Clamp an identity axis value to the allowed range. */
    static float ClampIdentityAxisValue(float Value);

    /** Classify drift direction given a recent delta. */
    static ESFIdentityDriftDirection ClassifyIdentityDrift(float Delta);
};

// Copyright Fallen Signal Studios LLC. All Rights Reserved.
