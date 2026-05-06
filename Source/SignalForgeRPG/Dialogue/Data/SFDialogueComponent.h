#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "SFDialogueTypes.h"
#include "SFDialogueComponent.generated.h"

class AActor;
class USFConversationDataAsset;
class UAudioComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSFOnConversationStarted, AActor*, SourceActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSFOnConversationEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSFOnDialogueNodeUpdated, const FSFDialogueDisplayData&, DisplayData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSFOnDialogueEventTriggered, FGameplayTag, EventTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSFOnDialogueCameraShotChanged, const FSFDialogueCameraShot&, CameraShot);

UCLASS(ClassGroup = (SignalForge), meta = (BlueprintSpawnableComponent))
class SIGNALFORGERPG_API USFDialogueComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USFDialogueComponent();

    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    bool StartConversation(USFConversationDataAsset* InConversation, AActor* InSourceActor);

    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void AdvanceConversation();

    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    bool SelectChoice(int32 ChoiceIndex);

    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void EndConversation();

    UFUNCTION(BlueprintPure, Category = "Dialogue")
    bool IsConversationActive() const { return bConversationActive; }

    UFUNCTION(BlueprintPure, Category = "Dialogue")
    USFConversationDataAsset* GetActiveConversation() const { return ActiveConversation; }

    UFUNCTION(BlueprintPure, Category = "Dialogue")
    AActor* GetActiveSourceActor() const { return ActiveSourceActor; }

    UFUNCTION(BlueprintPure, Category = "Dialogue")
    FName GetCurrentNodeId() const { return CurrentNodeId; }

    UFUNCTION(BlueprintPure, Category = "Dialogue")
    FSFDialogueDisplayData GetCurrentDisplayData() const;

    // NEW: a tiny helper for UI/replication to know choice count
    UFUNCTION(BlueprintPure, Category = "Dialogue")
    int32 GetVisibleChoiceCount() const { return CurrentVisibleChoiceIndices.Num(); }

    UPROPERTY(BlueprintAssignable, Category = "Dialogue")
    FSFOnConversationStarted OnConversationStarted;

    UPROPERTY(BlueprintAssignable, Category = "Dialogue")
    FSFOnConversationEnded OnConversationEnded;

    UPROPERTY(BlueprintAssignable, Category = "Dialogue")
    FSFOnDialogueNodeUpdated OnDialogueNodeUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Dialogue")
    FSFOnDialogueEventTriggered OnDialogueEventTriggered;

    UPROPERTY(BlueprintAssignable, Category = "Dialogue")
    FSFOnDialogueCameraShotChanged OnDialogueCameraShotChanged;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> ActiveDialogueAudioComponent = nullptr;

    FTimerHandle AutoAdvanceTimerHandle;

protected:
    UPROPERTY(Transient)
    TObjectPtr<USFConversationDataAsset> ActiveConversation = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<AActor> ActiveSourceActor = nullptr;

    UPROPERTY(Transient)
    FName CurrentNodeId = NAME_None;

    UPROPERTY(Transient)
    bool bConversationActive = false;

    UPROPERTY(Transient)
    TArray<int32> CurrentVisibleChoiceIndices;

protected:
    const FSFDialogueNode* GetCurrentNode() const;
    bool MoveToNode(FName NewNodeId);
    void BroadcastCurrentNode();
    void ProcessCurrentNodeChain();
    bool DoesNodePassTagChecks(const FSFDialogueNode& Node) const;
    bool DoesChoicePassTagChecks(const FSFDialogueChoice& Choice) const;
    bool FailConversation(const FString& Reason);
    bool TryResolveNode(FName NodeId, const FSFDialogueNode*& OutNode) const;

    virtual void GetOwnedDialogueTags(FGameplayTagContainer& OutTags) const;
    void ConfigureAdvanceForCurrentNode();
    void ClearAdvanceState();
    float CalculateWordsPerMinuteDelay(const FText& InText, float InWordsPerMinute) const;
    void ApplyConversationStaging();

    UFUNCTION()
    void HandleDialogueVoiceFinished();
};