// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "UI/SFCompassHUDWidget.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Narrative/SFNarrativeWaypointSubsystem.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogSFCompassHUD, Log, All);

namespace SFCompassHUD
{
	// Sub-cardinal labels, ordered N, NE, E, SE, S, SW, W, NW.
	static const TCHAR* CardinalLabels[] = {
		TEXT("N"), TEXT("NE"), TEXT("E"), TEXT("SE"),
		TEXT("S"), TEXT("SW"), TEXT("W"), TEXT("NW")
	};
	static constexpr int32 NumCardinalLabels = UE_ARRAY_COUNT(CardinalLabels);
	static constexpr float CardinalStepDegrees = 360.0f / static_cast<float>(NumCardinalLabels);
}

void USFCompassHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildCardinalTickLabels();
	TryResolveSubsystem();
}

void USFCompassHUDWidget::NativeDestruct()
{
	StopAutoResolveRetryTimer();

	if (USFNarrativeWaypointSubsystem* Sub = Subsystem.Get())
	{
		Sub->OnWaypointsChanged.RemoveDynamic(this, &USFCompassHUDWidget::HandleWaypointsChanged);
		Sub->OnTrackedWaypointChanged.RemoveDynamic(this, &USFCompassHUDWidget::HandleTrackedWaypointChanged);
	}
	Subsystem = nullptr;

	Super::NativeDestruct();
}

void USFCompassHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshPlayerYaw();
	RecomputeStripCoordinates();
	BP_OnCompassTick(InDeltaTime);
}

void USFCompassHUDWidget::BuildCardinalTickLabels()
{
	CardinalTicks.Reset(SFCompassHUD::NumCardinalLabels);
	for (int32 Index = 0; Index < SFCompassHUD::NumCardinalLabels; ++Index)
	{
		FSFCompassCardinalTick Tick;
		Tick.Label = SFCompassHUD::CardinalLabels[Index];
		Tick.YawDegrees = static_cast<float>(Index) * SFCompassHUD::CardinalStepDegrees;
		Tick.StripCoordinate = 0.0f;
		Tick.AngularOffsetDegrees = 0.0f;
		CardinalTicks.Add(Tick);
	}
}

void USFCompassHUDWidget::TryResolveSubsystem()
{
	if (Subsystem.IsValid())
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		StartAutoResolveRetryTimer();
		return;
	}
	USFNarrativeWaypointSubsystem* Sub = World->GetSubsystem<USFNarrativeWaypointSubsystem>();
	if (!Sub)
	{
		StartAutoResolveRetryTimer();
		return;
	}

	Subsystem = Sub;
	Sub->OnWaypointsChanged.AddDynamic(this, &USFCompassHUDWidget::HandleWaypointsChanged);
	Sub->OnTrackedWaypointChanged.AddDynamic(this, &USFCompassHUDWidget::HandleTrackedWaypointChanged);

	// Adopt current state immediately.
	HandleWaypointsChanged();
	FSFWaypointSnapshot Tracked;
	if (Sub->GetTrackedWaypoint(Tracked))
	{
		HandleTrackedWaypointChanged(Tracked);
	}

	StopAutoResolveRetryTimer();
}

void USFCompassHUDWidget::StartAutoResolveRetryTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (World->GetTimerManager().IsTimerActive(AutoResolveRetryHandle))
	{
		return;
	}
	AutoResolveRetryElapsedSeconds = 0.0f;
	const float Period = FMath::Max(0.1f, AutoResolveRetryPeriodSeconds);
	World->GetTimerManager().SetTimer(
		AutoResolveRetryHandle,
		this,
		&USFCompassHUDWidget::TickAutoResolveRetry,
		Period,
		/*bLoop=*/true);
}

void USFCompassHUDWidget::StopAutoResolveRetryTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoResolveRetryHandle);
	}
	AutoResolveRetryElapsedSeconds = 0.0f;
}

void USFCompassHUDWidget::TickAutoResolveRetry()
{
	if (Subsystem.IsValid())
	{
		StopAutoResolveRetryTimer();
		return;
	}
	TryResolveSubsystem();
	if (Subsystem.IsValid())
	{
		StopAutoResolveRetryTimer();
		return;
	}

	AutoResolveRetryElapsedSeconds += FMath::Max(0.1f, AutoResolveRetryPeriodSeconds);
	if (AutoResolveRetryElapsedSeconds >= MaxAutoResolveRetrySeconds)
	{
		UE_LOG(LogSFCompassHUD, Warning,
			TEXT("[CompassHUD] Gave up resolving USFNarrativeWaypointSubsystem after %.1fs."),
			AutoResolveRetryElapsedSeconds);
		StopAutoResolveRetryTimer();
	}
}

void USFCompassHUDWidget::HandleWaypointsChanged()
{
	USFNarrativeWaypointSubsystem* Sub = Subsystem.Get();
	if (!Sub)
	{
		return;
	}
	CachedActiveWaypoints = Sub->GetActiveWaypoints();
	RebuildCompassMarkers();
	BP_OnCompassMarkersChanged(CompassMarkers);
}

void USFCompassHUDWidget::HandleTrackedWaypointChanged(const FSFWaypointSnapshot& InTrackedWaypoint)
{
	CurrentTrackedWaypoint = InTrackedWaypoint;
	bHasTrackedWaypoint = InTrackedWaypoint.IsValidSnapshot();
	BP_OnTrackedWaypointChanged(CurrentTrackedWaypoint, bHasTrackedWaypoint);
}

void USFCompassHUDWidget::RebuildCompassMarkers()
{
	CompassMarkers.Reset(CachedActiveWaypoints.Num());
	for (const FSFWaypointSnapshot& Snapshot : CachedActiveWaypoints)
	{
		if (!Snapshot.IsValidSnapshot())
		{
			continue;
		}
		if (!Snapshot.bRevealOnCompass)
		{
			continue;
		}
		FSFCompassMarker Marker;
		Marker.Waypoint = Snapshot;
		Marker.StripCoordinate = 0.0f;
		Marker.AngularOffsetDegrees = 0.0f;
		Marker.bIsBehind = false;
		Marker.DistanceMeters = 0.0f;
		Marker.bIsTracked = Snapshot.bIsTracked;
		CompassMarkers.Add(Marker);
	}
	// Strip coordinates are recomputed every tick; no need to do it here.
}

void USFCompassHUDWidget::RefreshPlayerYaw()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	float NewYaw = CachedPlayerYaw;
	if (bUseCameraRotation && PC->PlayerCameraManager)
	{
		const FRotator CamRot = PC->PlayerCameraManager->GetCameraRotation();
		NewYaw = CamRot.Yaw;
	}
	else
	{
		const FRotator CtrlRot = PC->GetControlRotation();
		NewYaw = CtrlRot.Yaw;
	}

	CachedPlayerYaw = WrapYawPositive(NewYaw);
}

void USFCompassHUDWidget::RecomputeStripCoordinates()
{
	// Cardinal ticks
	for (FSFCompassCardinalTick& Tick : CardinalTicks)
	{
		const float Signed = WrapSignedDegrees(Tick.YawDegrees - CachedPlayerYaw);
		Tick.AngularOffsetDegrees = Signed;
		Tick.StripCoordinate = AngleToStripCoordinate(Signed);
	}

	// Waypoint markers — needs the player world location for distance + bearing.
	APlayerController* PC = GetOwningPlayer();
	FVector PlayerLoc = FVector::ZeroVector;
	bool bHasPlayerLoc = false;
	if (PC)
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			PlayerLoc = Pawn->GetActorLocation();
			bHasPlayerLoc = true;
		}
		else if (PC->PlayerCameraManager)
		{
			PlayerLoc = PC->PlayerCameraManager->GetCameraLocation();
			bHasPlayerLoc = true;
		}
	}

	for (int32 Index = CompassMarkers.Num() - 1; Index >= 0; --Index)
	{
		FSFCompassMarker& Marker = CompassMarkers[Index];

		float Bearing = 0.0f;
		if (bHasPlayerLoc)
		{
			const FVector Delta = Marker.Waypoint.WorldLocation - PlayerLoc;
			Marker.DistanceMeters = static_cast<float>(Delta.Size2D() * 0.01); // cm -> m
			// Bearing yaw of the target relative to world (atan2(Y, X)).
			const float BearingYaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
			Bearing = WrapSignedDegrees(BearingYaw - CachedPlayerYaw);
		}
		else
		{
			Marker.DistanceMeters = 0.0f;
			Bearing = 0.0f;
		}

		Marker.AngularOffsetDegrees = Bearing;
		Marker.bIsBehind = FMath::Abs(Bearing) > 90.0f;
		Marker.StripCoordinate = AngleToStripCoordinate(Bearing);

		if (FMath::Abs(Marker.StripCoordinate) > MaxStripCoordinateAbs)
		{
			// Out of strip range — drop from this frame's draw list.
			// We keep the underlying snapshot in CachedActiveWaypoints,
			// so it'll reappear automatically when it rotates back in.
			CompassMarkers.RemoveAtSwap(Index);
		}
	}
}

float USFCompassHUDWidget::AngleToStripCoordinate(float SignedAngleDegrees) const
{
	const float SafeHalfFov = FMath::Max(1.0f, CompassHalfFovDegrees);
	return SignedAngleDegrees / SafeHalfFov;
}

bool USFCompassHUDWidget::GetStripCoordinateForWorldLocation(
	FVector WorldLocation,
	float& OutStripCoordinate,
	float& OutAngularOffsetDegrees,
	bool& OutbIsBehind) const
{
	OutStripCoordinate = 0.0f;
	OutAngularOffsetDegrees = 0.0f;
	OutbIsBehind = false;

	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return false;
	}

	FVector PlayerLoc = FVector::ZeroVector;
	bool bHasPlayerLoc = false;
	if (APawn* Pawn = PC->GetPawn())
	{
		PlayerLoc = Pawn->GetActorLocation();
		bHasPlayerLoc = true;
	}
	else if (PC->PlayerCameraManager)
	{
		PlayerLoc = PC->PlayerCameraManager->GetCameraLocation();
		bHasPlayerLoc = true;
	}
	if (!bHasPlayerLoc)
	{
		return false;
	}

	const FVector Delta = WorldLocation - PlayerLoc;
	const float BearingYaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
	const float Signed = WrapSignedDegrees(BearingYaw - CachedPlayerYaw);
	OutAngularOffsetDegrees = Signed;
	OutbIsBehind = FMath::Abs(Signed) > 90.0f;
	OutStripCoordinate = AngleToStripCoordinate(Signed);
	return true;
}

bool USFCompassHUDWidget::GetStripCoordinateForYaw(
	float TargetYawDegrees,
	float& OutStripCoordinate,
	float& OutAngularOffsetDegrees) const
{
	const float Signed = WrapSignedDegrees(TargetYawDegrees - CachedPlayerYaw);
	OutAngularOffsetDegrees = Signed;
	OutStripCoordinate = AngleToStripCoordinate(Signed);
	return true;
}

float USFCompassHUDWidget::WrapYawPositive(float Yaw)
{
	float Wrapped = FMath::Fmod(Yaw, 360.0f);
	if (Wrapped < 0.0f)
	{
		Wrapped += 360.0f;
	}
	return Wrapped;
}

float USFCompassHUDWidget::WrapSignedDegrees(float Angle)
{
	float Wrapped = FMath::Fmod(Angle + 180.0f, 360.0f);
	if (Wrapped < 0.0f)
	{
		Wrapped += 360.0f;
	}
	return Wrapped - 180.0f;
}
