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

UINTERFACE(MinimalAPI, Blueprintable)
class USFCombatantInterface : public UInterface
{
	GENERATED_BODY()
};

class SIGNALFORGERPG_API ISFCombatantInterface
{
	GENERATED_BODY()

public:
	/** True if this combatant has been killed and should not be re-damaged or re-targeted. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Combat|Combatant")
	bool IsDead() const;

	/** True if the combatant is currently in a defensive block stance. Read-only -- damage gating is decided by the hit resolver, not here. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Combat|Combatant")
	bool IsBlocking() const;

	/** World location to aim at (auto-aim, projectile homing). Default impl returns actor location; characters override to center-of-mass. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Combat|Combatant")
	FVector GetCombatLocation() const;

	/** Best-effort socket lookup for hit FX / targeting (e.g. "head", "spine_03"). Returns actor transform if socket is unknown. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Combat|Combatant")
	FTransform GetCombatSocketTransform(FName SocketName) const;

	/** Read combatant state as a tag container (useful for ability activation policies). */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Combat|Combatant")
	void GetCombatStateTags(FGameplayTagContainer& OutTags) const;

	/** Called by the hit resolver after damage is applied so the target can remember who hit it (XP, death attribution). */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Combat|Combatant")
	void RegisterDamageInstigator(AActor* Instigator);
};
