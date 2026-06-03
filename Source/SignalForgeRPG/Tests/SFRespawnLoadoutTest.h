// Copyright Fallen Signal Studios 2026.

#pragma once

#include "CoreMinimal.h"
#include "FunctionalTest.h"
#include "Animation/AnimInstance.h"
#include "Inventory/SFSlotTypes.h"
#include "Templates/SubclassOf.h"
#include "UObject/SoftObjectPtr.h"
#include "SFRespawnLoadoutTest.generated.h"

class ASFCharacterBase;
class ASFPlayerController;
class USFWeaponData;

/**
 * Functional test that exercises the respawn-loadout pipeline end-to-end.
 *
 * Scenario:
 *  1. Wait for a player pawn to be possessed.
 *  2. Equip WeaponToEquip into SlotToEquip and confirm it became active.
 *  3. Capture the overlay mode, upper-body-overlay flag, and linked anim layer
 *     class as the "before" baseline.
 *  4. Kill the pawn through HandleDeath() and let the standard death flow run:
 *     HandlePawnDied -> SnapshotLoadoutForRespawn -> game-mode-driven
 *     RestartPlayerAtTransform -> OnPossess(NewPawn) -> RestoreLoadoutAfterRespawn
 *     -> deferred ApplyEquipmentSnapshotToFreshPawn one tick later.
 *  5. After PostRespawnWaitSeconds, find the new pawn and assert the three
 *     overlay values match the pre-death baseline.
 *
 * Place this actor in a test map alongside a NavMeshBoundsVolume and a
 * PlayerStart, then run it from the Session Frontend or via
 * "Automation RunTests Project.Functional Tests".
 */
UCLASS()
class SIGNALFORGERPG_API ASFRespawnLoadoutTest : public AFunctionalTest
{
	GENERATED_BODY()

public:
	ASFRespawnLoadoutTest();

	// AFunctionalTest
	virtual bool IsReady_Implementation() override;
	virtual void PrepareTest() override;
	virtual void StartTest() override;

protected:
	/**
	 * Weapon definition to equip on the player before killing them. The test
	 * fails fast if this isn't set -- the whole point is to verify a configured
	 * weapon's animation data round-trips through respawn.
	 */
	UPROPERTY(EditAnywhere, Category = "Respawn Test")
	TSoftObjectPtr<USFWeaponData> WeaponToEquip;

	/** Slot to put WeaponToEquip into. */
	UPROPERTY(EditAnywhere, Category = "Respawn Test")
	ESFEquipmentSlot SlotToEquip = ESFEquipmentSlot::PrimaryWeapon;

	/**
	 * How long to wait after triggering respawn before asserting the new pawn's
	 * state. Needs to cover: game-mode pawn destruction + RestartPlayerAtTransform
	 * + OnPossess + the one-tick-deferred ApplyEquipmentSnapshotToFreshPawn.
	 * One second is generous; reduce if the test gets adopted into a fast suite.
	 */
	UPROPERTY(EditAnywhere, Category = "Respawn Test", meta = (ClampMin = "0.1"))
	float PostRespawnWaitSeconds = 1.0f;

private:
	/** Locate the player pawn currently possessed by player controller 0. */
	ASFCharacterBase* FindPlayerCharacter() const;

	/**
	 * Capture overlay state from the currently-equipped weapon into the
	 * Before* fields. Returns false if the character has no current weapon.
	 */
	bool CaptureBaselineFromCharacter(ASFCharacterBase* Character);

	/** Step 1: equip the weapon and stash the baseline. */
	void Step_EquipAndBaseline();

	/** Step 2: kill the pawn and request respawn from the player controller. */
	void Step_KillAndRequestRespawn();

	/** Step 3: assert the restored pawn's overlay state matches the baseline. */
	void Step_AssertRestored();

	/** Overlay snapshot captured before death. */
	uint8 BeforeOverlayMode = 0;
	bool bBeforeUseUpperBodyOverlay = false;
	TSubclassOf<UAnimInstance> BeforeOverlayLinkedAnimLayerClass;
	bool bBaselineCaptured = false;

	/** Keep the loaded weapon alive across the death/respawn gap. */
	UPROPERTY(Transient)
	TObjectPtr<USFWeaponData> LoadedWeaponDef;
};
