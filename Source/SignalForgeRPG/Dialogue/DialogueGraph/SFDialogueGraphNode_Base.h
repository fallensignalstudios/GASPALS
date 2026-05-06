#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "GameplayTagContainer.h"
#include "Dialogue/SFDialogueCameraTypes.h"
#include "Dialogue/Data/SFDialogueTypes.h"
#include "Sound/SoundBase.h"
#include "SFDialogueGraphNode_Base.generated.h"

UCLASS(Abstract)
class SIGNALFORGERPG_API USFDialogueGraphNode_Base : public UEdGraphNode
{
	GENERATED_BODY()

public:
	USFDialogueGraphNode_Base();

	UPROPERTY()
	FGuid NodeGuidValue;

	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FName SpeakerId = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FGameplayTagContainer RequiredTags;

	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FGameplayTagContainer BlockedTags;

	UPROPERTY(EditAnywhere, Category = "Dialogue|Camera")
	FSFDialogueCameraShot CameraShot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Advance")
	ESFDialogueAdvanceMode AdvanceMode = ESFDialogueAdvanceMode::Manual;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Advance", meta = (EditCondition = "AdvanceMode == ESFDialogueAdvanceMode::FixedDuration", ClampMin = "0.0"))
	float AdvanceDelaySeconds = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Advance", meta = (EditCondition = "AdvanceMode == ESFDialogueAdvanceMode::WordsPerMinute", ClampMin = "1.0"))
	float WordsPerMinute = 140.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Advance", meta = (EditCondition = "AdvanceMode == ESFDialogueAdvanceMode::AudioFinished"))
	TObjectPtr<USoundBase> VoiceClip = nullptr;

	UFUNCTION()
	FName GetRuntimeNodeId() const;

	virtual ESFDialogueNodeType GetRuntimeNodeType() const PURE_VIRTUAL(USFDialogueGraphNode_Base::GetRuntimeNodeType, return ESFDialogueNodeType::Line;);

	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const;
	virtual void PostPlacedNewNode() override;
	virtual void PostLoad() override;

	UEdGraphPin* GetInputPin() const;
	UEdGraphPin* GetOutputPin() const;


protected:

	virtual bool CanUserDeleteNode() const override { return true; }
	virtual bool CanDuplicateNode() const override { return true; }
};