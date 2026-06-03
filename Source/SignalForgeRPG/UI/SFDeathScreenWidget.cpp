#include "UI/SFDeathScreenWidget.h"

#include "Input/SFPlayerController.h"

#define LOCTEXT_NAMESPACE "SFDeathScreen"

void USFDeathScreenWidget::InitializeDeathScreen(bool bInIsDarkZoneDeath, const FText& InZoneOrCheckpointName)
{
	bIsDarkZoneDeath = bInIsDarkZoneDeath;
	ZoneOrCheckpointName = InZoneOrCheckpointName;

	// Copy is C++-authoritative so the same wording shows everywhere; the
	// designer just renders it. Tone target: Destiny's "Guardian Down" --
	// short, weighty, name-the-place.\n\t//
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

	OnDeathScreenShown();
}

void USFDeathScreenWidget::RequestRespawn()
{
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

#undef LOCTEXT_NAMESPACE
