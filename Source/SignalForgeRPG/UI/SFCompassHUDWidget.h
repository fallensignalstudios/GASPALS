// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Narrative/SFWaypointTypes.h"
#include "UI/SFUserWidgetBase.h"
#include "SFCompassHUDWidget.generated.h"

class USFNarrativeWaypointSubsystem;

/**
 * Per-frame view of a single compass marker, computed from a waypoint
 * snapshot and the player's current view yaw. Designers consume these to
 * drive icon position along a horizontal compass strip.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFCompassMarker
{
	GENERATED_BODY()

	/** The underlying waypoint snapshot this marker represents. */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|Waypoint")
	FSFWaypointSnapshot Waypoint;

	/**
	 * Position along the compass strip in [-1, +1], where -1 is the left
	 * edge (HalfFov to the player's left), 0 is dead-center (forward), and
	 * +1 is the right edge. Values outside [-1, +1] indicate the marker is
	 * outside the visible strip. Designers usually clamp + scale this to
	 * the strip's pixel width.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|Waypoint")
	float StripCoordinate = 0.0f;

	/**
	 * Signed angular offset from the player's forward direction, in
	 * degrees. Negative = left, positive = right. Wrapped to [-180, +180].
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|Waypoint")
	float AngularOffsetDegrees = 0.0f;

	/** True when the marker sits behind the player (|AngularOffset| > 90). */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|Waypoint")
	bool bIsBehind = false;

	/** Horizontal distance from the player to the waypoint, in meters. */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|Waypoint")
	float DistanceMeters = 0.0f;

	/** Convenience: this marker matches the user's tracked quest. */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|Waypoint")
	bool bIsTracked = false;
};

/**
 * Per-frame view of a cardinal/sub-cardinal tick (N, NE, E, ...) used to
 * drive the compass strip's background label positions.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFCompassCardinalTick
{
	GENERATED_BODY()

	/** Label text such as "N", "NE", "E". Drives the visual tick. */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Compass")
	FString Label;

	/** Yaw this tick represents (degrees, world-space, N = 0). */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Compass")
	float YawDegrees = 0.0f;

	/** Strip coordinate the same way FSFCompassMarker computes it. */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Compass")
	float StripCoordinate = 0.0f;

	/** Signed angular offset from player forward, [-180, +180]. */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Compass")
	float AngularOffsetDegrees = 0.0f;
};

/**
 * Player-facing compass widget.
 *
 * Responsibilities:
 *  - Track the player's forward yaw each tick and expose it for BP visuals.
 *  - Provide cardinal / sub-cardinal tick data (N, NE, E, SE, S, SW, W, NW)
 *    suitable for a scrolling horizontal compass strip.
 *  - Subscribe to USFNarrativeWaypointSubsystem so any active waypoint with
 *    bRevealOnCompass = true appears as a marker (and the user's tracked
 *    waypoint can be drawn with emphasis).
 *
 * Designers drive the visual layout entirely from blueprints. Typical wiring:
 *  - On BP_OnCompassTick, position background labels (cardinals) and marker
 *    icons using StripCoordinate * (StripPixelWidth * 0.5).
 *  - On BP_OnCompassMarkersChanged, rebuild the pool of marker child widgets.
 *
 * The widget self-resolves the narrative waypoint subsystem on construct,
 * with a retry pump for early-boot timing (mirrors the toast / waypoint HUD).
 */
UCLASS(Blueprintable)
class SIGNALFORGERPG_API USFCompassHUDWidget : public USFUserWidgetBase
{
	GENERATED_BODY()

public:
	// -- Read API (BP-friendly) --

	/** Current player view yaw in degrees, wrapped to [0, 360). */
	UFUNCTION(BlueprintPure, Category = "Narrative|Compass")
	float GetPlayerYawDegrees() const { return CachedPlayerYaw; }

	/** Half-angle (in degrees) that maps to StripCoordinate == 1.0. */
	UFUNCTION(BlueprintPure, Category = "Narrative|Compass")
	float GetCompassHalfFovDegrees() const { return CompassHalfFovDegrees; }

	/**
	 * Compute the [-1, +1] strip coordinate for a world-space target. Returns
	 * false when the calling context has no valid player view (e.g. the local
	 * player controller isn't available yet).
	 *
	 * @param OutStripCoordinate -1..+1 strip position (can exceed range when off-strip).
	 * @param OutAngularOffsetDegrees signed degrees from player forward (left negative, right positive).
	 * @param OutbIsBehind true when |angle| > 90.
	 */
	UFUNCTION(BlueprintCallable, Category = "Narrative|Compass")
	bool GetStripCoordinateForWorldLocation(
		FVector WorldLocation,
		float& OutStripCoordinate,
		float& OutAngularOffsetDegrees,
		bool& OutbIsBehind) const;

	/**
	 * Compute the [-1, +1] strip coordinate for a world yaw (N = 0, E = 90,
	 * S = 180, W = 270). Useful for placing fixed compass ticks.
	 */
	UFUNCTION(BlueprintCallable, Category = "Narrative|Compass")
	bool GetStripCoordinateForYaw(
		float TargetYawDegrees,
		float& OutStripCoordinate,
		float& OutAngularOffsetDegrees) const;

	/**
	 * The marker list computed during the last tick. Each entry contains a
	 * full FSFWaypointSnapshot plus its current strip coordinate so the
	 * blueprint can position icons without recomputing geometry.
	 */
	UFUNCTION(BlueprintPure, Category = "Narrative|Compass")
	const TArray<FSFCompassMarker>& GetCompassMarkers() const { return CompassMarkers; }

	/**
	 * The cardinal tick list computed during the last tick. Always returns
	 * the 8 sub-cardinal directions (N, NE, E, SE, S, SW, W, NW) — designers
	 * can ignore subset (e.g. only show majors when zoomed).
	 */
	UFUNCTION(BlueprintPure, Category = "Narrative|Compass")
	const TArray<FSFCompassCardinalTick>& GetCardinalTicks() const { return CardinalTicks; }

	/** True if the user is currently tracking a quest with a waypoint. */
	UFUNCTION(BlueprintPure, Category = "Narrative|Compass")
	bool HasTrackedWaypoint() const { return bHasTrackedWaypoint; }

	/** Snapshot of the tracked waypoint (read inside BP_OnTrackedWaypointChanged or per-tick). */
	UFUNCTION(BlueprintPure, Category = "Narrative|Compass")
	const FSFWaypointSnapshot& GetTrackedWaypoint() const { return CurrentTrackedWaypoint; }

	// -- Designer-tunable behaviour --

	/**
	 * How wide (in world yaw degrees) the compass strip represents from
	 * center to one edge. 90 means the strip shows ~180 degrees of arc; 60
	 * is a tighter, more zoomed feel; 30 is very zoomed. Designers usually
	 * pick something between 45 and 90.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Compass", meta = (ClampMin = "10.0", ClampMax = "180.0"))
	float CompassHalfFovDegrees = 90.0f;

	/**
	 * Master "off-strip culling" threshold. Markers whose |StripCoordinate|
	 * exceeds this are excluded from BP_OnCompassTick / GetCompassMarkers.
	 * Set > 1.0 to allow marker pinning at the edges, or huge to disable.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Compass", meta = (ClampMin = "0.5", ClampMax = "10.0"))
	float MaxStripCoordinateAbs = 1.05f;

	/**
	 * If true, GetPlayerYawDegrees uses the camera/view rotation (matches
	 * what the player sees on screen). If false, uses the control rotation
	 * (the input intent). For most third-person and first-person games this
	 * should be true.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Compass")
	bool bUseCameraRotation = true;

protected:
	// -- UUserWidget --
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// -- Subsystem wiring --
	UFUNCTION()
	void HandleWaypointsChanged();

	UFUNCTION()
	void HandleTrackedWaypointChanged(const FSFWaypointSnapshot& InTrackedWaypoint);

	void TryResolveSubsystem();

	UFUNCTION()
	void TickAutoResolveRetry();

	void StartAutoResolveRetryTimer();
	void StopAutoResolveRetryTimer();

	// -- BP hooks (designer-driven visuals) --

	/**
	 * Fires every tick AFTER the widget refreshes player yaw, cardinal
	 * ticks, and compass marker strip coordinates. Designers use this to
	 * reposition labels/icons. No data is passed because GetCompassMarkers /
	 * GetCardinalTicks / GetPlayerYawDegrees already expose the current
	 * frame's data.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Narrative|Compass")
	void BP_OnCompassTick(float DeltaSeconds);

	/**
	 * Fires when the set of compass markers changes (a waypoint was added,
	 * removed, or its identity/icon tag changed). Designers usually rebuild
	 * their marker child-widget pool here.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Narrative|Compass")
	void BP_OnCompassMarkersChanged(const TArray<FSFCompassMarker>& Markers);

	/** Fires when the tracked waypoint identity changes (or appears / disappears). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Narrative|Compass")
	void BP_OnTrackedWaypointChanged(const FSFWaypointSnapshot& TrackedWaypoint, bool bHasTracked);

	// -- Internals --

	/** Pulls a fresh player yaw from the local view and writes CachedPlayerYaw. */
	void RefreshPlayerYaw();

	/** Rebuild CompassMarkers from the subsystem's active waypoints list (identity change). */
	void RebuildCompassMarkers();

	/** Recompute strip coordinates on the existing marker / tick lists for this frame. */
	void RecomputeStripCoordinates();

	/** Rebuild the 8-entry cardinal tick label table (called once at construct). */
	void BuildCardinalTickLabels();

	/** Convert a desired angular offset (signed degrees) to strip coordinate using the configured half-FOV. */
	float AngleToStripCoordinate(float SignedAngleDegrees) const;

	/** Wrap a yaw to [0, 360). */
	static float WrapYawPositive(float Yaw);

	/** Wrap a signed angle to [-180, +180]. */
	static float WrapSignedDegrees(float Angle);

protected:
	UPROPERTY()
	TWeakObjectPtr<USFNarrativeWaypointSubsystem> Subsystem;

	UPROPERTY()
	TArray<FSFWaypointSnapshot> CachedActiveWaypoints;

	UPROPERTY()
	TArray<FSFCompassMarker> CompassMarkers;

	UPROPERTY()
	TArray<FSFCompassCardinalTick> CardinalTicks;

	UPROPERTY()
	FSFWaypointSnapshot CurrentTrackedWaypoint;

	UPROPERTY()
	bool bHasTrackedWaypoint = false;

	UPROPERTY()
	float CachedPlayerYaw = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Narrative|Compass", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float AutoResolveRetryPeriodSeconds = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Narrative|Compass", meta = (ClampMin = "0.5", ClampMax = "60.0"))
	float MaxAutoResolveRetrySeconds = 10.0f;

	FTimerHandle AutoResolveRetryHandle;
	float AutoResolveRetryElapsedSeconds = 0.0f;
};
