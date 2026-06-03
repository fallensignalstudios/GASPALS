#pragma once

#include "CoreMinimal.h"
#include "UI/SFUserWidgetBase.h"
#include "SFDeathScreenWidget.generated.h"

/**
 * "You have fallen" screen, in the spirit of Destiny's death overlay.
 *
 * C++ owns the state machine and the player-facing decision (restart from
 * checkpoint vs. respawn near death location). The visual treatment -- the
 * black-vignette fade-in, the wipe-sweep title, the "Guardian Down"-style
 * copy, the call-to-action button -- is authored in WBP_DeathScreen by the
 * designer, who binds the named BP events below.
 *
 * Wiring on the designer side:
 *   - Override OnDeathScreenShown to play the entry animation.
 *   - Bind a button OnClicked to RequestRespawn() -- the widget already knows
 *     which respawn mode is active based on bIsDarkZoneDeath.
 *   - Read DeathSubtitle / RespawnActionLabel in the Designer's bindings to
 *     show the right copy.
 */
UCLASS()
class SIGNALFORGERPG_API USFDeathScreenWidget : public USFUserWidgetBase
{
	GENERATED_BODY()

public:
	/**
	 * Called by the player controller after the widget is added to viewport.
	 * Populates the death-mode state and fires the BP entry hook.
	 */
	UFUNCTION(BlueprintCallable, Category = "Death Screen")
	void InitializeDeathScreen(bool bInIsDarkZoneDeath, const FText& InZoneOrCheckpointName);

	/**
	 * Designer wires the respawn/restart button to this. Forwards the request
	 * to the owning player controller, which calls back into the game mode.
	 */
	UFUNCTION(BlueprintCallable, Category = "Death Screen")
	void RequestRespawn();

	/** True if the player died inside a dark zone (forces checkpoint restart). */
	UPROPERTY(BlueprintReadOnly, Category = "Death Screen")
	bool bIsDarkZoneDeath = false;

	/** Friendly zone or checkpoint name to surface on the screen. */
	UPROPERTY(BlueprintReadOnly, Category = "Death Screen")
	FText ZoneOrCheckpointName;

	/** Designer-bindable copy. C++ picks the line based on dark-zone status. */
	UPROPERTY(BlueprintReadOnly, Category = "Death Screen")
	FText DeathTitle;

	UPROPERTY(BlueprintReadOnly, Category = "Death Screen")
	FText DeathSubtitle;

	UPROPERTY(BlueprintReadOnly, Category = "Death Screen")
	FText RespawnActionLabel;

	/** Override in BP to drive the entry animation / sound / camera shake. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Death Screen")
	void OnDeathScreenShown();

	/** Override in BP to play the exit animation before the controller respawns. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Death Screen")
	void OnRespawnRequested();
};
