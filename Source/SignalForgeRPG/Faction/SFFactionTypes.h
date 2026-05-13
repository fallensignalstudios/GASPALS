#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SFFactionTypes.generated.h"

/**
 * Coarse attitude between two factions. Resolved through
 * USFFactionRelationshipData lookups (static) and, optionally,
 * the per-player dynamic faction standing band on the narrative
 * replicator. Combat hostility gating uses this enum.
 */
UENUM(BlueprintType)
enum class ESFFactionAttitude : uint8
{
	/** No data; default to Neutral for combat purposes. */
	Unknown		UMETA(DisplayName = "Unknown"),

	/** Allies / will-not-target. Friendly fire is blocked. */
	Friendly	UMETA(DisplayName = "Friendly"),

	/** No combat interest either direction. */
	Neutral		UMETA(DisplayName = "Neutral"),

	/** Hostile. Will engage, applies damage, grants XP on kill. */
	Hostile		UMETA(DisplayName = "Hostile"),
};

/**
 * Single row of a faction relationship matrix. Each row says
 * "TargetFaction -> Attitude" for the owning source faction.
 * Used by USFFactionRelationshipData.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFFactionAttitudeRow
{
	GENERATED_BODY()

	/** The other faction we're describing our attitude toward. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction", meta = (Categories = "Faction"))
	FGameplayTag TargetFaction;

	/** How the owning faction feels about TargetFaction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction")
	ESFFactionAttitude Attitude = ESFFactionAttitude::Neutral;
};

/**
 * One faction's view of the world: a list of explicit per-faction
 * attitudes plus a fallback for any faction not in the list.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFFactionRelationshipEntry
{
	GENERATED_BODY()

	/** The faction this entry describes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction", meta = (Categories = "Faction"))
	FGameplayTag SourceFaction;

	/** Fallback when no row matches. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction")
	ESFFactionAttitude DefaultAttitude = ESFFactionAttitude::Neutral;

	/** Explicit overrides per other faction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction")
	TArray<FSFFactionAttitudeRow> AttitudesTowardOthers;
};
