// SFWeaponHolderInterface.h
//
// ISFWeaponHolder: contract for any actor that carries a weapon and an ammo
// reserve. Lets the GAS abilities, AI behavior tree decorators, and combat
// helpers operate on "the thing holding the gun" without casting to
// ASFCharacterBase.
//
// Design notes:
//   - The interface intentionally exposes the existing component types
//     (USFEquipmentComponent, USFAmmoReserveComponent) rather than
//     re-exporting every weapon-system getter through the interface. The
//     equipment surface is large and stable; mirroring it method-by-method
//     would be churn for no architectural benefit. Migration becomes a
//     one-line change at the call site: Cast<ASFCharacterBase>(Avatar) ->
//     Execute_GetEquipmentComponent(Avatar), then the existing Equipment->
//     calls work unchanged.
//   - GetActiveMuzzleTransform is a convenience for trace origin lookups
//     (the WeaponFire / WeaponBeam abilities both reach into the equipped
//     weapon actor's mesh socket the same way). Routing it through the
//     interface lets non-character holders (turrets, mounted guns) supply
//     their own muzzle transform without subclassing ASFWeaponActor.

#pragma once

#include "UObject/Interface.h"
#include "SFWeaponHolderInterface.generated.h"

class USFEquipmentComponent;
class USFAmmoReserveComponent;

UINTERFACE(MinimalAPI, Blueprintable)
class USFWeaponHolderInterface : public UInterface
{
	GENERATED_BODY()
};

class SIGNALFORGERPG_API ISFWeaponHolderInterface
{
	GENERATED_BODY()

public:
	/** Equipment component that owns slot state, the spawned weapon actor, and the active FSFWeaponInstanceData. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Combat|WeaponHolder")
	USFEquipmentComponent* GetEquipmentComponent() const;

	/** Ammo reserve component (per-character ammo pool by USFAmmoType, separate from per-clip state on the weapon instance). */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Combat|WeaponHolder")
	USFAmmoReserveComponent* GetAmmoReserveComponent() const;

	/** Muzzle world transform of the currently equipped weapon actor. Returns false (and identity transform) if no weapon is equipped or the muzzle socket is missing. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Combat|WeaponHolder")
	bool GetActiveMuzzleTransform(FTransform& OutTransform) const;
};
