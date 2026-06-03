// Copyright Fallen Signal Studios 2026.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/SFInventoryComponent.h"
#include "Inventory/SFInventoryTypes.h"
#include "Inventory/SFItemDefinition.h"
#include "Tests/SFInventoryBroadcastCounter.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

/**
 * Pure-logic spec for USFInventoryComponent::SetInventoryEntriesFromSave.
 *
 * This is the load-side of the save round-trip: it restores InventoryEntries
 * from a TArray<FSFInventoryEntry> coming out of the save service. The
 * function intentionally does NOT call FSFInventoryEntry::Normalize() (which
 * would clobber durability/instance tags and reseed defaults). Instead, it
 * enforces a small set of minimum invariants and drops rows whose item
 * definition failed to resolve.
 *
 * These specs lock that contract down so future refactors can't silently
 * change save-restore behavior without a test screaming.
 */

BEGIN_DEFINE_SPEC(
	FSFInventoryComponentSpec,
	"SignalForge.Inventory.SFInventoryComponent.SetInventoryEntriesFromSave",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::ClientContext
		| EAutomationTestFlags::ProductFilter)

	/** Component-under-test. Kept alive via Strong pointer to survive GC between Its. */
	TStrongObjectPtr<USFInventoryComponent> Inventory;

	/**
	 * Pool of item definitions created per-It. We rely on TStrongObjectPtr to
	 * keep them rooted; clearing the array at TearDown releases them for GC.
	 */
	TArray<TStrongObjectPtr<USFItemDefinition>> ItemDefs;

	/** Receives OnInventoryUpdated broadcasts and counts them. */
	TStrongObjectPtr<USFInventoryBroadcastCounter> Counter;

	/** Build a non-null, distinguishable USFItemDefinition for use in test entries. */
	USFItemDefinition* MakeItemDef(FName ItemId);

	/** Build an entry by direct field assignment (no Normalize() call). */
	FSFInventoryEntry MakeEntry(
		USFItemDefinition* Def,
		int32 Quantity,
		int32 ItemLevel,
		const FGuid& EntryId);

END_DEFINE_SPEC(FSFInventoryComponentSpec)

USFItemDefinition* FSFInventoryComponentSpec::MakeItemDef(FName ItemId)
{
	USFItemDefinition* Def = NewObject<USFItemDefinition>(GetTransientPackage());
	Def->ItemId = ItemId;
	ItemDefs.Add(TStrongObjectPtr<USFItemDefinition>(Def));
	return Def;
}

FSFInventoryEntry FSFInventoryComponentSpec::MakeEntry(
	USFItemDefinition* Def,
	int32 Quantity,
	int32 ItemLevel,
	const FGuid& EntryId)
{
	// Default-constructed entry skips Normalize(), which is exactly what we
	// want when feeding SetInventoryEntriesFromSave: we need to exercise the
	// function's own clamp logic, not the constructor's.
	FSFInventoryEntry Entry;
	Entry.EntryId = EntryId;
	Entry.ItemDefinition = Def;
	Entry.Quantity = Quantity;
	Entry.ItemLevel = ItemLevel;
	return Entry;
}

void FSFInventoryComponentSpec::Define()
{
	BeforeEach([this]()
	{
		Inventory = TStrongObjectPtr<USFInventoryComponent>(
			NewObject<USFInventoryComponent>(GetTransientPackage()));

		Counter = TStrongObjectPtr<USFInventoryBroadcastCounter>(
			NewObject<USFInventoryBroadcastCounter>(GetTransientPackage()));

		Inventory->OnInventoryUpdated.AddDynamic(
			Counter.Get(),
			&USFInventoryBroadcastCounter::HandleBroadcast);
	});

	AfterEach([this]()
	{
		if (Inventory.IsValid() && Counter.IsValid())
		{
			Inventory->OnInventoryUpdated.RemoveDynamic(
				Counter.Get(),
				&USFInventoryBroadcastCounter::HandleBroadcast);
		}
		Inventory.Reset();
		Counter.Reset();
		ItemDefs.Reset();
	});

	Describe("when restoring a non-empty save payload", [this]()
	{
		It("round-trips entries (count, ItemDefinition, Quantity, ItemLevel, EntryId) verbatim", [this]()
		{
			USFItemDefinition* DefA = MakeItemDef(TEXT("ItemA"));
			USFItemDefinition* DefB = MakeItemDef(TEXT("ItemB"));
			const FGuid IdA = FGuid::NewGuid();
			const FGuid IdB = FGuid::NewGuid();

			TArray<FSFInventoryEntry> Incoming;
			Incoming.Add(MakeEntry(DefA, /*Quantity=*/5, /*ItemLevel=*/3, IdA));
			Incoming.Add(MakeEntry(DefB, /*Quantity=*/1, /*ItemLevel=*/7, IdB));

			Inventory->SetInventoryEntriesFromSave(Incoming);

			const TArray<FSFInventoryEntry>& Restored = Inventory->GetInventoryEntries();
			TestEqual(TEXT("Restored entry count"), Restored.Num(), 2);

			TestEqual(TEXT("Entry 0 ItemDefinition"), Restored[0].ItemDefinition.Get(), DefA);
			TestEqual(TEXT("Entry 0 Quantity"), Restored[0].Quantity, 5);
			TestEqual(TEXT("Entry 0 ItemLevel"), Restored[0].ItemLevel, 3);
			TestEqual(TEXT("Entry 0 EntryId"), Restored[0].EntryId, IdA);

			TestEqual(TEXT("Entry 1 ItemDefinition"), Restored[1].ItemDefinition.Get(), DefB);
			TestEqual(TEXT("Entry 1 Quantity"), Restored[1].Quantity, 1);
			TestEqual(TEXT("Entry 1 ItemLevel"), Restored[1].ItemLevel, 7);
			TestEqual(TEXT("Entry 1 EntryId"), Restored[1].EntryId, IdB);
		});

		It("broadcasts OnInventoryUpdated exactly once for the whole batch", [this]()
		{
			USFItemDefinition* Def = MakeItemDef(TEXT("Single"));
			TArray<FSFInventoryEntry> Incoming;
			Incoming.Add(MakeEntry(Def, 1, 1, FGuid::NewGuid()));
			Incoming.Add(MakeEntry(Def, 2, 1, FGuid::NewGuid()));
			Incoming.Add(MakeEntry(Def, 3, 1, FGuid::NewGuid()));

			Inventory->SetInventoryEntriesFromSave(Incoming);

			TestEqual(
				TEXT("Single coherent broadcast (not one-per-entry, not zero)"),
				Counter->BroadcastCount,
				1);
		});

		It("drops rows whose ItemDefinition is null", [this]()
		{
			USFItemDefinition* DefA = MakeItemDef(TEXT("Kept"));

			TArray<FSFInventoryEntry> Incoming;
			Incoming.Add(MakeEntry(DefA, 1, 1, FGuid::NewGuid()));
			// Null-def row: simulates an asset that failed soft-pointer resolution.
			Incoming.Add(MakeEntry(/*Def=*/nullptr, 1, 1, FGuid::NewGuid()));
			Incoming.Add(MakeEntry(DefA, 2, 1, FGuid::NewGuid()));

			Inventory->SetInventoryEntriesFromSave(Incoming);

			const TArray<FSFInventoryEntry>& Restored = Inventory->GetInventoryEntries();
			TestEqual(TEXT("Null-def row dropped"), Restored.Num(), 2);
			TestEqual(TEXT("Surviving entry 0 quantity"), Restored[0].Quantity, 1);
			TestEqual(TEXT("Surviving entry 1 quantity"), Restored[1].Quantity, 2);
		});
	});

	Describe("when restoring entries that violate minimum invariants", [this]()
	{
		It("clamps Quantity < 1 up to 1", [this]()
		{
			USFItemDefinition* Def = MakeItemDef(TEXT("ClampQ"));
			TArray<FSFInventoryEntry> Incoming;
			Incoming.Add(MakeEntry(Def, /*Quantity=*/0, 1, FGuid::NewGuid()));
			Incoming.Add(MakeEntry(Def, /*Quantity=*/-3, 1, FGuid::NewGuid()));

			Inventory->SetInventoryEntriesFromSave(Incoming);

			const TArray<FSFInventoryEntry>& Restored = Inventory->GetInventoryEntries();
			TestEqual(TEXT("Restored count"), Restored.Num(), 2);
			TestEqual(TEXT("Quantity 0 -> 1"), Restored[0].Quantity, 1);
			TestEqual(TEXT("Quantity -3 -> 1"), Restored[1].Quantity, 1);
		});

		It("clamps ItemLevel < 1 up to 1", [this]()
		{
			USFItemDefinition* Def = MakeItemDef(TEXT("ClampL"));
			TArray<FSFInventoryEntry> Incoming;
			Incoming.Add(MakeEntry(Def, 1, /*ItemLevel=*/0, FGuid::NewGuid()));
			Incoming.Add(MakeEntry(Def, 1, /*ItemLevel=*/-99, FGuid::NewGuid()));

			Inventory->SetInventoryEntriesFromSave(Incoming);

			const TArray<FSFInventoryEntry>& Restored = Inventory->GetInventoryEntries();
			TestEqual(TEXT("Restored count"), Restored.Num(), 2);
			TestEqual(TEXT("ItemLevel 0 -> 1"), Restored[0].ItemLevel, 1);
			TestEqual(TEXT("ItemLevel -99 -> 1"), Restored[1].ItemLevel, 1);
		});

		It("preserves a Quantity > MaxStackSize (does NOT call Normalize)", [this]()
		{
			// This locks in the comment on SetInventoryEntriesFromSave: we trust
			// the save author. Normalize would clamp to GetMaxStackSize() and
			// silently delete items the player had. We must not do that on load.
			USFItemDefinition* Def = MakeItemDef(TEXT("NoNormalize"));
			Def->bStackable = true;
			Def->MaxStackSize = 5;

			TArray<FSFInventoryEntry> Incoming;
			Incoming.Add(MakeEntry(Def, /*Quantity=*/999, 1, FGuid::NewGuid()));

			Inventory->SetInventoryEntriesFromSave(Incoming);

			const TArray<FSFInventoryEntry>& Restored = Inventory->GetInventoryEntries();
			TestEqual(TEXT("Quantity preserved as-saved"), Restored[0].Quantity, 999);
		});

		It("generates an EntryId when the incoming row has an invalid GUID", [this]()
		{
			USFItemDefinition* Def = MakeItemDef(TEXT("EnsureId"));
			TArray<FSFInventoryEntry> Incoming;
			Incoming.Add(MakeEntry(Def, 1, 1, /*EntryId=*/FGuid()));

			Inventory->SetInventoryEntriesFromSave(Incoming);

			const TArray<FSFInventoryEntry>& Restored = Inventory->GetInventoryEntries();
			TestEqual(TEXT("Restored count"), Restored.Num(), 1);
			TestTrue(TEXT("EnsureEntryId produced a valid GUID"), Restored[0].EntryId.IsValid());
		});
	});

	Describe("when overwriting an existing inventory", [this]()
	{
		It("replaces prior entries with the incoming set (no append)", [this]()
		{
			USFItemDefinition* DefA = MakeItemDef(TEXT("Old"));
			USFItemDefinition* DefB = MakeItemDef(TEXT("New"));

			TArray<FSFInventoryEntry> First;
			First.Add(MakeEntry(DefA, 1, 1, FGuid::NewGuid()));
			First.Add(MakeEntry(DefA, 1, 1, FGuid::NewGuid()));
			Inventory->SetInventoryEntriesFromSave(First);

			TestEqual(TEXT("First load count"), Inventory->GetInventoryEntries().Num(), 2);

			TArray<FSFInventoryEntry> Second;
			Second.Add(MakeEntry(DefB, 1, 1, FGuid::NewGuid()));
			Inventory->SetInventoryEntriesFromSave(Second);

			const TArray<FSFInventoryEntry>& Restored = Inventory->GetInventoryEntries();
			TestEqual(TEXT("Second load replaces (not appends)"), Restored.Num(), 1);
			TestEqual(TEXT("Surviving definition is the new one"), Restored[0].ItemDefinition.Get(), DefB);
		});

		It("broadcasts once per call regardless of whether prior state existed", [this]()
		{
			USFItemDefinition* Def = MakeItemDef(TEXT("BroadcastTwice"));

			TArray<FSFInventoryEntry> Payload;
			Payload.Add(MakeEntry(Def, 1, 1, FGuid::NewGuid()));

			Inventory->SetInventoryEntriesFromSave(Payload);
			Inventory->SetInventoryEntriesFromSave(Payload);

			TestEqual(TEXT("Two calls = two broadcasts"), Counter->BroadcastCount, 2);
		});
	});

	Describe("when restoring an empty payload", [this]()
	{
		It("clears the inventory and still broadcasts exactly once", [this]()
		{
			USFItemDefinition* Def = MakeItemDef(TEXT("WillBeCleared"));

			TArray<FSFInventoryEntry> Seed;
			Seed.Add(MakeEntry(Def, 1, 1, FGuid::NewGuid()));
			Inventory->SetInventoryEntriesFromSave(Seed);
			Counter->BroadcastCount = 0;

			Inventory->SetInventoryEntriesFromSave(TArray<FSFInventoryEntry>());

			TestEqual(TEXT("Inventory cleared"), Inventory->GetInventoryEntries().Num(), 0);
			TestEqual(TEXT("Empty payload still broadcasts once"), Counter->BroadcastCount, 1);
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
