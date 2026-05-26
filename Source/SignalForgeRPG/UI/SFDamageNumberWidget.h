// Copyright Fallen Signal Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/SFUserWidgetBase.h"
#include "SFDamageNumberWidget.generated.h"

/**
 * Single damage-number floater. Spawned by USFDamageNumberSubsystem when the
 * local player lands damage. Auto-animates a Destiny-style pop: scale burst
 * in the first beat, rises + drifts sideways, fades out near the end of
 * Lifespan, then RemoveFromParent()s itself.
 *
 * Animation runs entirely in NativeTick driving the widget's RenderTransform
 * and ColorAndOpacity, so designers don't need to author UMG animations.
 * BP can still bind OnInitialized() to layer extra punch (sounds, particles).
 */
UCLASS()
class SIGNALFORGERPG_API USFDamageNumberWidget : public USFUserWidgetBase
{
	GENERATED_BODY()

public:
	/**
	 * Configure this floater. Call once immediately after CreateWidget,
	 * before AddToViewport. ScreenPos is the viewport-space anchor; the
	 * subsystem sets the widget's slot position to ScreenPos.
	 */
	UFUNCTION(BlueprintCallable, Category = "SF|DamageNumber")
	void InitDamageNumber(float InDamage, bool bInIsCrit, bool bInIsWeakpoint, FVector2D InScreenPos);

	/** Read-only accessors for BP designers laying out the widget. */
	UFUNCTION(BlueprintPure, Category = "SF|DamageNumber") float GetDamage() const { return Damage; }
	UFUNCTION(BlueprintPure, Category = "SF|DamageNumber") bool  IsCrit() const { return bIsCrit; }
	UFUNCTION(BlueprintPure, Category = "SF|DamageNumber") bool  IsWeakpoint() const { return bIsWeakpoint; }
	UFUNCTION(BlueprintPure, Category = "SF|DamageNumber") FLinearColor GetResolvedColor() const;
	UFUNCTION(BlueprintPure, Category = "SF|DamageNumber") FText GetTagLabel() const;
	UFUNCTION(BlueprintPure, Category = "SF|DamageNumber") FVector2D GetScreenPos() const { return ScreenPos; }

	/**
	 * Designer hook fired right after InitDamageNumber finishes wiring state.
	 * Use this in BP to bind the damage text, set the color on a TextBlock,
	 * play a sound, spawn a Niagara burst, etc. Named distinctly from
	 * UUserWidget::OnInitialized() to avoid colliding with that engine hook.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "SF|DamageNumber")
	void OnDamageNumberInitialized();

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Total time the floater is visible before being removed (seconds). */
	UPROPERTY(EditDefaultsOnly, Category = "SF|DamageNumber|Animation", meta = (ClampMin = "0.1"))
	float Lifespan = 1.0f;

	/** Vertical rise in pixels over Lifespan. */
	UPROPERTY(EditDefaultsOnly, Category = "SF|DamageNumber|Animation")
	float RiseDistance = 120.0f;

	/** Horizontal jitter range in pixels (random per-instance, -Jitter..+Jitter). */
	UPROPERTY(EditDefaultsOnly, Category = "SF|DamageNumber|Animation")
	float SideJitter = 40.0f;

	/** Scale punch at spawn (eases back to 1.0 within ScalePunchTime). */
	UPROPERTY(EditDefaultsOnly, Category = "SF|DamageNumber|Animation", meta = (ClampMin = "1.0"))
	float ScalePunch = 1.5f;

	/** Time in seconds for the scale punch to ease back to 1.0. */
	UPROPERTY(EditDefaultsOnly, Category = "SF|DamageNumber|Animation", meta = (ClampMin = "0.01"))
	float ScalePunchTime = 0.12f;

	/** Crit hits use this scale punch instead (snappier, bigger). */
	UPROPERTY(EditDefaultsOnly, Category = "SF|DamageNumber|Animation", meta = (ClampMin = "1.0"))
	float CritScalePunch = 1.9f;

	/** Fraction of Lifespan spent fading out at the end (0..1). */
	UPROPERTY(EditDefaultsOnly, Category = "SF|DamageNumber|Animation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FadeOutFraction = 0.35f;

	/** Color used for normal (non-crit, non-weakpoint) hits. */
	UPROPERTY(EditDefaultsOnly, Category = "SF|DamageNumber|Colors")
	FLinearColor NormalColor = FLinearColor::White;

	/** Color used when the hit was a crit roll. */
	UPROPERTY(EditDefaultsOnly, Category = "SF|DamageNumber|Colors")
	FLinearColor CritColor = FLinearColor(1.0f, 0.75f, 0.1f, 1.0f);

	/** Color used when the hit landed on a weakpoint (overrides crit color if both). */
	UPROPERTY(EditDefaultsOnly, Category = "SF|DamageNumber|Colors")
	FLinearColor WeakpointColor = FLinearColor(1.0f, 0.2f, 0.2f, 1.0f);

	/** Tag chip text shown when bIsCrit (and not weakpoint). */
	UPROPERTY(EditDefaultsOnly, Category = "SF|DamageNumber|Labels")
	FText CritTagLabel = NSLOCTEXT("SF.DamageNumber", "CritTag", "CRIT");

	/** Tag chip text shown when bIsWeakpoint. */
	UPROPERTY(EditDefaultsOnly, Category = "SF|DamageNumber|Labels")
	FText WeakpointTagLabel = NSLOCTEXT("SF.DamageNumber", "WeakTag", "WEAK");

private:
	/** Total damage dealt by this hit (post-mitigation). */
	UPROPERTY(VisibleAnywhere, Category = "SF|DamageNumber|State")
	float Damage = 0.0f;

	UPROPERTY(VisibleAnywhere, Category = "SF|DamageNumber|State")
	bool bIsCrit = false;

	UPROPERTY(VisibleAnywhere, Category = "SF|DamageNumber|State")
	bool bIsWeakpoint = false;

	/** Viewport-space anchor for the floater (set by subsystem on spawn). */
	UPROPERTY(VisibleAnywhere, Category = "SF|DamageNumber|State")
	FVector2D ScreenPos = FVector2D::ZeroVector;

	/** Random horizontal drift target chosen once at Init, in pixels. */
	float SideDrift = 0.0f;

	/** Seconds elapsed since InitDamageNumber. */
	float Elapsed = 0.0f;

	/** Set true once InitDamageNumber has run so NativeTick can safely animate. */
	bool bInitialized = false;
};
