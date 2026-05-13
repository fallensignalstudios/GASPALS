#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "GenericTeamAgentInterface.h"
#include "Faction/SFFactionTypes.h"
#include "SFFactionStatics.generated.h"

class AActor;
class USFFactionRelationshipData;

/**
 * Cross-cutting hostility / attitude resolver. Combat, AI, and progression
 * all funnel through here so faction logic stays in one place.
 *
 * Public surface mirrors gameplay needs:
 *   - AreHostile(A,B)         : combat damage gate.
 *   - GetEffectiveAttitude... : team-attitude (used by AI perception via the
 *                               IGenericTeamAgentInterface implementation).
 *   - GetFactionTag(Actor)    : convenience that finds the
 *                               USFFactionComponent (or, fallback, the
 *                               NPC narrative identity component).
 *
 * Resolution order in GetEffectiveAttitudeBetweenFactions:
 *   1. (Optional) Dynamic per-player standing band from the narrative
 *      replicator -- if the source actor is a pawn controlled by a
 *      player and the standing band for TargetFaction crosses the
 *      Hostile or Allied thresholds, that wins.
 *   2. Static lookup in the default USFFactionRelationshipData asset
 *      pointed at by USignalForgeDeveloperSettings.
 *   3. Fall back to ESFFactionAttitude::Neutral.
 */
UCLASS()
class SIGNALFORGERPG_API USFFactionStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Convenience: AreHostile(A, B) -- the most common combat call. */
	UFUNCTION(BlueprintPure, Category = "Faction", meta = (DefaultToSelf = "FromActor"))
	static bool AreHostile(const AActor* FromActor, const AActor* ToActor);

	/** Returns the static attitude between two factions using the global relationship asset. */
	UFUNCTION(BlueprintPure, Category = "Faction")
	static ESFFactionAttitude GetStaticAttitudeBetweenFactions(FGameplayTag SourceFaction, FGameplayTag TargetFaction);

	/** Returns the effective attitude, applying dynamic standing on top of static. */
	UFUNCTION(BlueprintPure, Category = "Faction", meta = (DefaultToSelf = "FromActor"))
	static ESFFactionAttitude GetEffectiveAttitudeBetweenActors(const AActor* FromActor, const AActor* ToActor);

	/** Reads the faction tag from FactionComponent if present, else from the NPC identity component. */
	UFUNCTION(BlueprintPure, Category = "Faction")
	static FGameplayTag GetFactionTag(const AActor* Actor);

	/** Map a faction tag to a stable FGenericTeamId for AI perception. */
	static FGenericTeamId TeamIdFromFactionTag(FGameplayTag FactionTag);

	/** Resolve the global relationship asset from developer settings (may be null). */
	static const USFFactionRelationshipData* GetRelationshipData();
};
