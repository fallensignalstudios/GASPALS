#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Camera/CameraActor.h"
#include "LevelSequence.h"
#include "SFDialogueCameraTypes.generated.h"

class AActor;
class ACameraActor;
class ULevelSequence;

UENUM(BlueprintType)
enum class ESFDialogueShotTargetType : uint8
{
	None        UMETA(DisplayName = "None"),
	SourceActor UMETA(DisplayName = "Source Actor"),
	PlayerActor UMETA(DisplayName = "Player Actor"),
	CustomActor UMETA(DisplayName = "Custom Actor")
};

UENUM(BlueprintType)
enum class ESFDialogueCameraSourceMode : uint8
{
	Procedural    UMETA(DisplayName = "Procedural"),
	CameraActor   UMETA(DisplayName = "Camera Actor"),
	LevelSequence UMETA(DisplayName = "Level Sequence")
};

USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFDialogueCameraShot
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Camera")
	ESFDialogueCameraSourceMode SourceMode = ESFDialogueCameraSourceMode::Procedural;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Camera")
	ESFDialogueShotTargetType TargetType = ESFDialogueShotTargetType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Camera", meta = (EditCondition = "TargetType == ESFDialogueShotTargetType::CustomActor", EditConditionHides))
	TObjectPtr<AActor> CustomTargetActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Camera", meta = (EditCondition = "SourceMode == ESFDialogueCameraSourceMode::CameraActor", EditConditionHides))
	TObjectPtr<ACameraActor> CameraActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Camera", meta = (EditCondition = "SourceMode == ESFDialogueCameraSourceMode::LevelSequence", EditConditionHides))
	TSoftObjectPtr<ULevelSequence> LevelSequence = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Camera")
	FName SocketName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Camera")
	FVector RelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Camera")
	FRotator RelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Camera", meta = (ClampMin = "5.0", ClampMax = "170.0"))
	float FOV = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Camera", meta = (ClampMin = "0.0"))
	float BlendTime = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Camera|Sequence", meta = (EditCondition = "SourceMode == ESFDialogueCameraSourceMode::LevelSequence", EditConditionHides))
	FName PlayerBindingTag = TEXT("Dialogue.Player");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Camera|Sequence", meta = (EditCondition = "SourceMode == ESFDialogueCameraSourceMode::LevelSequence", EditConditionHides))
	FName SourceBindingTag = TEXT("Dialogue.Source");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Camera|Sequence", meta = (EditCondition = "SourceMode == ESFDialogueCameraSourceMode::LevelSequence", EditConditionHides))
	FName CustomBindingTag = TEXT("Dialogue.Custom");

	bool IsValidShot() const
	{
		switch (SourceMode)
		{
		case ESFDialogueCameraSourceMode::CameraActor:
			return CameraActor != nullptr;

		case ESFDialogueCameraSourceMode::LevelSequence:
			return !LevelSequence.IsNull();

		case ESFDialogueCameraSourceMode::Procedural:
		default:
			return TargetType != ESFDialogueShotTargetType::None;
		}
	}
};