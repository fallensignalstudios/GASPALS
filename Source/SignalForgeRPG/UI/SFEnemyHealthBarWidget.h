#pragma once

#include "CoreMinimal.h"
#include "UI/SFUserWidgetBase.h"
#include "SFEnemyHealthBarWidget.generated.h"

class ASFCharacterBase;
class UProgressBar;
class UTextBlock;

/**
 * USFEnemyHealthBarWidget
 *
 * Destiny / Jedi-Survivor-style floating combat info plate above an NPC's head.
 * Shows the NPC's name, level, shields bar, and health bar. Hidden by default
 * and fades in the first time the NPC takes damage. Auto-hides after a few
 * seconds of no incoming damage. Permanently hides on death.
 *
 * Bind in WBP_EnemyHealthBar:
 *   - ProgressBar named "ShieldsBar"
 *   - ProgressBar named "HealthBar"
 *   - TextBlock named "NameText"
 *   - TextBlock named "LevelText"
 *
 * The widget targets the owning UWidgetComponent's owner pawn for data. The
 * native parent does all binding work; the WBP just supplies the visuals and
 * optional fade-in/out animations (assigned by name FadeInAnim / FadeOutAnim).
 */
UCLASS()
class SIGNALFORGERPG_API USFEnemyHealthBarWidget : public USFUserWidgetBase
{
	GENERATED_BODY()

public:
	/** Re-bind to a specific target. Useful for pooled widgets / debug. */
	UFUNCTION(BlueprintCallable, Category = "Enemy HUD")
	void InitializeForCharacter(ASFCharacterBase* InTarget);

	/** Manually request the bar to show (e.g. on aim-at). Resets the hide timer. */
	UFUNCTION(BlueprintCallable, Category = "Enemy HUD")
	void ShowBarTransient();

protected:
	//~ Begin UUserWidget
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float DeltaTime) override;
	//~ End UUserWidget

	/** Auto-hide after this many seconds without an incoming damage event. */
	UPROPERTY(EditDefaultsOnly, Category = "Enemy HUD", meta = (ClampMin = "0.5"))
	float HideAfterSecondsIdle = 4.0f;

	/** Interp speed for the displayed health / shield fill values \u2014 higher = snappier. */
	UPROPERTY(EditDefaultsOnly, Category = "Enemy HUD", meta = (ClampMin = "0.1"))
	float BarInterpSpeed = 8.0f;

	/** ProgressBar that fills with shields (top bar). Must match the WBP widget name. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Enemy HUD")
	TObjectPtr<UProgressBar> ShieldsBar;

	/** ProgressBar that fills with health (bottom bar). Must match the WBP widget name. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Enemy HUD")
	TObjectPtr<UProgressBar> HealthBar;

	/** NPC display name. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Enemy HUD")
	TObjectPtr<UTextBlock> NameText;

	/** "Lv 4" / "Lv 18". */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Enemy HUD")
	TObjectPtr<UTextBlock> LevelText;

	/**
	 * Latest known target percentages \u2014 driven by GAS delegates. The native tick
	 * smoothly interps the visible ProgressBar percent toward these values so a
	 * burst hit "bleeds" the bar down rather than snapping.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Enemy HUD")
	float TargetHealthPct = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy HUD")
	float TargetShieldsPct = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy HUD")
	float DisplayedHealthPct = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy HUD")
	float DisplayedShieldsPct = 1.0f;

	/** Optional fade animations authored in the WBP. */
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<class UWidgetAnimation> FadeInAnim;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<class UWidgetAnimation> FadeOutAnim;

private:
	/** Pawn we're reading attribute values from. Weak so we never extend its lifetime. */
	TWeakObjectPtr<ASFCharacterBase> TargetCharacter;

	/** True between the first damage event and the auto-hide timer firing. */
	bool bBarVisible = false;

	/** Cleared every time we receive a damage delegate; advanced by tick. */
	float SecondsSinceLastDamage = 0.0f;

	/** Suppress the very first delegate fire (initial broadcast on bind) from triggering visibility. */
	bool bHasReceivedInitialBroadcast = false;

	void BindToCharacterDelegates();
	void UnbindFromCharacterDelegates();
	void RefreshIdentityText();

	UFUNCTION()
	void HandleHealthChanged(float NewValue, float MaxValue);

	UFUNCTION()
	void HandleShieldsChanged(float NewValue, float MaxValue);

	UFUNCTION()
	void HandleCharacterDied(ASFCharacterBase* Victim, ASFCharacterBase* Killer);

	void RequestShow();
	void RequestHide();
};
