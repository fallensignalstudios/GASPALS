#include "UI/SFDeathScreenWidget.h"

#include "Input/SFPlayerController.h"

#define LOCTEXT_NAMESPACE "SFDeathScreen"

void USFDeathScreenWidget::InitializeDeathScreen(bool bInIsDarkZoneDeath, const FText& InZoneOrCheckpointName)
{
	bIsDarkZoneDeath = bInIsDarkZoneDeath;
	ZoneOrCheckpointName = InZoneOrCheckpointName;

	// Copy is C++-authoritative so the same wording shows everywhere; the
	// designer just renders it. Tone target: Destiny's "Guardian Down" --
	// short, weighty, name-the-place.
	// Subtitle changes based on stakes: dark zones cost progress (back to
	// checkpoint), everywhere else it's "dust yourself off."
	if (bIsDarkZoneDeath)
	{
		DeathTitle = LOCTEXT("DeathTitle_DarkZone", "YOU HAVE FALLEN");

		if (!ZoneOrCheckpointName.IsEmpty())
		{
			DeathSubtitle = FText::Format(
				LOCTEXT("DeathSubtitle_DarkZoneNamed", "The {0} does not forgive. Restart from your last checkpoint."),
				ZoneOrCheckpointName);
		}
		else
		{
			DeathSubtitle = LOCTEXT("DeathSubtitle_DarkZone", "This place does not forgive. Restart from your last checkpoint.");
		}

		RespawnActionLabel = LOCTEXT("Action_Restart", "Restart from Checkpoint");
	}
	else
	{
		DeathTitle = LOCTEXT("DeathTitle_Default", "YOU HAVE FALLEN");
		DeathSubtitle = LOCTEXT("DeathSubtitle_Default", "Pick yourself up. The mission is not over.");
		RespawnActionLabel = LOCTEXT("Action_Respawn", "Respawn");
	}

	// Reset countdown state on every show. Pressing-to-respawn is gated until
	// SecondsRemaining hits zero -- mirrors Destiny's "sit in the death cam
	// for a beat" pacing.
	bIsRespawnReady = false;
	SecondsRemaining = FMath::Max(0.0f, RespawnLockoutSeconds);
	RespawnKeyHintLabel = FText::GetEmpty();

	// Make sure the widget itself receives key events while focused. The
	// controller puts us in UI-only mode and focuses us, so NativeOnKeyDown
	// will fire for the respawn key once the countdown finishes.
	SetIsFocusable(true);

	OnDeathScreenShown();

	// Fire the first tick immediately so the BP can render the full 10
	// rather than waiting a frame.
	OnCountdownTick(SecondsRemaining);

	if (SecondsRemaining <= 0.0f)
	{
		// Zero-lockout configuration: skip straight to ready.
		bIsRespawnReady = true;
		RebuildRespawnKeyHintLabel();
		OnCountdownFinished();
	}
}

void USFDeathScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bIsRespawnReady)
	{
		return;
	}

	if (SecondsRemaining <= 0.0f)
	{
		return;
	}

	const float Previous = SecondsRemaining;
	SecondsRemaining = FMath::Max(0.0f, SecondsRemaining - InDeltaTime);

	if (!FMath::IsNearlyEqual(Previous, SecondsRemaining))
	{
		OnCountdownTick(SecondsRemaining);
	}

	if (SecondsRemaining <= 0.0f && !bIsRespawnReady)
	{
		bIsRespawnReady = true;
		RebuildRespawnKeyHintLabel();
		OnCountdownFinished();
	}
}

FReply USFDeathScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Swallow input until the lockout elapses -- the player should sit with
	// the death for the full beat, like Destiny's wipe.
	if (!bIsRespawnReady)
	{
		return FReply::Handled();
	}

	if (InKeyEvent.GetKey() == RespawnKey)
	{
		RequestRespawn();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void USFDeathScreenWidget::RequestRespawn()
{
	// Gate: pre-countdown clicks/key presses are no-ops. This lets designers
	// safely wire a button to RequestRespawn that's always visible -- it just
	// won't do anything until OnCountdownFinished has fired.
	if (!bIsRespawnReady)
	{
		return;
	}

	// Fire the BP exit hook first so the designer can play a fade-out
	// animation; the controller call below tears the widget down anyway,
	// but at least one tick of the exit anim usually plays before the
	// world reset. If you need the full anim, gate the controller call
	// behind a BP-side animation-finished event.
	OnRespawnRequested();

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (ASFPlayerController* SFPC = Cast<ASFPlayerController>(PC))
		{
			SFPC->RespawnFromDeathScreen(bIsDarkZoneDeath);
		}
	}
}

void USFDeathScreenWidget::RebuildRespawnKeyHintLabel()
{
	const FText KeyDisplay = RespawnKey.GetDisplayName();

	if (bIsDarkZoneDeath)
	{
		RespawnKeyHintLabel = FText::Format(
			LOCTEXT("RespawnHint_DarkZone", "Press [{0}] to restart from checkpoint"),
			KeyDisplay);
	}
	else
	{
		RespawnKeyHintLabel = FText::Format(
			LOCTEXT("RespawnHint_Default", "Press [{0}] to respawn"),
			KeyDisplay);
	}
}

#undef LOCTEXT_NAMESPACE
