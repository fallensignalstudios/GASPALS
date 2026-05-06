#include "SFNarrativeSaveService.h"

#include "Kismet/GameplayStatics.h"
#include "SFEndingRuntime.h"
#include "SFFactionRuntime.h"
#include "SFIdentityRuntime.h"
#include "SFNarrativeComponent.h"
#include "SFNarrativeEventHub.h"
#include "SFNarrativeSaveGame.h"
#include "SFOutcomeRuntime.h"
#include "SFQuestRuntime.h"
#include "SFWorldStateRuntime.h"

bool USFNarrativeSaveService::Initialize(
    USFNarrativeComponent* InOwner,
    USFQuestRuntime* InQuestRuntime,
    USFWorldStateRuntime* InWorldStateRuntime,
    USFFactionRuntime* InFactionRuntime,
    USFIdentityRuntime* InIdentityRuntime,
    USFOutcomeRuntime* InOutcomeRuntime,
    USFEndingRuntime* InEndingRuntime)
{
    OwnerComponent = InOwner;
    QuestRuntime = InQuestRuntime;
    WorldStateRuntime = InWorldStateRuntime;
    FactionRuntime = InFactionRuntime;
    IdentityRuntime = InIdentityRuntime;
    OutcomeRuntime = InOutcomeRuntime;
    EndingRuntime = InEndingRuntime;

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

    SaveObject->Data = BuildSaveData();

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

    FSFNarrativeSaveData SaveData = SaveObject->Data;
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

    if (QuestRuntime)
    {
        const FSFNarrativeSaveData QuestData = QuestRuntime->BuildSaveData();
        SaveData.QuestSnapshots = QuestData.QuestSnapshots;
        SaveData.LifetimeTaskCounters = QuestData.LifetimeTaskCounters;
    }

    if (WorldStateRuntime)
    {
        SaveData.WorldFacts = WorldStateRuntime->BuildWorldFactSnapshots();
    }

    if (FactionRuntime)
    {
        SaveData.FactionSnapshots = FactionRuntime->BuildFactionSnapshots();
    }

    if (IdentityRuntime)
    {
        SaveData.IdentityAxes = IdentityRuntime->BuildIdentitySnapshots();
    }

    if (OutcomeRuntime)
    {
        SaveData.AppliedOutcomes = OutcomeRuntime->BuildOutcomeSnapshots();
    }

    if (EndingRuntime)
    {
        SaveData.EndingStates = EndingRuntime->BuildEndingSnapshots();
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

    if (WorldStateRuntime)
    {
        WorldStateRuntime->LoadFromSnapshots(SaveData.WorldFacts);
    }

    if (FactionRuntime)
    {
        FactionRuntime->LoadFromSnapshots(SaveData.FactionSnapshots);
    }

    if (IdentityRuntime)
    {
        IdentityRuntime->LoadFromSnapshots(SaveData.IdentityAxes);
    }

    if (OutcomeRuntime)
    {
        OutcomeRuntime->LoadFromSnapshots(SaveData.AppliedOutcomes);
    }

    if (EndingRuntime)
    {
        EndingRuntime->LoadFromSnapshots(SaveData.EndingStates);
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
        && QuestRuntime != nullptr
        && WorldStateRuntime != nullptr
        && FactionRuntime != nullptr
        && IdentityRuntime != nullptr
        && OutcomeRuntime != nullptr
        && EndingRuntime != nullptr;
}