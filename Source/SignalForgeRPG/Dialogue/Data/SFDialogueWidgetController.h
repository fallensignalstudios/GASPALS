#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "Dialogue/Data/SFDialogueTypes.h"
#include "SFDialogueWidgetController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueActiveChangedSignature, bool, bIsActive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueDisplayDataChangedSignature, const FSFDialogueDisplayData&, DisplayData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialoguePauseChangedSignature, bool, bIsPaused);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueEventForwardedSignature, FGameplayTag, EventTag);

class USFDialogueComponent;
class AActor;

UCLASS(BlueprintType, Blueprintable)
class SIGNALFORGERPG_API USFDialogueWidgetController : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Bind this controller to a player-avatar actor. The actor must implement
	 * ISFPlayerAvatarInterface (both protagonists do). Accepts AActor* so the
	 * second protagonist works without recompiling Blueprint callers.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void Initialize(AActor* InPlayerAvatar);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void AdvanceDialogue();

	UFUNCTION(BlueprintCallable, Category = "UI")
	bool SelectChoice(int32 ChoiceIndex);

	UFUNCTION(BlueprintCallable, Category = "UI")
	bool SkipCurrentLine();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void PauseDialogue();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ResumeDialogue();

	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsDialogueActive() const { return bIsDialogueActive; }

	UFUNCTION(BlueprintPure, Category = "UI")
	FName GetCurrentSpeakerId() const { return CurrentDisplayData.SpeakerId; }

	UFUNCTION(BlueprintPure, Category = "UI")
	FText GetCurrentLineText() const { return CurrentDisplayData.LineText; }

	UFUNCTION(BlueprintPure, Category = "UI")
	TArray<FSFDialogueChoice> GetCurrentChoices() const { return CurrentDisplayData.Choices; }

	UFUNCTION(BlueprintPure, Category = "UI")
	ESFDialogueNodeType GetCurrentNodeType() const { return CurrentDisplayData.NodeType; }

	UFUNCTION(BlueprintPure, Category = "UI")
	int32 GetCurrentChoiceCount() const { return CurrentDisplayData.Choices.Num(); }

	/** The avatar actor we're bound to. Read-only on BP side; implements ISFPlayerAvatarInterface. */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<AActor> PlayerAvatar;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<USFDialogueComponent> DialogueComponent;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	bool bIsDialogueActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	FSFDialogueDisplayData CurrentDisplayData;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnDialogueActiveChangedSignature OnDialogueActiveChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnDialogueDisplayDataChangedSignature OnDialogueDisplayDataChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnDialoguePauseChangedSignature OnDialoguePauseChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnDialogueEventForwardedSignature OnDialogueEventForwarded;

protected:
	UFUNCTION()
	void HandleConversationStarted(AActor* SourceActor);

	UFUNCTION()
	void HandleConversationEnded();

	UFUNCTION()
	void HandleDialogueNodeUpdated(const FSFDialogueDisplayData& DisplayData);

	UFUNCTION()
	void HandlePauseStateChanged(bool bPaused);

	UFUNCTION()
	void HandleDialogueEvent(FGameplayTag EventTag);
};