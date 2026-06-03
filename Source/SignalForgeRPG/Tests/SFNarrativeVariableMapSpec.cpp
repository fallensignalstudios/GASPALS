// Copyright Fallen Signal Studios 2026.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameplayTagContainer.h"
#include "Narrative/SFNarrativeTypes.h"
#include "Narrative/SFNarrativeVariableMap.h"

/**
 * Pure-logic spec for FSFNarrativeVariableMap snapshot round-trips.
 *
 * The variable map exposes BuildSnapshots() (flatten) and
 * RestoreFromSnapshots() (rehydrate). Both sides encode/decode variable names
 * into FSFWorldFactKey::ContextId as "<Base>|<VarName>" (or bare <VarName>
 * when Base is None). Round-tripping has to preserve type, value, and the
 * ContextId encoding/decoding contract.
 *
 * There are some asymmetries that the spec documents and locks down so future
 * refactors don't silently break save compatibility:
 *
 *  - Tag variables flatten to one snapshot. Restoring a single Tag snapshot
 *    for a given name comes back as Type=Tag.
 *  - TagContainer variables flatten to N Tag snapshots (one per tag). When
 *    restore sees 2+ Tag snapshots sharing the same variable name, it
 *    promotes the type to TagContainer. A single-element TagContainer does
 *    NOT survive the round-trip as a TagContainer -- it comes back as Tag.
 *    This is the documented TODO on the TagContainer flatten path.
 *  - Invalid (empty) FGameplayTags are dropped on both sides.
 */

namespace SFNarrativeVariableMapSpecTags
{
	/**
	 * Resolve an already-registered native gameplay tag by name.
	 *
	 * IMPORTANT: we cannot call AddNativeGameplayTag from inside a spec --
	 * by the time specs run, UGameplayTagsManager has been sealed via
	 * DoneAddingNativeTags() and any new registration fires an ensure that
	 * the automation framework escalates to a test failure. So we use tags
	 * that the project registers itself in SignalForgeGameplayTags.cpp at
	 * module startup. We only care about their *identity* here -- they are
	 * opaque keys for the round-trip, not gameplay state.
	 */
	static FGameplayTag Resolve(const TCHAR* TagName)
	{
		return FGameplayTag::RequestGameplayTag(FName(TagName), /*ErrorIfNotFound=*/false);
	}

	/** Fact-tag base. Any well-known stable native tag works as an opaque key. */
	static FGameplayTag GetBaseTag()
	{
		return Resolve(TEXT("Attributes.Health"));
	}

	/** Distinct from the fact tag -- used as the *value* of a Tag-typed variable. */
	static FGameplayTag GetPayloadTagA()
	{
		return Resolve(TEXT("Attributes.MaxHealth"));
	}

	static FGameplayTag GetPayloadTagB()
	{
		return Resolve(TEXT("Attributes.Stamina"));
	}

	/**
	 * Distinct fact tag for negative-path tests that need to prove the
	 * restore side rejects mismatched fact tags.
	 */
	static FGameplayTag GetOtherFactTag()
	{
		return Resolve(TEXT("Attributes.MaxStamina"));
	}
}

BEGIN_DEFINE_SPEC(
	FSFNarrativeVariableMapSpec,
	"SignalForge.Narrative.SFNarrativeVariableMap.SnapshotRoundTrip",
	EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::ClientContext
		| EAutomationTestFlags::ProductFilter)

	FSFNarrativeVariableMap Map;
	FGameplayTag FactBase;

END_DEFINE_SPEC(FSFNarrativeVariableMapSpec)

void FSFNarrativeVariableMapSpec::Define()
{
	BeforeEach([this]()
	{
		Map = FSFNarrativeVariableMap();
		FactBase = SFNarrativeVariableMapSpecTags::GetBaseTag();

		// Stable native tags from SignalForgeGameplayTags.cpp. If they ever get
		// renamed/removed, surface that loudly instead of letting every It fail
		// with mystery 'expected 1, was 0' errors.
		if (!FactBase.IsValid()
			|| !SFNarrativeVariableMapSpecTags::GetPayloadTagA().IsValid()
			|| !SFNarrativeVariableMapSpecTags::GetPayloadTagB().IsValid()
			|| !SFNarrativeVariableMapSpecTags::GetOtherFactTag().IsValid())
		{
			AddError(TEXT(
				"FSFNarrativeVariableMapSpec relies on Attributes.* native tags from "
				"SignalForgeGameplayTags.cpp. One or more were not resolvable -- update "
				"SFNarrativeVariableMapSpecTags::Get*Tag() to point at currently-registered tags."));
		}
	});

	Describe("scalar round-trips (Int/Float/Bool/Name)", [this]()
	{
		It("preserves an Int through Build -> Restore", [this]()
		{
			Map.SetInt(TEXT("Score"), 42);

			TArray<FSFWorldFactSnapshot> Snapshots;
			Map.BuildSnapshots(Snapshots, FactBase, /*ContextIdBase=*/NAME_None);
			TestEqual(TEXT("Int flattens to 1 snapshot"), Snapshots.Num(), 1);

			FSFNarrativeVariableMap Restored;
			Restored.RestoreFromSnapshots(Snapshots, FactBase, /*ContextIdBase=*/NAME_None);

			int32 OutValue = 0;
			TestTrue(TEXT("GetInt succeeds"), Restored.GetInt(TEXT("Score"), OutValue));
			TestEqual(TEXT("Int value preserved"), OutValue, 42);
			TestEqual(
				TEXT("Type preserved as Int"),
				Restored.GetVariableType(TEXT("Score")),
				ESFNarrativeVariableType::Int);
		});

		It("preserves a Float through Build -> Restore", [this]()
		{
			Map.SetFloat(TEXT("Affinity"), 3.5f);

			TArray<FSFWorldFactSnapshot> Snapshots;
			Map.BuildSnapshots(Snapshots, FactBase);

			FSFNarrativeVariableMap Restored;
			Restored.RestoreFromSnapshots(Snapshots, FactBase);

			float OutValue = 0.0f;
			TestTrue(TEXT("GetFloat succeeds"), Restored.GetFloat(TEXT("Affinity"), OutValue));
			TestEqual(TEXT("Float value preserved"), OutValue, 3.5f);
		});

		It("preserves a Bool through Build -> Restore", [this]()
		{
			Map.SetBool(TEXT("MetCharacter"), true);

			TArray<FSFWorldFactSnapshot> Snapshots;
			Map.BuildSnapshots(Snapshots, FactBase);

			FSFNarrativeVariableMap Restored;
			Restored.RestoreFromSnapshots(Snapshots, FactBase);

			bool OutValue = false;
			TestTrue(TEXT("GetBool succeeds"), Restored.GetBool(TEXT("MetCharacter"), OutValue));
			TestTrue(TEXT("Bool value preserved (true)"), OutValue);
		});

		It("preserves a Name through Build -> Restore", [this]()
		{
			const FName Expected(TEXT("OutpostAlpha"));
			Map.SetName(TEXT("CurrentRegion"), Expected);

			TArray<FSFWorldFactSnapshot> Snapshots;
			Map.BuildSnapshots(Snapshots, FactBase);

			FSFNarrativeVariableMap Restored;
			Restored.RestoreFromSnapshots(Snapshots, FactBase);

			FName OutValue = NAME_None;
			TestTrue(TEXT("GetName succeeds"), Restored.GetName(TEXT("CurrentRegion"), OutValue));
			TestEqual(TEXT("Name value preserved"), OutValue, Expected);
		});
	});

	Describe("tag-typed variables", [this]()
	{
		It("preserves a single Tag through Build -> Restore", [this]()
		{
			const FGameplayTag Payload = SFNarrativeVariableMapSpecTags::GetPayloadTagA();
			Map.SetTag(TEXT("CurrentObjective"), Payload);

			TArray<FSFWorldFactSnapshot> Snapshots;
			Map.BuildSnapshots(Snapshots, FactBase);
			TestEqual(TEXT("Tag flattens to 1 snapshot"), Snapshots.Num(), 1);

			FSFNarrativeVariableMap Restored;
			Restored.RestoreFromSnapshots(Snapshots, FactBase);

			FGameplayTag OutTag;
			TestTrue(TEXT("GetTag succeeds"), Restored.GetTag(TEXT("CurrentObjective"), OutTag));
			TestEqual(TEXT("Tag preserved"), OutTag, Payload);
		});

		It("drops an invalid (empty) Tag at flatten time", [this]()
		{
			Map.SetTag(TEXT("BogusTag"), FGameplayTag());

			TArray<FSFWorldFactSnapshot> Snapshots;
			Map.BuildSnapshots(Snapshots, FactBase);

			TestEqual(TEXT("Invalid tag produces no snapshots"), Snapshots.Num(), 0);
		});
	});

	Describe("tag-container variables", [this]()
	{
		It("flattens a 2-tag container to 2 snapshots", [this]()
		{
			FGameplayTagContainer Container;
			Container.AddTag(SFNarrativeVariableMapSpecTags::GetPayloadTagA());
			Container.AddTag(SFNarrativeVariableMapSpecTags::GetPayloadTagB());
			Map.SetTagContainer(TEXT("Flags"), Container);

			TArray<FSFWorldFactSnapshot> Snapshots;
			Map.BuildSnapshots(Snapshots, FactBase);

			TestEqual(TEXT("Container flattens to one snapshot per tag"), Snapshots.Num(), 2);
		});

		It("restores 2+ same-name Tag snapshots back into a TagContainer", [this]()
		{
			// This is the documented TODO: TagContainer round-trips correctly
			// only when it has 2 or more elements. The restore side detects
			// repeat hits on the same variable name and promotes Tag -> TagContainer.
			const FGameplayTag TagA = SFNarrativeVariableMapSpecTags::GetPayloadTagA();
			const FGameplayTag TagB = SFNarrativeVariableMapSpecTags::GetPayloadTagB();

			FGameplayTagContainer Container;
			Container.AddTag(TagA);
			Container.AddTag(TagB);
			Map.SetTagContainer(TEXT("Flags"), Container);

			TArray<FSFWorldFactSnapshot> Snapshots;
			Map.BuildSnapshots(Snapshots, FactBase);

			FSFNarrativeVariableMap Restored;
			Restored.RestoreFromSnapshots(Snapshots, FactBase);

			TestEqual(
				TEXT("Type promoted back to TagContainer"),
				Restored.GetVariableType(TEXT("Flags")),
				ESFNarrativeVariableType::TagContainer);

			FGameplayTagContainer OutContainer;
			TestTrue(TEXT("GetTagContainer succeeds"), Restored.GetTagContainer(TEXT("Flags"), OutContainer));
			TestTrue(TEXT("Container has TagA"), OutContainer.HasTagExact(TagA));
			TestTrue(TEXT("Container has TagB"), OutContainer.HasTagExact(TagB));
			TestEqual(TEXT("Container size 2"), OutContainer.Num(), 2);
		});

		It("emits no snapshots for an empty TagContainer", [this]()
		{
			Map.SetTagContainer(TEXT("Empty"), FGameplayTagContainer());

			TArray<FSFWorldFactSnapshot> Snapshots;
			Map.BuildSnapshots(Snapshots, FactBase);

			TestEqual(TEXT("Empty container produces no snapshots"), Snapshots.Num(), 0);
		});
	});

	Describe("ContextId encoding", [this]()
	{
		It("encodes <Base>|<VarName> when ContextIdBase is set", [this]()
		{
			Map.SetInt(TEXT("Hits"), 7);

			TArray<FSFWorldFactSnapshot> Snapshots;
			const FName Base(TEXT("QuestA"));
			Map.BuildSnapshots(Snapshots, FactBase, Base);

			TestEqual(TEXT("One snapshot"), Snapshots.Num(), 1);
			TestEqual(
				TEXT("ContextId encoded as Base|VarName"),
				Snapshots[0].Key.ContextId,
				FName(TEXT("QuestA|Hits")));
		});

		It("uses the raw VarName as ContextId when ContextIdBase is None", [this]()
		{
			Map.SetInt(TEXT("Hits"), 7);

			TArray<FSFWorldFactSnapshot> Snapshots;
			Map.BuildSnapshots(Snapshots, FactBase, NAME_None);

			TestEqual(TEXT("One snapshot"), Snapshots.Num(), 1);
			TestEqual(
				TEXT("ContextId is raw VarName"),
				Snapshots[0].Key.ContextId,
				FName(TEXT("Hits")));
		});

		It("ignores snapshots whose ContextId does not start with the expected prefix on restore", [this]()
		{
			// Build under QuestA; restore under QuestB. Nothing should restore.
			Map.SetInt(TEXT("Hits"), 7);

			TArray<FSFWorldFactSnapshot> Snapshots;
			Map.BuildSnapshots(Snapshots, FactBase, FName(TEXT("QuestA")));

			FSFNarrativeVariableMap Restored;
			Restored.RestoreFromSnapshots(Snapshots, FactBase, FName(TEXT("QuestB")));

			TestFalse(
				TEXT("Variable rejected because prefix mismatch"),
				Restored.HasVariable(TEXT("Hits")));
		});

		It("ignores snapshots whose FactTag does not match FactTagBase on restore", [this]()
		{
			Map.SetInt(TEXT("Hits"), 7);

			TArray<FSFWorldFactSnapshot> Snapshots;
			Map.BuildSnapshots(Snapshots, FactBase, NAME_None);

			// Restore under a *different* fact tag.
			const FGameplayTag OtherFact = SFNarrativeVariableMapSpecTags::GetOtherFactTag();
			FSFNarrativeVariableMap Restored;
			Restored.RestoreFromSnapshots(Snapshots, OtherFact, NAME_None);

			TestFalse(
				TEXT("Variable rejected because fact tag mismatch"),
				Restored.HasVariable(TEXT("Hits")));
		});
	});

	Describe("Reset semantics", [this]()
	{
		It("Reset() empties all variables", [this]()
		{
			Map.SetInt(TEXT("A"), 1);
			Map.SetFloat(TEXT("B"), 2.0f);
			Map.SetBool(TEXT("C"), true);

			TestTrue(TEXT("Has A before Reset"), Map.HasVariable(TEXT("A")));
			Map.Reset();
			TestFalse(TEXT("A gone after Reset"), Map.HasVariable(TEXT("A")));
			TestFalse(TEXT("B gone after Reset"), Map.HasVariable(TEXT("B")));
			TestFalse(TEXT("C gone after Reset"), Map.HasVariable(TEXT("C")));
		});

		It("RestoreFromSnapshots starts by resetting existing variables", [this]()
		{
			Map.SetInt(TEXT("Stale"), 999);

			// Restore from an empty snapshot list -- this should still wipe Stale.
			TArray<FSFWorldFactSnapshot> Empty;
			Map.RestoreFromSnapshots(Empty, FactBase, NAME_None);

			TestFalse(TEXT("Stale variable wiped by empty restore"), Map.HasVariable(TEXT("Stale")));
		});
	});

	Describe("multi-variable batches", [this]()
	{
		It("round-trips a mixed-type batch", [this]()
		{
			Map.SetInt(TEXT("I"), 10);
			Map.SetFloat(TEXT("F"), 1.25f);
			Map.SetBool(TEXT("B"), true);
			Map.SetName(TEXT("N"), FName(TEXT("Outpost")));

			TArray<FSFWorldFactSnapshot> Snapshots;
			Map.BuildSnapshots(Snapshots, FactBase, NAME_None);
			TestEqual(TEXT("Four snapshots"), Snapshots.Num(), 4);

			FSFNarrativeVariableMap Restored;
			Restored.RestoreFromSnapshots(Snapshots, FactBase, NAME_None);

			int32 IVal = 0;
			float FVal = 0.0f;
			bool BVal = false;
			FName NVal = NAME_None;

			TestTrue(TEXT("Restored I"), Restored.GetInt(TEXT("I"), IVal));
			TestEqual(TEXT("I value"), IVal, 10);

			TestTrue(TEXT("Restored F"), Restored.GetFloat(TEXT("F"), FVal));
			TestEqual(TEXT("F value"), FVal, 1.25f);

			TestTrue(TEXT("Restored B"), Restored.GetBool(TEXT("B"), BVal));
			TestTrue(TEXT("B value"), BVal);

			TestTrue(TEXT("Restored N"), Restored.GetName(TEXT("N"), NVal));
			TestEqual(TEXT("N value"), NVal, FName(TEXT("Outpost")));
		});
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
