// Copyright Fallen Signal Studios. All Rights Reserved.

#include "UI/SFDamageNumberSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Input/SFPlayerController.h"
#include "UI/SFDamageNumberWidget.h"

void USFDamageNumberSubsystem::ShowDamageNumber(float Damage, bool bIsCrit, bool bIsWeakpoint, FVector WorldLocation)
{
	ULocalPlayer* LP = GetLocalPlayer();
	if (!LP)
	{
		return;
	}

	ASFPlayerController* PC = Cast<ASFPlayerController>(LP->GetPlayerController(LP->GetWorld()));
	if (!PC)
	{
		return;
	}

	if (!PC->DamageNumberWidgetClass)
	{
		// No floater BP wired up yet — nothing to spawn. Silent so we don't
		// log-spam every hit before designers hook up the class.
		return;
	}

	// Project the impact point to screen space. Returns false when the point
	// is off-screen or behind the camera; in that case skip the floater
	// rather than clamp to a screen edge (Destiny does the same).
	FVector2D ScreenPos = FVector2D::ZeroVector;
	const bool bProjected = PC->ProjectWorldLocationToScreen(WorldLocation, ScreenPos, /*bPlayerViewportRelative=*/ true);
	if (!bProjected)
	{
		return;
	}

	USFDamageNumberWidget* Widget = CreateWidget<USFDamageNumberWidget>(PC, PC->DamageNumberWidgetClass);
	if (!Widget)
	{
		return;
	}

	// Add to viewport first so it gets a CanvasPanelSlot we can position via
	// SetPositionInViewport, which respects DPI scaling. Then call Init so
	// the widget can snap its own slot and play OnInitialized.
	Widget->AddToViewport(WidgetZOrder);
	// ProjectWorldLocationToScreen returns absolute pixel coords; passing
	// bRemoveDPIScale=true lets SetPositionInViewport convert to layout-space
	// so the widget sits exactly under the projected point on high-DPI monitors.
	Widget->SetPositionInViewport(ScreenPos, /*bRemoveDPIScale=*/ true);
	Widget->InitDamageNumber(Damage, bIsCrit, bIsWeakpoint, ScreenPos);
}
