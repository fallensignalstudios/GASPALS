#include "Inventory/SFLootTable.h"

#include "Inventory/SFItemDefinition.h"

USFLootTable::USFLootTable()
{
	// Sensible defaults: one weighted pick, no uniqueness constraint. A
	// freshly-created table with one entry will roll that one entry once.
}

TArray<FSFLootRoll> USFLootTable::RollDrops(int32 InRandomSeed) const
{
	// Seed 0 -> use a non-deterministic stream so designers get random
	// drops in PIE without having to plumb a seed. Anything else seeds
	// deterministically for tests / save-slot encounter replays.
	FRandomStream Stream;
	if (InRandomSeed == 0)
	{
		Stream.GenerateNewSeed();
	}
	else
	{
		Stream.Initialize(InRandomSeed);
	}

	TArray<FSFLootRoll> Rolls;
	RollDropsWithStream(Stream, Rolls);
	return Rolls;
}

void USFLootTable::RollDropsWithStream(FRandomStream& Stream, TArray<FSFLootRoll>& OutRolls) const
{
	OutRolls.Reset();

	// Helper: append a concrete roll using one entry's quantity range.
	// Clamps + skips entries with no item set so a half-authored row doesn't
	// crash the spawn path downstream.
	auto AppendRollFromEntry = [&Stream, &OutRolls](const FSFLootEntry& Entry)
	{
		if (!Entry.ItemDefinition)
		{
			return;
		}
		const int32 Min = FMath::Max(1, Entry.MinQuantity);
		const int32 Max = FMath::Max(Min, Entry.MaxQuantity);
		FSFLootRoll Roll;
		Roll.ItemDefinition = Entry.ItemDefinition;
		Roll.Quantity = Stream.RandRange(Min, Max);
		OutRolls.Add(Roll);
	};

	// Phase 1 -- guaranteed entries always drop, regardless of roll mode.
	// Quest items, mandatory currency, etc. live here. Doing this first
	// means an "always credits + maybe a med pack" table reads top-down in
	// the asset editor exactly the way it executes.
	for (const FSFLootEntry& Entry : Entries)
	{
		if (Entry.bGuaranteed)
		{
			AppendRollFromEntry(Entry);
		}
	}

	// Phase 2 -- the random pass. Skip guaranteed entries this pass; they
	// already dropped above and double-counting them would let a guaranteed
	// row also win a weighted slot, which is confusing for designers.
	if (RollMode == ESFLootRollMode::IndependentChance)
	{
		for (const FSFLootEntry& Entry : Entries)
		{
			if (Entry.bGuaranteed || !Entry.ItemDefinition)
			{
				continue;
			}
			if (Stream.FRand() <= Entry.DropChance)
			{
				AppendRollFromEntry(Entry);
			}
		}
		return;
	}

	// WeightedPick path. Build a parallel array of indices into Entries
	// plus a total weight; we'll resample from this list each pick.
	struct FCandidate
	{
		int32 EntryIndex = INDEX_NONE;
		float Weight = 0.0f;
	};
	TArray<FCandidate> Candidates;
	Candidates.Reserve(Entries.Num());
	float TotalWeight = 0.0f;
	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		const FSFLootEntry& Entry = Entries[i];
		if (Entry.bGuaranteed || !Entry.ItemDefinition || Entry.Weight <= 0.0f)
		{
			continue;
		}
		Candidates.Add({ i, Entry.Weight });
		TotalWeight += Entry.Weight;
	}

	if (Candidates.Num() == 0 || TotalWeight <= 0.0f)
	{
		// Nothing eligible. Leave OutRolls at whatever Phase 1 produced.
		return;
	}

	const int32 MinRolls = FMath::Max(0, MinRollsPerDrop);
	const int32 MaxRolls = FMath::Max(MinRolls, MaxRollsPerDrop);
	const int32 NumRolls = Stream.RandRange(MinRolls, MaxRolls);

	for (int32 RollIdx = 0; RollIdx < NumRolls; ++RollIdx)
	{
		if (Candidates.Num() == 0 || TotalWeight <= 0.0f)
		{
			break;
		}

		// Standard weighted draw: pick a target in [0, TotalWeight) and
		// walk the candidate list accumulating weight until we cross it.
		const float Target = Stream.FRand() * TotalWeight;
		float Accum = 0.0f;
		int32 PickedIdx = INDEX_NONE;
		for (int32 c = 0; c < Candidates.Num(); ++c)
		{
			Accum += Candidates[c].Weight;
			if (Target <= Accum)
			{
				PickedIdx = c;
				break;
			}
		}
		// Floating-point slop can leave Target slightly > Accum on the last
		// iteration; fall back to the final candidate so a roll is never
		// "lost" silently.
		if (PickedIdx == INDEX_NONE)
		{
			PickedIdx = Candidates.Num() - 1;
		}

		const FCandidate& Picked = Candidates[PickedIdx];
		AppendRollFromEntry(Entries[Picked.EntryIndex]);

		// Optional uniqueness: pluck the picked candidate so it can't win
		// again in this roll. Subtract its weight from the running total
		// so the resampling math stays correct without rebuilding the list.
		if (bUniqueEntriesPerRoll)
		{
			TotalWeight -= Picked.Weight;
			Candidates.RemoveAtSwap(PickedIdx);
		}
	}
}
