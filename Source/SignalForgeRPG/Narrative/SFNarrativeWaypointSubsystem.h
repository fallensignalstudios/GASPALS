// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "Narrative/SFWaypointTypes.h"
#include "Narrative/SFQuestDisplayTypes.h"
#include "SFNarrativeWaypointSubsystem.generated.h"

class APlayerController;
class ASFNarrativeWaypoint;
class USFNarrativeComponent;
class USFQuestDefinition;
class USFQuestInstance;
class USFQuestLogWidgetController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSFOnWaypointsChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSFOnTrackedWaypointChangedSignature, const FSFWaypointSnapshot&, TrackedWaypoint);

/**
 * Single source of truth for active quest waypoints in the current world.
 *
 * The subsystem watches the local player's USFNarrativeComponent for quest
 * state and task progress changes, plus any USFQuestLogWidgetController's
 * tracked-quest pin, and republishes a deduplicated array of
 * FSFWaypointSnapshot for HUDs / minimaps / compasses to consume.
 *
 * Resolution order per (QuestId, TaskId):
 *  1. A registered ASFNarrativeWaypoint actor whose key matches
 *  2. FSFQuestTaskDefinition::WaypointLocation when bHasWaypoint is true,
 *     gated by WaypointLevel being either empty or loaded
 *
 * The subsystem is purely observational; it does not own quest state and
 * therefore is safe to recreate (e.g. on level travel) without touching
 * save data.
 */
UCLASS()
class SIGNALFORGERPG_API USFNarrativeWaypointSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // -- UWorldSubsystem --
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

    // -- Public read API --

    /** Snapshot of every active waypoint, in tracked-first order. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Quest|Waypoint")
    const TArray<FSFWaypointSnapshot>& GetActiveWaypoints() const { return ActiveWaypoints; }

    /**
     * The single best waypoint to render as a hero arrow / compass anchor:
     * the tracked quest's first incomplete task that has a waypoint. Falls
     * back to the first active waypoint when nothing is tracked.
     *
     * @return True if OutWaypoint was populated.
     */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|Waypoint")
    bool GetTrackedWaypoint(FSFWaypointSnapshot& OutWaypoint) const;

    /** Fired whenever ActiveWaypoints meaningfully changes. */
    UPROPERTY(BlueprintAssignable, Category = "Narrative|Quest|Waypoint")
    FSFOnWaypointsChangedSignature OnWaypointsChanged;

    /** Fired when the "tracked" waypoint changes (or appears / disappears). */
    UPROPERTY(BlueprintAssignable, Category = "Narrative|Quest|Waypoint")
    FSFOnTrackedWaypointChangedSignature OnTrackedWaypointChanged;

    // -- Registration API used by ASFNarrativeWaypoint --

    /** Register a level actor as the marker for the supplied (QuestId, TaskId). */
    void RegisterWaypointActor(ASFNarrativeWaypoint* Actor);

    /** Detach a previously registered actor. Safe to call with a null/stale actor. */
    void UnregisterWaypointActor(ASFNarrativeWaypoint* Actor);

    // -- Adopt an external controller's tracked-quest signal. --

    /** Wire the subsystem to a quest log controller so changing the tracked quest updates the tracked waypoint. */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|Waypoint")
    void AdoptQuestLogWidgetController(USFQuestLogWidgetController* Controller);

    /**
     * Manually set the tracked quest id when there is no widget controller
     * driving the selection (e.g. early in boot or for tests).
     */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|Waypoint")
    void SetTrackedQuestId(FName QuestId);

    UFUNCTION(BlueprintPure, Category = "Narrative|Quest|Waypoint")
    FName GetTrackedQuestId() const { return TrackedQuestId; }

    /** Force a recompute. Useful after manual data changes / debugging. */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|Waypoint")
    void RefreshActiveWaypoints();

protected:
    // Narrative component binding (mirrors the toast / quest log retry pump).
    void TryBindToLocalPlayerNarrativeComponent();
    UFUNCTION()
    void TickAutoResolveRetry();
    void StartAutoResolveRetryTimer();
    void StopAutoResolveRetryTimer();

    void BindToNarrativeComponent(USFNarrativeComponent* NC);
    void UnbindFromNarrativeComponent();

    // Narrative delegate handlers.
    UFUNCTION()
    void HandleQuestStarted(USFQuestInstance* QuestInstance);
    UFUNCTION()
    void HandleQuestStateChanged(USFQuestInstance* QuestInstance, FName StateId);
    UFUNCTION()
    void HandleQuestTaskProgressed(USFQuestInstance* QuestInstance, FGameplayTag TaskTag, FName ContextId, int32 Quantity);
    UFUNCTION()
    void HandleQuestAbandoned(USFQuestInstance* QuestInstance);

    // Widget controller binding.
    UFUNCTION()
    void HandleTrackedQuestChanged(FName InTrackedQuestId, bool bIsTracked);

    // Build steps.
    void RebuildActiveWaypoints();
    bool BuildSnapshotForTask(
        USFQuestInstance* Instance,
        const FSFQuestTaskDefinition& TaskDef,
        const FSFQuestObjectiveDisplayEntry* OptionalDisplay,
        FSFWaypointSnapshot& OutSnapshot) const;

    /** Is FSFQuestTaskDefinition::WaypointLevel currently loaded (or unspecified)? */
    bool IsAuthoredCoordinateAvailable(const FSFQuestTaskDefinition& TaskDef) const;

    /** Diff and broadcast OnWaypointsChanged / OnTrackedWaypointChanged. */
    void DiffAndBroadcast(const TArray<FSFWaypointSnapshot>& OldList, const FSFWaypointSnapshot& OldTracked);

    APlayerController* GetLocalPlayerController() const;

protected:
    UPROPERTY()
    TWeakObjectPtr<USFNarrativeComponent> BoundNarrativeComponent;

    UPROPERTY()
    TWeakObjectPtr<USFQuestLogWidgetController> BoundQuestLogController;

    /** All currently registered ASFNarrativeWaypoint actors, keyed by "QuestId|TaskId". */
    UPROPERTY()
    TMap<FString, TWeakObjectPtr<ASFNarrativeWaypoint>> RegisteredActorsByKey;

    UPROPERTY()
    TArray<FSFWaypointSnapshot> ActiveWaypoints;

    UPROPERTY()
    FName TrackedQuestId = NAME_None;

    // Auto-resolve retry plumbing (mirrors the toast widget).
    UPROPERTY(EditDefaultsOnly, Category = "Narrative|Quest|Waypoint", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float AutoResolveRetryPeriodSeconds = 0.5f;

    UPROPERTY(EditDefaultsOnly, Category = "Narrative|Quest|Waypoint", meta = (ClampMin = "0.5", ClampMax = "60.0"))
    float MaxAutoResolveRetrySeconds = 15.0f;

    FTimerHandle AutoResolveRetryHandle;
    float AutoResolveRetryElapsedSeconds = 0.0f;
};
