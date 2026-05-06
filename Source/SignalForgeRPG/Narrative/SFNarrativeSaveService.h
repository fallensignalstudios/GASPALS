#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeSaveService.generated.h"

class USFNarrativeComponent;
class USFQuestRuntime;
class USFWorldStateRuntime;
class USFFactionRuntime;
class USFIdentityRuntime;
class USFOutcomeRuntime;
class USFEndingRuntime;

UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFNarrativeSaveService : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Narrative|Save")
    virtual bool Initialize(
        USFNarrativeComponent* InOwner,
        USFQuestRuntime* InQuestRuntime,
        USFWorldStateRuntime* InWorldStateRuntime,
        USFFactionRuntime* InFactionRuntime,
        USFIdentityRuntime* InIdentityRuntime,
        USFOutcomeRuntime* InOutcomeRuntime,
        USFEndingRuntime* InEndingRuntime);

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

protected:
    UPROPERTY(Transient)
    TObjectPtr<USFNarrativeComponent> OwnerComponent = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USFQuestRuntime> QuestRuntime = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USFWorldStateRuntime> WorldStateRuntime = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USFFactionRuntime> FactionRuntime = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USFIdentityRuntime> IdentityRuntime = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USFOutcomeRuntime> OutcomeRuntime = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USFEndingRuntime> EndingRuntime = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Narrative|Save")
    int32 CurrentSchemaVersion = 2;
};