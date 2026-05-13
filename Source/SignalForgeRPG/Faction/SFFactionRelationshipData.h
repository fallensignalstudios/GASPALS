#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Faction/SFFactionTypes.h"
#include "SFFactionRelationshipData.generated.h"

/**
 * Designer-authored relationship matrix between faction tags.
 *
 * A single global asset is referenced by USignalForgeDeveloperSettings
 * (DefaultFactionRelationships). USFFactionStatics queries this asset
 * for static attitudes; dynamic per-player standings (from the
 * narrative replicator) can override results at runtime.
 *
 * Lookup order in USFFactionStatics::GetAttitude:
 *   1. Source's entry -> explicit row for Target.
 *   2. Source's entry -> DefaultAttitude.
 *   3. SelfHostility for matching tags (used when Source == Target).
 *   4. Asset-level GlobalDefault.
 */
UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFFactionRelationshipData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Per-faction view of the rest of the world. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction")
	TArray<FSFFactionRelationshipEntry> Entries;

	/** Attitude two characters share if both have the exact same faction tag. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction")
	ESFFactionAttitude SelfHostility = ESFFactionAttitude::Friendly;

	/** Fallback when neither source nor target are listed at all. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction")
	ESFFactionAttitude GlobalDefault = ESFFactionAttitude::Neutral;

	/** Returns true if Source was found in Entries (Out filled with matching entry). */
	bool FindEntry(FGameplayTag SourceFaction, FSFFactionRelationshipEntry& OutEntry) const;

	/** Returns the static attitude of Source toward Target using the rules above. */
	ESFFactionAttitude GetAttitude(FGameplayTag SourceFaction, FGameplayTag TargetFaction) const;
};
