// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "SFNarrativeSaveService.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "SFNarrativeComponent.h"
#include "SFNarrativeEventHub.h"
#include "SFNarrativeSaveGame.h"
#include "SFNarrativeStateSubsystem.h"
#include "SFQuestRuntime.h"

bool USFNarrativeSaveService::Initialize(
    USFNarrativeComponent* InOwner,
    USFQuestRuntime* InQuestRuntime)
{
    OwnerComponent = InOwner;
    QuestRuntime = InQuestRuntime;

    return ValidateDependencies();
}

bool USFNarrativeSaveService::SaveToSlot(const FString& SlotName, int32 SlotIndex)
{
    if (!ValidateDependencies() || SlotName.IsEmpty())
    {
        return false;
    }

    BroadcastBeginSave(SlotName);

    USFNarrativeSaveGame* SaveObject = Cast<USFNarrativeSaveGame>(
        UGameplayStatics::CreateSaveGameObject(USFNarrativeSaveGame::StaticClass()));

    if (!SaveObject)
    {
        return false;
    }

    SaveObject->NarrativeData = BuildSaveData();
    SaveObject->SlotName = SlotName;
    SaveObject->UserIndex = SlotIndex;
    SaveObject->NarrativeVersion = CurrentSchemaVersion;
    SaveObject->SaveTimestamp = FDateTime::UtcNow();

    const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveObject, SlotName, SlotIndex);
    if (bSaved)
    {
        BroadcastCompleteSave(SlotName);
    }

    return bSaved;
}

bool USFNarrativeSaveService::LoadFromSlot(const FString& SlotName, int32 SlotIndex)
{
    if (!ValidateDependencies() || SlotName.IsEmpty() || !UGameplayStatics::DoesSaveGameExist(SlotName, SlotIndex))
    {
        return false;
    }

    BroadcastBeginLoad(SlotName);

    USFNarrativeSaveGame* SaveObject = Cast<USFNarrativeSaveGame>(
        UGameplayStatics::LoadGameFromSlot(SlotName, SlotIndex));

    if (!SaveObject)
    {
        return false;
    }

    FSFNarrativeSaveData SaveData = SaveObject->NarrativeData;
    if (!CanApplySchema(SaveData.SchemaVersion))
    {
        return false;
    }

    if (!MigrateSaveData(SaveData))
    {
        return false;
    }

    BroadcastBeforeApplyLoadedState(SlotName);

    const bool bApplied = ApplySaveData(SaveData);
    if (bApplied)
    {
        BroadcastAfterApplyLoadedState(SlotName);
        BroadcastCompleteLoad(SlotName);
    }

    return bApplied;
}

bool USFNarrativeSaveService::DeleteSlot(const FString& SlotName, int32 SlotIndex)
{
    if (SlotName.IsEmpty())
    {
        return false;
    }

    return UGameplayStatics::DeleteGameInSlot(SlotName, SlotIndex);
}

int32 USFNarrativeSaveService::GetCurrentSchemaVersion() const
{
    return CurrentSchemaVersion;
}

FSFNarrativeSaveData USFNarrativeSaveService::BuildSaveData() const
{
    FSFNarrativeSaveData SaveData;
    SaveData.SchemaVersion = CurrentSchemaVersion;

    if (!ValidateDependencies())
    {
        return SaveData;
    }

    // Per-owner quest data.
    if (QuestRuntime)
    {
        const FSFNarrativeSaveData QuestData = QuestRuntime->BuildSaveData();
        SaveData.QuestSnapshots = QuestData.QuestSnapshots;
        SaveData.LifetimeTaskCounters = QuestData.LifetimeTaskCounters;
    }

    // World-scoped data: facts, factions, identity, outcomes, endings.
    if (USFNarrativeStateSubsystem* StateSubsystem = GetStateSubsystem())
    {
        const FSFNarrativeSaveData WorldData = StateSubsystem->BuildSaveData();
        SaveData.WorldFacts = WorldData.WorldFacts;
        SaveData.FactionSnapshots = WorldData.FactionSnapshots;
        SaveData.IdentityAxes = WorldData.IdentityAxes;
        SaveData.AppliedOutcomes = WorldData.AppliedOutcomes;
        SaveData.EndingStates = WorldData.EndingStates;
    }

    return SaveData;
}

bool USFNarrativeSaveService::ApplySaveData(const FSFNarrativeSaveData& SaveData)
{
    if (!ValidateDependencies() || !CanApplySchema(SaveData.SchemaVersion))
    {
        return false;
    }

    if (QuestRuntime)
    {
        QuestRuntime->LoadFromSaveData(SaveData);
    }

    if (USFNarrativeStateSubsystem* StateSubsystem = GetStateSubsystem())
    {
        StateSubsystem->LoadFromSaveData(SaveData);
    }

    return true;
}

bool USFNarrativeSaveService::MigrateSaveData(FSFNarrativeSaveData& InOutSaveData) const
{
    if (!CanApplySchema(InOutSaveData.SchemaVersion))
    {
        return false;
    }

    if (InOutSaveData.SchemaVersion == CurrentSchemaVersion)
    {
        return true;
    }

    while (InOutSaveData.SchemaVersion < CurrentSchemaVersion)
    {
        switch (InOutSaveData.SchemaVersion)
        {
        case 1:
            // V1 -> V2: world-scoped state moved into the canonical model.
            // Drop any legacy world data; gameplay code rebuilds at runtime.
            InOutSaveData.WorldFacts.Reset();
            InOutSaveData.FactionSnapshots.Reset();
            InOutSaveData.IdentityAxes.Reset();
            InOutSaveData.AppliedOutcomes.Reset();
            InOutSaveData.EndingStates.Reset();
            InOutSaveData.SchemaVersion = 2;
            break;

        default:
            return false;
        }
    }

    return InOutSaveData.SchemaVersion == CurrentSchemaVersion;
}

void USFNarrativeSaveService::BroadcastBeginSave(const FString& SlotName) const
{
    if (!OwnerComponent)
    {
        return;
    }

    if (USFNarrativeEventHub* Hub = OwnerComponent->GetEventHub())
    {
        Hub->BroadcastBeginSave(SlotName);
    }
}

void USFNarrativeSaveService::BroadcastCompleteSave(const FString& SlotName) const
{
    if (!OwnerComponent)
    {
        return;
    }

    if (USFNarrativeEventHub* Hub = OwnerComponent->GetEventHub())
    {
        Hub->BroadcastCompleteSave(SlotName);
    }
}

void USFNarrativeSaveService::BroadcastBeginLoad(const FString& SlotName) const
{
    if (!OwnerComponent)
    {
        return;
    }

    if (USFNarrativeEventHub* Hub = OwnerComponent->GetEventHub())
    {
        Hub->BroadcastBeginLoad(SlotName);
    }
}

void USFNarrativeSaveService::BroadcastBeforeApplyLoadedState(const FString& SlotName) const
{
    if (!OwnerComponent)
    {
        return;
    }

    if (USFNarrativeEventHub* Hub = OwnerComponent->GetEventHub())
    {
        Hub->BroadcastBeforeApplyLoadedState(SlotName);
    }
}

void USFNarrativeSaveService::BroadcastAfterApplyLoadedState(const FString& SlotName) const
{
    if (!OwnerComponent)
    {
        return;
    }

    if (USFNarrativeEventHub* Hub = OwnerComponent->GetEventHub())
    {
        Hub->BroadcastAfterApplyLoadedState(SlotName);
    }
}

void USFNarrativeSaveService::BroadcastCompleteLoad(const FString& SlotName) const
{
    if (!OwnerComponent)
    {
        return;
    }

    if (USFNarrativeEventHub* Hub = OwnerComponent->GetEventHub())
    {
        Hub->BroadcastCompleteLoad(SlotName);
    }
}

bool USFNarrativeSaveService::CanApplySchema(int32 SchemaVersion) const
{
    return SchemaVersion > 0 && SchemaVersion <= CurrentSchemaVersion;
}

bool USFNarrativeSaveService::ValidateDependencies() const
{
    return OwnerComponent != nullptr
        && QuestRuntime != nullptr;
}

USFNarrativeStateSubsystem* USFNarrativeSaveService::GetStateSubsystem() const
{
    if (!OwnerComponent)
    {
        return nullptr;
    }

    UWorld* World = OwnerComponent->GetWorld();
    if (!World)
    {
        return nullptr;
    }

    return World->GetSubsystem<USFNarrativeStateSubsystem>();
}
