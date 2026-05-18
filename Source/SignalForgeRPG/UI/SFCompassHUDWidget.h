// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "Narrative/SFWaypointTypes.h"
#include "Styling/SlateBrush.h"
#include "UI/SFUserWidgetBase.h"
#include "SFCompassHUDWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UCanvasPanelSlot;
class UImage;
class UOverlay;
class USizeBox;
class USFNarrativeWaypointSubsystem;
class UTextBlock;

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

	/** Position along the compass strip in [-1, +1]. -1 = left edge, 0 = forward, +1 = right edge. */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|Waypoint")
	float StripCoordinate = 0.0f;

	/** Signed angular offset from player forward in degrees, wrapped to [-180, +180]. */
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
 * Per-frame view of a cardinal/sub-cardinal tick (N, NE, E, ...).
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFCompassCardinalTick
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Compass")
	FString Label;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Compass")
	float YawDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Compass")
	float StripCoordinate = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Compass")
	float AngularOffsetDegrees = 0.0f;
};

/**
 * Optional per-tag override for marker visuals.
 * Designers fill the TMap in widget defaults: tag -> brush.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFCompassMarkerStyle
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Compass")
	FSlateBrush IconBrush;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Compass")
	FLinearColor IconTint = FLinearColor::White;
};

/**
 * Fully self-contained player compass HUD widget.
 *
 * The widget builds its own visual tree at construct time (background +
 * clipped strip + center pip + cardinal labels) and spawns native marker
 * child widgets for every active quest waypoint. You add it to your HUD,
 * optionally drop a few brushes into its defaults, and you are done.
 *
 * What you get out of the box:
 *  - Eight cardinal/sub-cardinal labels (N, NE, E, SE, S, SW, W, NW) sliding
 *    on a horizontal strip as the player rotates.
 *  - One marker per active waypoint with bRevealOnCompass = true. The
 *    tracked quest's marker shows a ring overlay. Markers that move behind
 *    the player show a left/right "behind" chevron at the strip edge.
 *  - All sized/positioned in code — no WBP layout required.
 *
 * What you can customise (optional, in widget defaults):
 *  - StripPixelWidth, StripPixelHeight, BackgroundColor, CenterPipColor.
 *  - DefaultMarkerBrush, TrackedRingBrush, BehindArrowBrush, TickBrush.
 *  - MarkerStylesByTag — per-IconTag overrides for different waypoint types.
 *  - CompassHalfFovDegrees, MaxStripCoordinateAbs, bUseCameraRotation.
 *
 * The widget self-resolves USFNarrativeWaypointSubsystem on construct
 * (with the same retry pump as the toast widget), so early-boot ordering
 * does not break it.
 */
UCLASS(Blueprintable)
class SIGNALFORGERPG_API USFCompassHUDWidget : public USFUserWidgetBase
{
	GENERATED_BODY()

public:
	USFCompassHUDWidget(const FObjectInitializer& ObjectInitializer);

	// -- BP read API --

	UFUNCTION(BlueprintPure, Category = "Narrative|Compass")
	float GetPlayerYawDegrees() const { return CachedPlayerYaw; }

	UFUNCTION(BlueprintPure, Category = "Narrative|Compass")
	float GetCompassHalfFovDegrees() const { return CompassHalfFovDegrees; }

	UFUNCTION(BlueprintCallable, Category = "Narrative|Compass")
	bool GetStripCoordinateForWorldLocation(
		FVector WorldLocation,
		float& OutStripCoordinate,
		float& OutAngularOffsetDegrees,
		bool& OutbIsBehind) const;

	UFUNCTION(BlueprintCallable, Category = "Narrative|Compass")
	bool GetStripCoordinateForYaw(
		float TargetYawDegrees,
		float& OutStripCoordinate,
		float& OutAngularOffsetDegrees) const;

	UFUNCTION(BlueprintPure, Category = "Narrative|Compass")
	const TArray<FSFCompassMarker>& GetCompassMarkers() const { return CompassMarkers; }

	UFUNCTION(BlueprintPure, Category = "Narrative|Compass")
	const TArray<FSFCompassCardinalTick>& GetCardinalTicks() const { return CardinalTicks; }

	UFUNCTION(BlueprintPure, Category = "Narrative|Compass")
	bool HasTrackedWaypoint() const { return bHasTrackedWaypoint; }

	UFUNCTION(BlueprintPure, Category = "Narrative|Compass")
	const FSFWaypointSnapshot& GetTrackedWaypoint() const { return CurrentTrackedWaypoint; }

	// -- Designer-tunable layout --

	/** Strip width in pixels (drives both visuals and StripCoordinate -> pixel mapping). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Compass|Layout", meta = (ClampMin = "100.0"))
	float StripPixelWidth = 800.0f;

	/** Strip height in pixels. Cardinal labels and markers both fit inside this height. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Compass|Layout", meta = (ClampMin = "16.0"))
	float StripPixelHeight = 48.0f;

	/** Background panel tint (set alpha to 0 to disable the background entirely). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Compass|Layout")
	FLinearColor BackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.45f);

	/** Center forward indicator tint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Compass|Layout")
	FLinearColor CenterPipColor = FLinearColor(1.0f, 0.85f, 0.25f, 1.0f);

	/** Marker icon size in pixels. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Compass|Layout", meta = (ClampMin = "8.0"))
	float MarkerPixelSize = 24.0f;

	/** Cardinal label vertical offset from strip top (pixels). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Compass|Layout")
	float CardinalLabelTopPadding = 4.0f;

	/** Distance (in pixels) the compass is offset from the top edge of the viewport. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Compass|Layout")
	float CompassTopOffsetPixels = 24.0f;

	// -- Designer-tunable visuals (optional — sensible fallbacks built in) --

	/** Cardinal tick image (a thin vertical line). If unset, a solid colored box is drawn. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Compass|Visuals")
	FSlateBrush TickBrush;

	/** Default marker icon when no per-tag override applies. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Compass|Visuals")
	FSlateBrush DefaultMarkerBrush;

	/** Ring overlay drawn on the tracked marker. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Compass|Visuals")
	FSlateBrush TrackedRingBrush;

	/** Small chevron drawn when a marker is behind the player. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Compass|Visuals")
	FSlateBrush BehindArrowBrush;

	/** Per-IconTag overrides — looked up by FSFWaypointSnapshot::IconTag. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Compass|Visuals")
	TMap<FGameplayTag, FSFCompassMarkerStyle> MarkerStylesByTag;

	/** Tint applied to the tracked marker icon (independent of the ring brush). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Compass|Visuals")
	FLinearColor TrackedMarkerTint = FLinearColor(1.0f, 0.85f, 0.25f, 1.0f);

	/** Tint applied to non-tracked marker icons (multiplies onto the brush). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Compass|Visuals")
	FLinearColor UntrackedMarkerTint = FLinearColor::White;

	/** Distance text font size. Set to 0 to hide distance labels entirely. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Compass|Visuals", meta = (ClampMin = "0.0"))
	float DistanceFontSize = 9.0f;

	// -- Designer-tunable behaviour --

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Compass", meta = (ClampMin = "10.0", ClampMax = "180.0"))
	float CompassHalfFovDegrees = 90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Compass", meta = (ClampMin = "0.5", ClampMax = "10.0"))
	float MaxStripCoordinateAbs = 1.05f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Compass")
	bool bUseCameraRotation = true;

	/** When markers move behind the player, pin them to the strip edge with the behind chevron instead of culling. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Compass")
	bool bPinBehindMarkersToEdge = true;

	/** Show distance text below each marker (e.g. "120m"). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Compass")
	bool bShowMarkerDistance = true;

protected:
	// -- UUserWidget --
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	// -- BP hooks (still available for designers who want extra polish) --
	UFUNCTION(BlueprintImplementableEvent, Category = "Narrative|Compass")
	void BP_OnCompassTick(float DeltaSeconds);

	UFUNCTION(BlueprintImplementableEvent, Category = "Narrative|Compass")
	void BP_OnCompassMarkersChanged(const TArray<FSFCompassMarker>& Markers);

	UFUNCTION(BlueprintImplementableEvent, Category = "Narrative|Compass")
	void BP_OnTrackedWaypointChanged(const FSFWaypointSnapshot& TrackedWaypoint, bool bHasTracked);

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

	// -- Internals --
	void BuildCardinalTickLabels();
	void BuildVisualTree();
	void SpawnCardinalLabelWidgets();
	void RebuildMarkerWidgets();
	void RefreshPlayerYaw();
	void RecomputeStripCoordinates();
	void ApplyMarkerVisualState();
	void UpdateChildPositions();

	float AngleToStripCoordinate(float SignedAngleDegrees) const;
	float StripCoordinateToPixelX(float StripCoordinate) const;
	void ResolveMarkerStyle(const FSFWaypointSnapshot& Snapshot, FSlateBrush& OutBrush, FLinearColor& OutTint) const;

	static float WrapYawPositive(float Yaw);
	static float WrapSignedDegrees(float Angle);
	static FString MakeMarkerKey(const FSFWaypointSnapshot& Snapshot);

private:
	// Cached visual tree pointers (created in RebuildWidget).
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> CompassSizeBox;

	UPROPERTY(Transient)
	TObjectPtr<UOverlay> CompassOverlay;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> BackgroundBorder;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> StripCanvas;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> CenterPipBorder;

	// Spawned label children keyed by their cardinal text.
	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UOverlay>> CardinalLabelOverlays;

	// Spawned marker children keyed by "QuestId|TaskId".
	struct FMarkerWidgetSet
	{
		TWeakObjectPtr<UOverlay> Root;
		TWeakObjectPtr<UWidget> Icon;        // UImage when designer brush set, UBorder when fallback.
		TWeakObjectPtr<UWidget> Ring;        // ditto.
		TWeakObjectPtr<UWidget> BehindArrow; // ditto.
		TWeakObjectPtr<UTextBlock> DistanceText;
		bool bIconIsBorder = false;
		bool bRingIsBorder = false;
		bool bBehindIsBorder = false;
	};
	TMap<FString, FMarkerWidgetSet> MarkerWidgets;

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

	UPROPERTY(EditDefaultsOnly, Category = "Narrative|Compass|Retry", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float AutoResolveRetryPeriodSeconds = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Narrative|Compass|Retry", meta = (ClampMin = "0.5", ClampMax = "60.0"))
	float MaxAutoResolveRetrySeconds = 10.0f;

	FTimerHandle AutoResolveRetryHandle;
	float AutoResolveRetryElapsedSeconds = 0.0f;

	bool bVisualTreeBuilt = false;
};
