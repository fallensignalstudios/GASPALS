// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "SFNarrativeRestoreHelpers.h"

#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "UObject/UnrealType.h" // UScriptStruct::SerializeItem

#include "SFNarrativeComponent.h"
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

    // USFQuestInstance::RestoreFromSnapshot expects (Owner, Definition, Snapshot).
    // Pass the QuestInstance's outer NarrativeComponent if available.
    USFNarrativeComponent* Owner = QuestInstance->GetTypedOuter<USFNarrativeComponent>();
    return QuestInstance->RestoreFromSnapshot(Owner, QuestDef, Snapshot);
}

void USFNarrativeRestoreHelpers::RestoreQuestRuntimeFromSaveData(
    const FSFNarrativeSaveData& SaveData,
    USFQuestRuntime* QuestRuntime)
{
    if (!QuestRuntime)
    {
        return;
    }

    // USFQuestRuntime exposes LoadFromSaveData as the single canonical entry
    // point that handles quest snapshots, lifetime task counters, and any
    // other save-time state in one transactional call.
    QuestRuntime->LoadFromSaveData(SaveData);
}

bool USFNarrativeRestoreHelpers::DecodeNarrativeSnapshotFromOwnerPayload(
    const FSFNarrativeOwnerSnapshot& OwnerSnapshot,
    FSFNarrativeSnapshot& OutSnapshot)
{
    // FSFNarrativeSnapshot has no Reset() method; assignment to a default
    // instance is the canonical way to clear it.
    OutSnapshot = FSFNarrativeSnapshot{};
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

    // Use UScriptStruct::SerializeItem to (de)serialize the FSFNarrativeSnapshot.
    // FSFNarrativeSnapshot itself doesn't expose a struct serializer hook, so
    // we drive it via its UScriptStruct.
    if (UScriptStruct* SnapshotStruct = FSFNarrativeSnapshot::StaticStruct())
    {
        SnapshotStruct->SerializeItem(Reader, &OutSnapshot, /*Defaults=*/ nullptr);
        return true;
    }

    return false;
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

    // Serialize the struct via its UScriptStruct so we don't depend on a
    // member-level serializer hook on FSFNarrativeSnapshot.
    if (UScriptStruct* SnapshotStruct = FSFNarrativeSnapshot::StaticStruct())
    {
        SnapshotStruct->SerializeItem(
            Writer,
            const_cast<FSFNarrativeSnapshot*>(&Snapshot),
            /*Defaults=*/ nullptr);
    }
}

void USFNarrativeRestoreHelpers::RestoreFlagsAndVariablesFromSnapshot(
    const FSFNarrativeSnapshot& Snapshot,
    FSFNarrativeFlagMap& OutFlags,
    FSFNarrativeVariableMap& OutVariables)
{
    OutFlags = Snapshot.LocalFlags;
    OutVariables = Snapshot.LocalVariables;
}
