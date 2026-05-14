// Copyright Fallen Signal Studios LLC.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SFAmmoReserveComponent.generated.h"

class USFAmmoType;

/** Broadcast when a character's reserve count for an ammo type changes. */
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_ThreeParams(
	FSFOnAmmoReserveChanged,
	USFAmmoReserveComponent, OnAmmoReserveChanged,
	USFAmmoType*, AmmoType,
	int32, NewAmount,
	int32, MaxCarried);

/**
 * Tracks how many rounds of each ammo type a character is carrying.
 * Attach to ASFCharacterBase (or any actor that should carry ammo).
 *
 * Designed to be the single source of truth for "reserve" ammo — the magazine
 * count itself lives on FSFWeaponInstanceData.AmmoInClip on the equipped
 * weapon. The reload ability pulls from this reserve into the clip.
 */
UCLASS(ClassGroup = (SignalForge), meta = (BlueprintSpawnableComponent))
class SIGNALFORGERPG_API USFAmmoReserveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFAmmoReserveComponent();

	/** How many rounds of AmmoType this character is carrying. Zero if unseen. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ammo")
	int32 GetAmmoCount(USFAmmoType* AmmoType) const;

	/** True if this character can store at least one more round of this type. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ammo")
	bool HasRoomFor(USFAmmoType* AmmoType) const;

	/** Add ammo of a given type. Returns the amount actually added (clamped to
	 *  MaxCarried). */
	UFUNCTION(BlueprintCallable, Category = "Ammo")
	int32 AddAmmo(USFAmmoType* AmmoType, int32 Amount);

	/** Try to consume up to RequestedAmount of an ammo type. Returns how many
	 *  rounds were actually consumed (0 if no reserve / no AmmoType). */
	UFUNCTION(BlueprintCallable, Category = "Ammo")
	int32 ConsumeAmmo(USFAmmoType* AmmoType, int32 RequestedAmount);

	/** Hard-set a count (for save/load or cheats). Clamps to MaxCarried. */
	UFUNCTION(BlueprintCallable, Category = "Ammo")
	void SetAmmoCount(USFAmmoType* AmmoType, int32 Amount);

	/** Ensure a baseline starting amount exists for this ammo type. No-op if
	 *  the character is already carrying at least DefaultStartingAmount. */
	UFUNCTION(BlueprintCallable, Category = "Ammo")
	void EnsureStartingAmmo(USFAmmoType* AmmoType);

	/**
	 * Direct read access to the underlying reserve map. Intended for
	 * save/load, debug UI, and similar bulk iteration. Mutating the
	 * returned reference is forbidden -- go through SetAmmoCount /
	 * AddAmmo / ConsumeAmmo so OnAmmoReserveChanged stays accurate.
	 */
	const TMap<TObjectPtr<USFAmmoType>, int32>& GetReserves() const { return Reserves; }

	/** Broadcast whenever a reserve count changes. UI binds here. */
	UPROPERTY(BlueprintAssignable, Category = "Ammo")
	FSFOnAmmoReserveChanged OnAmmoReserveChanged;

protected:
	/** Per-ammo-type round counts. Persisted directly; safe to save/load by
	 *  iterating the keys. */
	UPROPERTY(VisibleAnywhere, Category = "Ammo")
	TMap<TObjectPtr<USFAmmoType>, int32> Reserves;
};
