// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "UI/SFWaypointHUDWidget.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Narrative/SFNarrativeWaypointSubsystem.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogSFWaypointHUD, Log, All);

void USFWaypointHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();
    TryResolveSubsystem();
}

void USFWaypointHUDWidget::NativeDestruct()
{
    StopAutoResolveRetryTimer();

    if (USFNarrativeWaypointSubsystem* Sub = Subsystem.Get())
    {
        Sub->OnWaypointsChanged.RemoveDynamic(this, &USFWaypointHUDWidget::HandleWaypointsChanged);
        Sub->OnTrackedWaypointChanged.RemoveDynamic(this, &USFWaypointHUDWidget::HandleTrackedWaypointChanged);
    }
    Subsystem = nullptr;

    Super::NativeDestruct();
}

void USFWaypointHUDWidget::TryResolveSubsystem()
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
    Sub->OnWaypointsChanged.AddDynamic(this, &USFWaypointHUDWidget::HandleWaypointsChanged);
    Sub->OnTrackedWaypointChanged.AddDynamic(this, &USFWaypointHUDWidget::HandleTrackedWaypointChanged);

    // Adopt current state immediately.
    HandleWaypointsChanged();
    FSFWaypointSnapshot Tracked;
    if (Sub->GetTrackedWaypoint(Tracked))
    {
        HandleTrackedWaypointChanged(Tracked);
    }

    StopAutoResolveRetryTimer();
}

void USFWaypointHUDWidget::StartAutoResolveRetryTimer()
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
        &USFWaypointHUDWidget::TickAutoResolveRetry,
        Period,
        /*bLoop=*/true);
}

void USFWaypointHUDWidget::StopAutoResolveRetryTimer()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(AutoResolveRetryHandle);
    }
    AutoResolveRetryElapsedSeconds = 0.0f;
}

void USFWaypointHUDWidget::TickAutoResolveRetry()
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
        UE_LOG(LogSFWaypointHUD, Warning,
            TEXT("[WaypointHUD] Gave up resolving USFNarrativeWaypointSubsystem after %.1fs."),
            AutoResolveRetryElapsedSeconds);
        StopAutoResolveRetryTimer();
    }
}

void USFWaypointHUDWidget::HandleWaypointsChanged()
{
    USFNarrativeWaypointSubsystem* Sub = Subsystem.Get();
    if (!Sub)
    {
        return;
    }
    CurrentActiveWaypoints = Sub->GetActiveWaypoints();
    BP_OnActiveWaypointsChanged(CurrentActiveWaypoints);
}

void USFWaypointHUDWidget::HandleTrackedWaypointChanged(const FSFWaypointSnapshot& InTrackedWaypoint)
{
    CurrentTrackedWaypoint = InTrackedWaypoint;
    bHasTrackedWaypoint = InTrackedWaypoint.IsValidSnapshot();
    BP_OnTrackedWaypointChanged(CurrentTrackedWaypoint, bHasTrackedWaypoint);
}

bool USFWaypointHUDWidget::GetTrackedWaypointScreenInfo(FVector2D& OutScreenPosition, bool& OutbIsOnScreen, float& OutDistance) const
{
    OutScreenPosition = FVector2D::ZeroVector;
    OutbIsOnScreen = false;
    OutDistance = 0.0f;

    if (!bHasTrackedWaypoint || !CurrentTrackedWaypoint.IsValidSnapshot())
    {
        return false;
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        return false;
    }

    FVector CamLoc;
    FRotator CamRot;
    PC->GetPlayerViewPoint(CamLoc, CamRot);
    OutDistance = FVector::Distance(CamLoc, CurrentTrackedWaypoint.WorldLocation);

    // ProjectWorldLocationToScreen returns false when the point is behind the
    // camera; we still want a clamped screen edge for the off-screen arrow,
    // so callers can use OutbIsOnScreen to branch.
    FVector2D ScreenPos;
    const bool bProjected = PC->ProjectWorldLocationToScreen(CurrentTrackedWaypoint.WorldLocation, ScreenPos, /*bPlayerViewportRelative=*/true);
    OutScreenPosition = ScreenPos;
    OutbIsOnScreen = bProjected;
    return true;
}
