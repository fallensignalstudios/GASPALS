#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Math/RandomStream.h"
#include "SFLootTable.generated.h"

class USFItemDefinition;

/**
 * FSFLootEntry
 *
 * One row in a USFLootTable. Each entry describes a possible drop -- the
 * item to grant, how many, and either a weight (for weighted-pick rolls) or
 * an independent drop chance (for independent per-row coin flips).
 *
 * The table itself decides which of those two modes is used; see
 * USFLootTable::ERollMode.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFLootEntry
{
	GENERATED_BODY()

	/** Item to grant when this entry is selected. Required. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<USFItemDefinition> ItemDefinition = nullptr;

	/**
	 * Inclusive minimum quantity granted. Final quantity is a uniform random
	 * int in [MinQuantity, MaxQuantity]. Both clamped to >= 1.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1"))
	int32 MinQuantity = 1;

	/** Inclusive maximum quantity granted. Clamped to >= MinQuantity. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1"))
	int32 MaxQuantity = 1;

	/**
	 * Relative weight when the parent table rolls in WeightedPick mode.
	 * Higher = more likely. A weight of 0 disables the entry without
	 * deleting the row. Ignored in IndependentChance mode.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot|Weighted", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	/**
	 * Per-entry drop probability when the parent table rolls in
	 * IndependentChance mode. 0 = never, 1 = always. Ignored in WeightedPick
	 * mode unless the entry is also marked bGuaranteed.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot|Independent", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DropChance = 1.0f;

	/**
	 * If true, this entry ALWAYS drops on a successful roll regardless of
	 * the parent table's roll mode. Useful for guaranteed quest items or
	 * "always drops a credit chip" baseline drops.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
	bool bGuaranteed = false;
};

/**
 * FSFLootRoll
 *
 * One concrete drop produced by USFLootTable::RollDrops. The loot dropper
 * component takes a TArray<FSFLootRoll> and either spawns world pickup
 * actors at the dying character's location or routes them directly into
 * the killer's inventory, depending on the dropper's configuration.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFLootRoll
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	TObjectPtr<USFItemDefinition> ItemDefinition = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	int32 Quantity = 0;
};

UENUM(BlueprintType)
enum class ESFLootRollMode : uint8
{
	/**
	 * Roll N entries by relative weight, where N is uniformly random in
	 * [MinRollsPerDrop, MaxRollsPerDrop]. Entries can be picked multiple
	 * times unless bUniqueEntriesPerRoll is set. Good for "pick 1-2 random
	 * items from this enemy's loot pool".
	 */
	WeightedPick,

	/**
	 * Every entry rolls its own independent DropChance, all in one pass.
	 * MinRollsPerDrop / MaxRollsPerDrop are ignored. Good for "30% chance
	 * of ammo AND 10% chance of a med pack AND 5% chance of a credit chip".
	 */
	IndependentChance
};

/**
 * USFLootTable
 *
 * Data asset describing what an enemy (or container) can drop on death /
 * open. Designed to be reusable -- the same table can be shared between
 * many enemy archetypes, and a single enemy can hold a table reference
 * directly on its USFLootDropperComponent.
 *
 * Authoring tip: keep one table per archetype tier (GruntLoot,
 * HeavyLoot, EliteLoot, BossLoot) rather than per individual enemy. If a
 * specific encounter needs a unique twist, swap the table reference on
 * that placed actor in the level rather than authoring a one-off asset.
 */
UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFLootTable : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	USFLootTable();

	/**
	 * Returns a list of concrete drops produced by this table. Pure of
	 * side effects on the table itself; uses the supplied stream so callers
	 * can make rolls deterministic (e.g. seeded by save slot or encounter
	 * id) for tests or replays.
	 *
	 * Guaranteed entries are always included before the random pass runs,
	 * so a table can mix "always 5 credits + a random chance of a med pack".
	 */
	UFUNCTION(BlueprintCallable, Category = "Loot")
	TArray<FSFLootRoll> RollDrops(int32 InRandomSeed = 0) const;

	/**
	 * Variant that takes an explicit FRandomStream by reference so the
	 * caller can chain multiple rolls and share a single stream.
	 * Available to C++; not exposed to Blueprint (no FRandomStream UPROPERTY).
	 */
	void RollDropsWithStream(FRandomStream& Stream, TArray<FSFLootRoll>& OutRolls) const;

	const TArray<FSFLootEntry>& GetEntries() const { return Entries; }

protected:
	/** Drop pool. Order doesn't matter. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
	TArray<FSFLootEntry> Entries;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
	ESFLootRollMode RollMode = ESFLootRollMode::WeightedPick;

	/**
	 * Inclusive minimum number of weighted picks per RollDrops call.
	 * Ignored when RollMode is IndependentChance. 0 = sometimes no drop.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot|Weighted", meta = (ClampMin = "0", EditCondition = "RollMode == ESFLootRollMode::WeightedPick"))
	int32 MinRollsPerDrop = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot|Weighted", meta = (ClampMin = "0", EditCondition = "RollMode == ESFLootRollMode::WeightedPick"))
	int32 MaxRollsPerDrop = 1;

	/**
	 * In WeightedPick mode, if true a single RollDrops call won't pick the
	 * same entry twice. Pure cosmetic for visual variety in loot piles --
	 * stack-aware inventories merge duplicates anyway, so this mainly
	 * affects how many world-pickup actors spawn.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot|Weighted", meta = (EditCondition = "RollMode == ESFLootRollMode::WeightedPick"))
	bool bUniqueEntriesPerRoll = false;
};
