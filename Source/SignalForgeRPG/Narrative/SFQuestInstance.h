#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "SFNarrativeTypes.h"
#include "SFQuestInstance.generated.h"

class USFNarrativeComponent;
class USFQuestDefinition;

UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFQuestInstance : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Narrative|Quest")
    virtual bool Initialize(USFNarrativeComponent* InOwner, const USFQuestDefinition* InDefinition, FName StartStateId = NAME_None);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Quest")
    virtual bool RestoreFromSnapshot(USFNarrativeComponent* InOwner, const USFQuestDefinition* InDefinition, const FSFQuestSnapshot& Snapshot);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Quest")
    virtual bool EnterState(FName StateId, bool bFromReplication = false);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Quest")
    virtual bool ApplyTaskProgress(FGameplayTag TaskTag, FName ContextId, int32 Quantity, bool bFromReplication = false);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Quest")
    virtual bool ApplyReplicatedTaskProgressByTaskId(FName TaskId, int32 NewProgress, int32 DeltaAmount);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Quest")
    virtual void MarkAbandoned();

    UFUNCTION(BlueprintPure, Category = "Narrative|Quest")
    virtual bool IsFinished() const;

    UFUNCTION(BlueprintPure, Category = "Narrative|Quest")
    virtual bool IsSucceeded() const;

    UFUNCTION(BlueprintPure, Category = "Narrative|Quest")
    virtual bool IsFailed() const;

    UFUNCTION(BlueprintPure, Category = "Narrative|Quest")
    virtual ESFQuestCompletionState GetCompletionState() const;

    UFUNCTION(BlueprintPure, Category = "Narrative|Quest")
    virtual FName GetCurrentStateId() const;

    UFUNCTION(BlueprintPure, Category = "Narrative|Quest")
    virtual const USFQuestDefinition* GetDefinition() const;

    UFUNCTION(BlueprintPure, Category = "Narrative|Quest")
    virtual const TMap<FName, int32>& GetTaskProgressByTaskId() const;

    UFUNCTION(BlueprintPure, Category = "Narrative|Quest")
    virtual FSFQuestSnapshot BuildSnapshot() const;

protected:
    const FSFQuestStateDefinition* FindStateDefinition(FName StateId) const;
    bool AreAllTasksCompleteInState(const FSFQuestStateDefinition& StateDef) const;
    void RefreshCompletionStateFromStateDefinition(const FSFQuestStateDefinition& StateDef);
    int32 GetTaskProgress(FName TaskId) const;

protected:
    UPROPERTY(Transient)
    TObjectPtr<USFNarrativeComponent> OwnerComponent = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<const USFQuestDefinition> Definition = nullptr;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Narrative|Quest")
    ESFQuestCompletionState CompletionState = ESFQuestCompletionState::NotStarted;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Narrative|Quest")
    FName CurrentStateId = NAME_None;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Narrative|Quest")
    TArray<FName> ReachedStateIds;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Narrative|Quest")
    TMap<FName, int32> TaskProgressByTaskId;
};