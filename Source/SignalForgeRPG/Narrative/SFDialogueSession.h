//#pragma once
//
//#include "CoreMinimal.h"
//#include "UObject/Object.h"
//#include "SFNarrativeTypes.h"
//#include "SFDialogueSession.generated.h"
//
//class APlayerState;
//class USFNarrativeComponent;
//class USFDialogueDefinition;
//
//UCLASS(BlueprintType)
//class SIGNALFORGERPG_API USFDialogueSession : public UObject
//{
//    GENERATED_BODY()
//
//public:
//    UFUNCTION(BlueprintCallable, Category = "Narrative|Dialogue")
//    virtual bool Initialize(USFNarrativeComponent* InOwner, const USFDialogueDefinition* InDefinition, const FSFDialogueStartParams& Params);
//
//    UFUNCTION(BlueprintCallable, Category = "Narrative|Dialogue")
//    virtual bool Play();
//
//    UFUNCTION(BlueprintPure, Category = "Narrative|Dialogue")
//    virtual bool IsActive() const;
//
//    UFUNCTION(BlueprintPure, Category = "Narrative|Dialogue")
//    virtual bool CanSelectOption(FName OptionId) const;
//
//    UFUNCTION(BlueprintCallable, Category = "Narrative|Dialogue")
//    virtual bool SelectOption(FName OptionId, APlayerState* Selector);
//
//    UFUNCTION(BlueprintPure, Category = "Narrative|Dialogue")
//    virtual bool CanSkipCurrentLine() const;
//
//    UFUNCTION(BlueprintCallable, Category = "Narrative|Dialogue")
//    virtual bool SkipCurrentLine();
//
//    UFUNCTION(BlueprintPure, Category = "Narrative|Dialogue")
//    virtual bool CanExit() const;
//
//    UFUNCTION(BlueprintCallable, Category = "Narrative|Dialogue")
//    virtual void Exit(ESFDialogueExitReason Reason);
//
//    UFUNCTION(BlueprintPure, Category = "Narrative|Dialogue")
//    virtual ESFDialogueExitReason GetLastExitReason() const;
//
//    UFUNCTION(BlueprintPure, Category = "Narrative|Dialogue")
//    virtual const FSFDialogueChunk& GetCurrentChunk() const;
//
//    UFUNCTION(BlueprintPure, Category = "Narrative|Dialogue")
//    virtual const USFDialogueDefinition* GetDefinition() const;
//
//protected:
//    const FSFDialogueNodeDefinition* FindNodeDefinition(FName NodeId) const;
//    bool RebuildChunkFromNode(FName NodeId);
//    bool AdvanceFromNode(const FSFDialogueNodeDefinition& NodeDef);
//
//protected:
//    UPROPERTY(Transient)
//    TObjectPtr<USFNarrativeComponent> OwnerComponent = nullptr;
//
//    UPROPERTY(Transient)
//    TObjectPtr<const USFDialogueDefinition> Definition = nullptr;
//
//    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Narrative|Dialogue")
//    FSFDialogueStartParams StartParams;
//
//    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Narrative|Dialogue")
//    FSFDialogueChunk CurrentChunk;
//
//    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Narrative|Dialogue")
//    FName CurrentNodeId = NAME_None;
//
//    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Narrative|Dialogue")
//    bool bIsActive = false;
//
//    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Narrative|Dialogue")
//    ESFDialogueExitReason LastExitReason = ESFDialogueExitReason::Completed;
//};