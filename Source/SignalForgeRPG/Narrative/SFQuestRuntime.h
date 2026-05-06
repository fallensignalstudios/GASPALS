// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "UObject/PrimaryAssetId.h"
#include "SFNarrativeTypes.h"
#include "SFQuestRuntime.generated.h"

class USFNarrativeComponent;
class USFQuestDefinition;
class USFQuestInstance;

UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFQuestRuntime : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Narrative|Quest")
    virtual bool Initialize(USFNarrativeComponent* InOwner);

    //
    // Primary, asset/definition‑driven APIs
    //

    UFUNCTION(BlueprintCallable, Category = "Narrative|Quest")
    virtual USFQuestInstance* StartQuestByDefinition(USFQuestDefinition* QuestDef, FName StartStateId = NAME_None);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Quest")
    virtual bool RestartQuestByDefinition(USFQuestDefinition* QuestDef, FName StartStateId = NAME_None);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Quest")
    virtual bool AbandonQuestByDefinition(USFQuestDefinition* QuestDef);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Quest")
    virtual bool EnterQuestStateByDefinition(USFQuestDefinition* QuestDef, FName StateId, bool bFromReplication = false);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Quest")
    virtual USFQuestInstance* GetQuestInstanceByAssetId(FPrimaryAssetId QuestAssetId) const;

    //
    // Class‑based wrappers (for BP ergonomics)
    //

    UFUNCTION(BlueprintCallable, Category = "Narrative|Quest")
    virtual USFQuestInstance* StartQuest(TSubclassOf<USFQuestDefinition> QuestDefClass, FName StartStateId = NAME_None);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Quest")
    virtual bool RestartQuest(TSubclassOf<USFQuestDefinition> QuestDefClass, FName StartStateId = NAME_None);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Quest")
    virtual bool AbandonQuest(TSubclassOf<USFQuestDefinition> QuestDefClass);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Quest")
    virtual bool EnterQuestState(TSubclassOf<USFQuestDefinition> QuestDefClass, FName StateId, bool bFromReplication = false);

    UFUNCTION(BlueprintPure, Category = "Narrative|Quest")
    virtual USFQuestInstance* GetQuestInstance(TSubclassOf<USFQuestDefinition> QuestDefClass) const;

    //
    // Task + queries
    //

    UFUNCTION(BlueprintCallable, Category = "Narrative|Quest")
    virtual bool ApplyTaskProgress(FGameplayTag TaskTag, FName ContextId, int32 Quantity, bool bFromReplication = false);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Quest")
    virtual bool ApplyReplicatedTaskProgress(FName QuestId, FName TaskId, int32 NewProgress, int32 DeltaAmount);

    UFUNCTION(BlueprintPure, Category = "Narrative|Quest")
    virtual TArray<USFQuestInstance*> GetAllQuestInstances() const;

    UFUNCTION(BlueprintPure, Category = "Narrative|Quest")
    virtual int32 GetLifetimeTaskCount(FGameplayTag TaskTag, FName ContextId = NAME_None) const;

    //
    // Save/load
    //

    UFUNCTION(BlueprintPure, Category = "Narrative|Save")
    virtual FSFNarrativeSaveData BuildSaveData() const;

    UFUNCTION(BlueprintCallable, Category = "Narrative|Save")
    virtual bool LoadFromSaveData(const FSFNarrativeSaveData& SaveData);

protected:
    USFQuestDefinition* ResolveQuestDefinitionFromClass(TSubclassOf<USFQuestDefinition> QuestDefClass) const;
    USFQuestDefinition* ResolveQuestDefinitionByAssetId(FPrimaryAssetId QuestAssetId) const;
    FPrimaryAssetId GetQuestAssetId(const USFQuestDefinition* QuestDef) const;
    void IncrementLifetimeTaskCount(FGameplayTag TaskTag, FName ContextId, int32 Quantity);

protected:
    UPROPERTY(Transient)
    TObjectPtr<USFNarrativeComponent> OwnerComponent = nullptr;

    UPROPERTY(Transient)
    TMap<FPrimaryAssetId, TObjectPtr<USFQuestInstance>> QuestInstancesByAssetId;

    UPROPERTY(Transient)
    TMap<FSFTaskProgressKey, int32> LifetimeTaskCounts;
};
