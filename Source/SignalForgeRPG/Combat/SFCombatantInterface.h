// SFCombatantInterface.h
//
// ISFCombatant: contract for anything that can participate as a target or
// source in the combat pipeline. Lets abilities, projectiles, and AI talk to
// "the thing on the receiving end of a hit" without casting to a concrete
// character class. This is the foundation of the dual-protagonist refactor
// (and lets non-character combatants -- destructible cover, vehicles, future
// constructs -- enter the damage pipeline cleanly).
//
// Convention: read-only queries are const; the only writer surface is
// RegisterDamageInstigator, which the hit-resolver calls so the target can
// remember who damaged it for XP attribution / death attribution.
//
// Faction membership is intentionally NOT exposed here -- callers should keep
// using USFFactionStatics::AreHostile(AActor*, AActor*), which already works
// against any actor with USFFactionComponent regardless of class.

#pragma once

#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "SFCombatantInterface.generated.h"

struct FSFHitData;

UINTERFACE(MinimalAPI)
class USFCombatantInterface : public UInterface
{
	GENERATED_BODY()
};

class SIGNALFORGERPG_API ISFCombatantInterface
{
	GENERATED_BODY()

public:
	// NOTE on call convention:
	//   Plain C++ pure-virtuals, NOT BlueprintNativeEvents. The codebase has
	//   dozens of direct "Char->IsDead()" call sites (AI BT, regen tick, HUD,
	//   companion state machine, ...). UFUNCTION + BlueprintNativeEvent on a
	//   UInterface would route those direct calls through the UHT-generated
	//   event stub, which asserts at runtime ("Do not directly call Event
	//   functions in Interfaces. Call Execute_... instead."). Tripped first by
	//   SFStatRegenComponent::TickRegen() on the very first stat regen tick.
	//   See SFWeaponHolderInterface.h for the same pattern / rationale.

	/** True if this combatant has been killed and should not be re-damaged or re-targeted. */
	virtual bool IsDead() const = 0;

	/** True if the combatant is currently in a defensive block stance. Read-only -- damage gating is decided by the hit resolver, not here. */
	virtual bool IsBlocking() const = 0;

	/** World location to aim at (auto-aim, projectile homing). Default impl returns actor location; characters override to center-of-mass. */
	virtual FVector GetCombatLocation() const = 0;

	/** Best-effort socket lookup for hit FX / targeting (e.g. "head", "spine_03"). Returns actor transform if socket is unknown. */
	virtual FTransform GetCombatSocketTransform(FName SocketName) const = 0;

	/** Read combatant state as a tag container (useful for ability activation policies). */
	virtual void GetCombatStateTags(FGameplayTagContainer& OutTags) const = 0;

	/** Called by the hit resolver after damage is applied so the target can remember who hit it (XP, death attribution). */
	/* InInstigator is named with the In-prefix to avoid shadowing AActor::Instigator (the TObjectPtr<APawn> on every actor). */
	virtual void RegisterDamageInstigator(AActor* InInstigator) = 0;
};
