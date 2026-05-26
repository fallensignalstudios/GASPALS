// Copyright Fallen Signal Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "SFDamageNumberSubsystem.generated.h"

class USFDamageNumberWidget;

/**
 * Per-local-player spawner for Destiny-style damage-number floaters.
 *
 * Sits on the LocalPlayer so it's naturally local-only — damage numbers only
 * appear for the player who dealt the hit, even in multiplayer. Consumers
 * (typically USFAttributeSetBase::PostGameplayEffectExecute on the local
 * server/client) call ShowDamageNumber with a world-space hit location.
 *
 * The actual widget class comes from the owning ASFPlayerController's
 * DamageNumberWidgetClass property, so designers can swap the floater BP
 * without touching C++.
 */
UCLASS()
class SIGNALFORGERPG_API USFDamageNumberSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * Project WorldLocation to the local player's viewport, spawn a damage
	 * number widget, and add it to the viewport at that position. Safe to
	 * call every hit — does nothing if no widget class is configured or the
	 * point projects behind the camera.
	 */
	UFUNCTION(BlueprintCallable, Category = "SF|DamageNumber")
	void ShowDamageNumber(float Damage, bool bIsCrit, bool bIsWeakpoint, FVector WorldLocation);

	/** Z-order applied to spawned floaters so they sit above standard HUD. */
	UPROPERTY(EditDefaultsOnly, Category = "SF|DamageNumber")
	int32 WidgetZOrder = 10;
};
