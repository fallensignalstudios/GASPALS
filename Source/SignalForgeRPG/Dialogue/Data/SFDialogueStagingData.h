#pragma once

#include "CoreMinimal.h"
#include "SFDialogueStagingData.generated.h"

UENUM(BlueprintType)
enum class ESFDialoguePositioningMode : uint8
{
	None            UMETA(DisplayName = "None"),
	TeleportPlayer  UMETA(DisplayName = "Teleport Player"),
	TeleportBoth    UMETA(DisplayName = "Teleport Both")
};

USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFDialogueStagingData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Staging")
	ESFDialoguePositioningMode PositioningMode = ESFDialoguePositioningMode::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Staging")
	FTransform PlayerTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Staging")
	FTransform SourceTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Staging")
	bool bSnapPlayerRotationToSource = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Staging")
	bool bDisablePlayerMovementDuringDialogue = true;
};