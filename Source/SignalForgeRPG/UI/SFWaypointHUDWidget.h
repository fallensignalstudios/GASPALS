// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Narrative/SFWaypointTypes.h"
#include "UI/SFUserWidgetBase.h"
#include "SFWaypointHUDWidget.generated.h"

class USFNarrativeWaypointSubsystem;

/**
 * Minimal HUD-side host for the waypoint marker. Subscribes to
 * USFNarrativeWaypointSubsystem and republishes a clean snapshot to BP.
 *
 * Designers drive the visual look entirely from blueprints:
 *  - BP_OnTrackedWaypointChanged fires when the hero arrow target changes
 *  - BP_OnActiveWaypointsChanged fires whenever the full list changes
 *  - GetTrackedWaypointScreenInfo(...) computes screen-space data each tick
 *    so the widget can show a 3D-anchored marker / off-screen arrow
 *
 * The widget doesn't enforce a layout. A typical setup places one
 * compass strip at the top and one off-screen arrow indicator on top of
 * the main viewport.
 */
UCLASS(Blueprintable)
class SIGNALFORGERPG_API USFWaypointHUDWidget : public USFUserWidgetBase
{
    GENERATED_BODY()

public:
    /** Read-only snapshot of the tracked waypoint (call inside BP_OnTrackedWaypointChanged or each tick). */
    UFUNCTION(BlueprintPure, Category = "Narrative|Quest|Waypoint")
    const FSFWaypointSnapshot& GetTrackedWaypoint() const { return CurrentTrackedWaypoint; }

    /** All currently active waypoints, including the tracked one. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Quest|Waypoint")
    const TArray<FSFWaypointSnapshot>& GetActiveWaypoints() const { return CurrentActiveWaypoints; }

    /**
     * Screen-space helper for the tracked waypoint. Returns true when a
     * valid waypoint is being tracked. OutScreenPosition is the projected
     * screen-space position in pixels, OutbIsOnScreen indicates whether the
     * waypoint is in the player's view, and OutDistance is the world-space
     * distance from the camera in centimeters.
     */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|Waypoint")
    bool GetTrackedWaypointScreenInfo(FVector2D& OutScreenPosition, bool& OutbIsOnScreen, float& OutDistance) const;

    /** Designer-tunable: how often we re-poll the subsystem to refresh actor positions (cheap, no rebuild). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Quest|Waypoint", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ActorPositionRefreshSeconds = 0.0f;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION()
    void HandleWaypointsChanged();

    UFUNCTION()
    void HandleTrackedWaypointChanged(const FSFWaypointSnapshot& InTrackedWaypoint);

    /** Resolve the subsystem; retry briefly if the world hasn't begun play yet. */
    void TryResolveSubsystem();

    UFUNCTION()
    void TickAutoResolveRetry();

    void StartAutoResolveRetryTimer();
    void StopAutoResolveRetryTimer();

    /** Blueprint hooks for visual updates. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Narrative|Quest|Waypoint")
    void BP_OnTrackedWaypointChanged(const FSFWaypointSnapshot& TrackedWaypoint, bool bHasTracked);

    UFUNCTION(BlueprintImplementableEvent, Category = "Narrative|Quest|Waypoint")
    void BP_OnActiveWaypointsChanged(const TArray<FSFWaypointSnapshot>& ActiveWaypoints);

protected:
    UPROPERTY()
    TWeakObjectPtr<USFNarrativeWaypointSubsystem> Subsystem;

    UPROPERTY()
    FSFWaypointSnapshot CurrentTrackedWaypoint;

    UPROPERTY()
    TArray<FSFWaypointSnapshot> CurrentActiveWaypoints;

    UPROPERTY()
    bool bHasTrackedWaypoint = false;

    UPROPERTY(EditDefaultsOnly, Category = "Narrative|Quest|Waypoint", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float AutoResolveRetryPeriodSeconds = 0.5f;

    UPROPERTY(EditDefaultsOnly, Category = "Narrative|Quest|Waypoint", meta = (ClampMin = "0.5", ClampMax = "60.0"))
    float MaxAutoResolveRetrySeconds = 10.0f;

    FTimerHandle AutoResolveRetryHandle;
    float AutoResolveRetryElapsedSeconds = 0.0f;
};
