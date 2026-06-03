#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "UI/SFUserWidgetBase.h"
#include "SFDeathScreenWidget.generated.h"

/**
 * "You have fallen" screen, in the spirit of Destiny's death overlay.
 *
 * C++ owns the state machine and the player-facing decision (restart from
 * checkpoint vs. respawn near death location), as well as the respawn lockout
 * countdown. The visual treatment -- the black-vignette fade-in, the wipe-
 * sweep title, the "Guardian Down"-style copy, the countdown ring -- is
 * authored in WBP_DeathScreen by the designer, who binds the named BP events
 * below.
 *
 * Flow:
 *   1. Controller calls InitializeDeathScreen, which fires OnDeathScreenShown
 *      and starts the RespawnLockoutSeconds countdown.
 *   2. While the countdown is ticking, OnCountdownTick(SecondsRemaining) is
 *      fired every frame the value changes; RespawnKeyHintLabel is empty.
 *   3. When the countdown completes, OnCountdownFinished() fires, the BP can
 *      reveal the "Press {key} to respawn" prompt (key surfaces via
 *      RespawnKeyHintLabel), and the widget begins accepting RespawnKey.
 *   4. The player presses RespawnKey (or BP code calls RequestRespawn()),
 *      OnRespawnRequested() fires for the BP's exit animation, and the
 *      controller is asked to perform the respawn.
 *
 * Wiring on the designer side:
 *   - Override OnDeathScreenShown for the entry animation.
 *   - Override OnCountdownTick / OnCountdownFinished to drive the ring + prompt.
 *   - Override OnRespawnRequested for the exit animation.
 *   - Optionally bind a button to RequestRespawn() -- it's no-op until the
 *     countdown finishes, mirroring Destiny's gated CTA.
 */
UCLASS()
class SIGNALFORGERPG_API USFDeathScreenWidget : public USFUserWidgetBase
{
	GENERATED_BODY()

public:
	/**
	 * Called by the player controller after the widget is added to viewport.
	 * Populates the death-mode state, kicks off the lockout countdown, and
	 * fires the BP entry hook.
	 */
	UFUNCTION(BlueprintCallable, Category = "Death Screen")
	void InitializeDeathScreen(bool bInIsDarkZoneDeath, const FText& InZoneOrCheckpointName);

	/**
	 * Designer wires the respawn/restart button (or auto-respawn logic) to
	 * this. Calls before the countdown finishes are ignored.
	 */
	UFUNCTION(BlueprintCallable, Category = "Death Screen")
	void RequestRespawn();

	/** True once the lockout countdown has elapsed. */
	UFUNCTION(BlueprintPure, Category = "Death Screen")
	bool IsRespawnReady() const { return bIsRespawnReady; }

	/** Current seconds remaining on the lockout (clamped to [0, RespawnLockoutSeconds]). */
	UFUNCTION(BlueprintPure, Category = "Death Screen")
	float GetSecondsRemaining() const { return SecondsRemaining; }

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

	/**
	 * "Press [Space] to respawn"-style hint surfaced once the countdown ends.
	 * Empty while the countdown is running. Localized via the RespawnKey's
	 * display name.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Death Screen")
	FText RespawnKeyHintLabel;

	/**
	 * How long the player has to sit with the consequences before they can
	 * press to respawn. Destiny's beat lands at ~10s.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death Screen")
	float RespawnLockoutSeconds = 10.0f;

	/**
	 * Key the player presses to respawn once the countdown is done. Defaults
	 * to Space; designers can swap it on the WBP defaults if they prefer
	 * something else. Held keys count as one press (we trigger on KeyDown).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death Screen")
	FKey RespawnKey = EKeys::SpaceBar;

	/** Override in BP to drive the entry animation / sound / camera shake. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Death Screen")
	void OnDeathScreenShown();

	/**
	 * Fired every time the lockout countdown changes (driven by NativeTick).
	 * Use this to update a ring/bar/text.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Death Screen")
	void OnCountdownTick(float InSecondsRemaining);

	/** Fired exactly once when the lockout countdown completes. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Death Screen")
	void OnCountdownFinished();

	/** Override in BP to play the exit animation before the controller respawns. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Death Screen")
	void OnRespawnRequested();

protected:
	//~ UUserWidget interface
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	//~ End UUserWidget interface

private:
	/** Refreshes RespawnKeyHintLabel using RespawnKey's localized display name. */
	void RebuildRespawnKeyHintLabel();

	/** True once SecondsRemaining has reached zero. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Death Screen", meta = (AllowPrivateAccess = "true"))
	bool bIsRespawnReady = false;

	/** Countdown state, ticked down in NativeTick. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Death Screen", meta = (AllowPrivateAccess = "true"))
	float SecondsRemaining = 0.0f;
};
