// Copyright Fallen Signal Studios LLC.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "SFAmmoType.generated.h"

class UTexture2D;

/**
 * Data asset describing a single ammunition type that can be carried by a
 * character and consumed by weapons. A weapon's FSFWeaponAmmoConfig points at
 * one of these; the AmmoReserveComponent on a character tracks how many rounds
 * of each type they're carrying.
 *
 * Multiple weapons can share an AmmoType (e.g. all 5.56 rifles draw from the
 * same pool). The MaxCarried value is the upper cap a single character can
 * hold, and is typically used by pickup/inventory UI.
 */
UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFAmmoType : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Display name shown in HUD ammo readouts and pickup tooltips. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo")
	FText DisplayName;

	/** Optional gameplay tag (e.g. Ammo.Rifle.556, Ammo.Energy.Plasma). Useful
	 *  for matching pickup drops or balancing categories. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo")
	FGameplayTag AmmoTag;

	/** Maximum rounds a single character can carry of this type. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = "1"))
	int32 MaxCarried = 240;

	/** Starting rounds granted to a character on first use of a weapon that
	 *  consumes this ammo (or when explicitly given). Used as a default; the
	 *  AmmoReserveComponent can be set up differently per character. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = "0"))
	int32 DefaultStartingAmount = 90;

	/** Icon used in HUD ammo widgets and pickup UI. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo|UI")
	TSoftObjectPtr<UTexture2D> Icon;
};
