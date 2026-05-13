#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class AAIController;
class APawn;
class ASFCharacterBase;
class UBlackboardComponent;

/**
 * Shared helpers used by SignalForge behavior tree nodes (tasks, services,
 * decorators). Kept here so we don't duplicate the same "get controlled
 * pawn / faction / blackboard / weapon" boilerplate in every node.
 *
 * All helpers tolerate null inputs and return null/false/zero on missing
 * pieces so callers can chain them safely.
 */
namespace SFBTHelpers
{
	/** Resolves the AI's controlled character. Returns nullptr if missing or wrong type. */
	SIGNALFORGERPG_API ASFCharacterBase* GetControlledCharacter(AAIController* AIOwner);

	/** Reads an Actor BB key by name. Returns nullptr on miss / cast failure. */
	SIGNALFORGERPG_API AActor* GetBBActor(UBlackboardComponent* BB, FName KeyName);

	/** Reads a Vector BB key by name. Writes default-zero on miss; returns success bool. */
	SIGNALFORGERPG_API bool GetBBVector(UBlackboardComponent* BB, FName KeyName, FVector& OutVec);

	/** True if Target is hostile to Self per the global faction relationship table. Tolerates nulls. */
	SIGNALFORGERPG_API bool IsHostile(const AActor* Self, const AActor* Target);

	/**
	 * Returns the effective "engagement distance" of the AI's currently-equipped weapon.
	 * Reads from USFWeaponData and picks the right field depending on weapon type:
	 *   - Hitscan ranged: RangedConfig.HitscanMaxRange
	 *   - Projectile ranged: rough estimate based on falloff end
	 *   - Beam: RangedConfig.HitscanMaxRange (beam re-uses ranged trace)
	 *   - Melee: a small melee range (TraceLength on the combat component)
	 *   - Caster: large default (projectile-based)
	 * Returns 0 if no weapon is equipped.
	 */
	SIGNALFORGERPG_API float GetEquippedWeaponRange(const ASFCharacterBase* Character);

	/** Eye-to-eye line trace using ECC_Visibility. Returns true if no blocking geometry. */
	SIGNALFORGERPG_API bool HasLineOfSight(const ASFCharacterBase* From, const AActor* To);
}
