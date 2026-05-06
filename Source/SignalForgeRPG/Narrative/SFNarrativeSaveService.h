// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeSaveService.generated.h"

class USFNarrativeComponent;
class USFQuestRuntime;
class USFNarrativeStateSubsystem;

/**
 * Top-level orchestrator for narrative save/load.
 *
 * Combines per-owner data from USFQuestRuntime with world-scoped data
 * from USFNarrativeStateSubsystem (world facts, factions, identity axes,
 * outcomes, endings) into a single FSFNarrativeSaveData payload.
 *
 * Design notes:
 *  - The SaveService no longer requires individual *Runtime classes for
 *    each subsystem domain. The narrative state subsystem already holds
 *    the canonical world-scoped state and exposes BuildSaveData() /
 *    LoadFromSaveData(), so we delegate to it directly.
 *  - QuestRuntime is per-owner (per character/component) and stays as
 *    an explicit dependency.
 */
UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFNarrativeSaveService : public UObject
{
    GENERATED_BODY()

public:
    /**
     * Wire up the save service to its owning narrative component and the
     * per-owner quest runtime. The world-scoped subsystem is resolved
     * lazily through the owner's world.
     */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Save")
    virtual bool Initialize(
        USFNarrativeComponent* InOwner,
        USFQuestRuntime* InQuestRuntime);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Save")
    virtual bool SaveToSlot(const FString& SlotName, int32 SlotIndex = 0);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Save")
    virtual bool LoadFromSlot(const FString& SlotName, int32 SlotIndex = 0);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Save")
    virtual bool DeleteSlot(const FString& SlotName, int32 SlotIndex = 0);

    UFUNCTION(BlueprintPure, Category = "Narrative|Save")
    virtual int32 GetCurrentSchemaVersion() const;

    UFUNCTION(BlueprintPure, Category = "Narrative|Save")
    virtual FSFNarrativeSaveData BuildSaveData() const;

    UFUNCTION(BlueprintCallable, Category = "Narrative|Save")
    virtual bool ApplySaveData(const FSFNarrativeSaveData& SaveData);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Save")
    virtual bool MigrateSaveData(FSFNarrativeSaveData& InOutSaveData) const;

protected:
    void BroadcastBeginSave(const FString& SlotName) const;
    void BroadcastCompleteSave(const FString& SlotName) const;
    void BroadcastBeginLoad(const FString& SlotName) const;
    void BroadcastBeforeApplyLoadedState(const FString& SlotName) const;
    void BroadcastAfterApplyLoadedState(const FString& SlotName) const;
    void BroadcastCompleteLoad(const FString& SlotName) const;

    bool CanApplySchema(int32 SchemaVersion) const;
    bool ValidateDependencies() const;

    /** Resolve the narrative state subsystem from the owner's world. */
    USFNarrativeStateSubsystem* GetStateSubsystem() const;

protected:
    UPROPERTY(Transient)
    TObjectPtr<USFNarrativeComponent> OwnerComponent = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USFQuestRuntime> QuestRuntime = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Narrative|Save")
    int32 CurrentSchemaVersion = 2;
};
