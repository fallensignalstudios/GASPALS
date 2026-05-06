#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Dialogue/Data/SFDialogueTypes.h"
#include "SFDialogueWidgetController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueActiveChangedSignature, bool, bIsActive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueDisplayDataChangedSignature, const FSFDialogueDisplayData&, DisplayData);

class ASFPlayerCharacter;
class USFDialogueComponent;
class AActor;

UCLASS(BlueprintType, Blueprintable)
class SIGNALFORGERPG_API USFDialogueWidgetController : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UI")
	void Initialize(ASFPlayerCharacter* InPlayerCharacter);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void AdvanceDialogue();

	UFUNCTION(BlueprintCallable, Category = "UI")
	bool SelectChoice(int32 ChoiceIndex);

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

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<ASFPlayerCharacter> PlayerCharacter;

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

protected:
	UFUNCTION()
	void HandleConversationStarted(AActor* SourceActor);

	UFUNCTION()
	void HandleConversationEnded();

	UFUNCTION()
	void HandleDialogueNodeUpdated(const FSFDialogueDisplayData& DisplayData);
};