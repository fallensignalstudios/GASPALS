#include "SFNarrativeRestoreHelpers.h"

#include "SFNarrativeStateSubsystem.h"
#include "SFNarrativeWorldState.h"
#include "SFQuestInstance.h"
#include "SFQuestDefinition.h"
#include "SFQuestRuntime.h"

bool USFNarrativeRestoreHelpers::RestoreWorldFromSaveData(
    const UObject* WorldContextObject,
    const FSFNarrativeSaveData& SaveData)
{
    if (!WorldContextObject)
    {
        return false;
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return false;
    }

    USFNarrativeStateSubsystem* StateSubsystem = World->GetSubsystem<USFNarrativeStateSubsystem>();
    if (!StateSubsystem)
    {
        return false;
    }

    // Directly feed the world portion of the save data back into the subsystem.
    return StateSubsystem->LoadFromSaveData(SaveData);
}

bool USFNarrativeRestoreHelpers::RestoreQuestInstanceFromSnapshot(
    const FSFQuestSnapshot& Snapshot,
    const USFQuestDefinition* QuestDef,
    USFQuestInstance* QuestInstance)
{
    if (!QuestDef || !QuestInstance)
    {
        return false;
    }

    // Basic consistency check.
    if (Snapshot.QuestAssetId != QuestDef->GetPrimaryAssetId())
    {
        // This snapshot is for a different quest; ignore.
        return false;
    }

    // Assuming USFQuestInstance exposes RestoreFromSnapshot.
    QuestInstance->RestoreFromSnapshot(Snapshot);
    return true;
}

void USFNarrativeRestoreHelpers::RestoreQuestRuntimeFromSaveData(
    const FSFNarrativeSaveData& SaveData,
    USFQuestRuntime* QuestRuntime)
{
    if (!QuestRuntime)
    {
        return;
    }

    // Clear any existing runtime state.
    QuestRuntime->ResetRuntime();

    // Let the runtime rehydrate its quest instances from snapshots.
    for (const FSFQuestSnapshot& Snapshot : SaveData.QuestSnapshots)
    {
        QuestRuntime->RestoreQuestFromSnapshot(Snapshot);
    }

    // Lifetime task counters.
    QuestRuntime->ClearLifetimeTaskCounters();
    for (const FSFTaskCounterSnapshot& CounterSnapshot : SaveData.LifetimeTaskCounters)
    {
        QuestRuntime->AddLifetimeTaskCount(
            CounterSnapshot.Key,
            CounterSnapshot.Count);
    }
}

bool USFNarrativeRestoreHelpers::DecodeNarrativeSnapshotFromOwnerPayload(
    const FSFNarrativeOwnerSnapshot& OwnerSnapshot,
    FSFNarrativeSnapshot& OutSnapshot)
{
    OutSnapshot.Reset();
    OutSnapshot.OwnerId = OwnerSnapshot.OwnerId;
    OutSnapshot.OwnerTags = OwnerSnapshot.PayloadTags;

    if (OwnerSnapshot.PayloadData.Num() == 0)
    {
        // No payload; treat as empty snapshot.
        return false;
    }

    FMemoryReader Reader(OwnerSnapshot.PayloadData, /*bIsPersistent=*/ true);
    Reader.SetByteSwapping(PLATFORM_LITTLE_ENDIAN == 0);

    // Simple versioned binary format: [uint32 Version][UScriptStruct serialized FSFNarrativeSnapshot]
    uint32 Version = 0;
    Reader << Version;

    if (Version == 0)
    {
        // Unknown / unsupported version.
        return false;
    }

    // Let Unreal handle struct serialization.
    OutSnapshot.StructSerializeFromMismatchedTag(FStructuredArchiveFromArchive(Reader).GetSlot(), nullptr);

    return true;
}

void USFNarrativeRestoreHelpers::EncodeNarrativeSnapshotToOwnerPayload(
    const FSFNarrativeSnapshot& Snapshot,
    FSFNarrativeOwnerSnapshot& OutOwnerSnapshot)
{
    OutOwnerSnapshot.OwnerId = Snapshot.OwnerId;
    OutOwnerSnapshot.PayloadTags = Snapshot.OwnerTags;

    OutOwnerSnapshot.PayloadData.Reset();

    FMemoryWriter Writer(OutOwnerSnapshot.PayloadData, /*bIsPersistent=*/ true);
    Writer.SetByteSwapping(PLATFORM_LITTLE_ENDIAN == 0);

    const uint32 Version = 1;
    Writer << const_cast<uint32&>(Version);

    // Serialize the struct.
    const_cast<FSFNarrativeSnapshot&>(Snapshot).StructSerialize(FStructuredArchiveFromArchive(Writer).GetSlot());
}

void USFNarrativeRestoreHelpers::RestoreFlagsAndVariablesFromSnapshot(
    const FSFNarrativeSnapshot& Snapshot,
    FSFNarrativeFlagMap& OutFlags,
    FSFNarrativeVariableMap& OutVariables)
{
    OutFlags = Snapshot.LocalFlags;
    OutVariables = Snapshot.LocalVariables;
}