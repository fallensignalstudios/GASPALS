#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/SFInteractableInterface.h"
#include "Dialogue/Data/SFConversationSourceInterface.h"
#include "SFNPCBase.generated.h"

class USFConversationDataAsset;

UCLASS()
class SIGNALFORGERPG_API ASFNPCBase
	: public ACharacter
	, public ISFInteractableInterface
	, public ISFConversationSourceInterface
{
	GENERATED_BODY()

public:
	ASFNPCBase();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	FText NPCName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	FText InteractionPromptText = FText::FromString(TEXT("Talk"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	FText InteractionSubPromptText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	bool bCanInteract = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	float InteractionRange = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	bool bShowPromptWhenUnavailable = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	ESFInteractionTriggerType InteractionTriggerType = ESFInteractionTriggerType::Press;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC", meta = (ClampMin = "0.0"))
	float InteractionHoldDuration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	FName PrimaryInteractionOptionId = TEXT("Talk");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName DialogueSpeakerId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<USFConversationDataAsset> ConversationAsset = nullptr;

public:
	virtual ESFInteractionAvailability GetInteractionAvailability_Implementation(
		const FSFInteractionContext& InteractionContext) const override;

	virtual FSFInteractionOption GetPrimaryInteractionOption_Implementation(
		const FSFInteractionContext& InteractionContext) const override;

	virtual TArray<FSFInteractionOption> GetInteractionOptions_Implementation(
		const FSFInteractionContext& InteractionContext) const override;

	virtual FSFInteractionExecutionResult Interact_Implementation(
		const FSFInteractionContext& InteractionContext) override;

	virtual FSFInteractionExecutionResult InteractWithOption_Implementation(
		const FSFInteractionContext& InteractionContext,
		FName OptionId) override;

	virtual void BeginInteractionFocus_Implementation(
		const FSFInteractionContext& InteractionContext) override;

	virtual void EndInteractionFocus_Implementation(
		const FSFInteractionContext& InteractionContext) override;

	virtual FVector GetInteractionLocation_Implementation(
		const FSFInteractionContext& InteractionContext) const override;

	virtual float GetInteractionRange_Implementation(
		const FSFInteractionContext& InteractionContext) const override;

	virtual bool CanInteract_Implementation(
		const FSFInteractionContext& InteractionContext) const override;

	virtual FText GetInteractionPromptText_Implementation(
		const FSFInteractionContext& InteractionContext) const override;

	virtual USFConversationDataAsset* GetConversationAsset_Implementation() const override;
	virtual FName GetDialogueSpeakerId_Implementation() const override;
};