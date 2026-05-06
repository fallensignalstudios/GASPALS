#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Dialogue/SFDialogueCameraTypes.h"
#include "Sound/SoundBase.h"
#include "SFDialogueTypes.generated.h"

UENUM(BlueprintType)
enum class ESFDialogueNodeType : uint8
{
	Line	UMETA(DisplayName = "Line"),
	Choice	UMETA(DisplayName = "Choice"),
	Event	UMETA(DisplayName = "Event"),
	End		UMETA(DisplayName = "End")
};

UENUM(BlueprintType)
enum class ESFDialogueAdvanceMode : uint8
{
	Manual          UMETA(DisplayName = "Manual"),
	FixedDuration   UMETA(DisplayName = "Fixed Duration"),
	WordsPerMinute  UMETA(DisplayName = "Words Per Minute"),
	AudioFinished   UMETA(DisplayName = "Audio Finished")
};

USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFDialogueChoice
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FText ChoiceText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName NextNodeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FGameplayTagContainer RequiredTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FGameplayTagContainer BlockedTags;

	bool IsValid() const
	{
		return !ChoiceText.IsEmpty();
	}
};

USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFDialogueNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName NodeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	ESFDialogueNodeType NodeType = ESFDialogueNodeType::Line;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName SpeakerId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue", meta = (MultiLine = true))
	FText LineText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TArray<FSFDialogueChoice> Choices;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName NextNodeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FGameplayTag EventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FGameplayTagContainer RequiredTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FGameplayTagContainer BlockedTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Camera")
	FSFDialogueCameraShot CameraShot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Advance")
	ESFDialogueAdvanceMode AdvanceMode = ESFDialogueAdvanceMode::Manual;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Advance")
	float AdvanceDelaySeconds = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Advance")
	float WordsPerMinute = 140.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Advance")
	TObjectPtr<USoundBase> VoiceClip = nullptr;

	bool IsValid() const
	{
		return NodeId != NAME_None;
	}
};

USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFDialogueDisplayData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FName SpeakerId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FText LineText;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	TArray<FSFDialogueChoice> Choices;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	ESFDialogueNodeType NodeType = ESFDialogueNodeType::End;
};