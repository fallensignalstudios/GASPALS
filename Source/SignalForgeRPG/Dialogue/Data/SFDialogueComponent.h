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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSFOnDialogueChoiceSelected, int32, ChoiceIndex, const FSFDialogueChoice&, Choice);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSFOnDialoguePauseStateChanged, bool, bPaused);

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

    /**
     * Pause the auto-advance timer / audio without ending the conversation.
     * The current node and history are preserved. Use Resume to continue.
     */
    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void PauseConversation();

    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void ResumeConversation();

    UFUNCTION(BlueprintPure, Category = "Dialogue")
    bool IsConversationPaused() const { return bConversationPaused; }

    /**
     * If on a Line node with auto-advance, perform the advance now (skip the
     * remaining delay / audio). On a Choice node this is a no-op. Returns true
     * if a skip actually happened.
     */
    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    bool SkipCurrentLine();

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

    /** Read-only view of past dialogue lines / choices in this conversation. */
    UFUNCTION(BlueprintPure, Category = "Dialogue|History")
    const TArray<FSFDialogueHistoryEntry>& GetDialogueHistory() const { return DialogueHistory; }

    /** Clear the rolling history buffer. */
    UFUNCTION(BlueprintCallable, Category = "Dialogue|History")
    void ClearDialogueHistory() { DialogueHistory.Reset(); }

    /** Capture a snapshot of the active conversation for save games. */
    UFUNCTION(BlueprintCallable, Category = "Dialogue|Snapshot")
    FSFDialogueSnapshot CreateSnapshot() const;

    /**
     * Restore a previously-captured snapshot. The conversation asset will be
     * synchronously loaded if needed. Returns true if the snapshot was valid
     * and the conversation resumed.
     */
    UFUNCTION(BlueprintCallable, Category = "Dialogue|Snapshot")
    bool RestoreFromSnapshot(const FSFDialogueSnapshot& Snapshot, AActor* InSourceActor);

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

    /** Broadcast every time the player picks a choice (after tag checks). */
    UPROPERTY(BlueprintAssignable, Category = "Dialogue")
    FSFOnDialogueChoiceSelected OnDialogueChoiceSelected;

    UPROPERTY(BlueprintAssignable, Category = "Dialogue")
    FSFOnDialoguePauseStateChanged OnDialoguePauseStateChanged;

    /**
     * Maximum entries retained in DialogueHistory. 0 disables history.
     * Old entries are dropped from the front when the buffer exceeds this.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue|History", meta = (ClampMin = "0"))
    int32 MaxHistoryEntries = 256;

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
    bool bConversationPaused = false;

    UPROPERTY(Transient)
    TArray<int32> CurrentVisibleChoiceIndices;

    UPROPERTY(Transient)
    TArray<FSFDialogueHistoryEntry> DialogueHistory;

    /** Remaining time on the auto-advance timer when paused (negative = no timer). */
    UPROPERTY(Transient)
    float PausedAdvanceTimeRemaining = -1.0f;

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
    void PushHistoryForCurrentNode(const FSFDialogueChoice* ChosenChoice);

    UFUNCTION()
    void HandleDialogueVoiceFinished();
};