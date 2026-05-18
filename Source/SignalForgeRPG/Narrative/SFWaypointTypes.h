// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/PrimaryAssetId.h"
#include "SFWaypointTypes.generated.h"

class AActor;
class ASFNarrativeWaypoint;

/**
 * Where a runtime waypoint's transform originated. Useful for UI ("X is
 * moving" vs "fixed location") and debugging.
 */
UENUM(BlueprintType)
enum class ESFWaypointSource : uint8
{
    /** Resolved from an ASFNarrativeWaypoint actor in the level. */
    LevelActor,

    /** Resolved from FSFQuestTaskDefinition::WaypointLocation. */
    AuthoredCoordinate,

    /** No location available (waypoint registered but actor missing / level unloaded). */
    Unresolved
};

/**
 * Single immutable view of an active waypoint produced by
 * USFNarrativeWaypointSubsystem. UI/HUD reads this; the subsystem rebuilds
 * the array whenever quest state, task progress, registered actors, or the
 * tracked quest change.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFWaypointSnapshot
{
    GENERATED_BODY()

    /** Owning quest's stable id (FSFQuestDefinition::QuestId). */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|Waypoint")
    FName QuestId = NAME_None;

    /** PrimaryAssetId of the owning quest (for routing across asset paths). */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|Waypoint")
    FPrimaryAssetId QuestAssetId;

    /** Task id this waypoint represents (FSFQuestTaskDefinition::TaskId). */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|Waypoint")
    FName TaskId = NAME_None;

    /** World-space location to draw the marker at. */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|Waypoint")
    FVector WorldLocation = FVector::ZeroVector;

    /** Resolution path that produced WorldLocation. */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|Waypoint")
    ESFWaypointSource Source = ESFWaypointSource::Unresolved;

    /** Final display label for the HUD (authored override -> objective text -> task id). */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|Waypoint")
    FText DisplayText;

    /** Icon-selection tag (designer styles per tag). */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|Waypoint")
    FGameplayTag IconTag;

    /** Whether this waypoint should appear on a compass/minimap. */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|Waypoint")
    bool bRevealOnCompass = true;

    /** True if the owning quest is the user's tracked quest. */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|Waypoint")
    bool bIsTracked = false;

    /** Weak reference to the level actor (when Source == LevelActor). */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|Waypoint")
    TWeakObjectPtr<AActor> SourceActor;

    /** Convenience: a stable composite key for diffing. */
    FString MakeKey() const
    {
        return FString::Printf(TEXT("%s|%s"), *QuestId.ToString(), *TaskId.ToString());
    }

    bool IsValidSnapshot() const
    {
        return Source != ESFWaypointSource::Unresolved
            && !QuestId.IsNone()
            && !TaskId.IsNone();
    }
};
