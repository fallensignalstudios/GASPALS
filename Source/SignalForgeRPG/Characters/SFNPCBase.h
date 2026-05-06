#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "Components/SFInteractableInterface.h"
#include "Dialogue/Data/SFConversationSourceInterface.h"
#include "SFNPCBase.generated.h"

class USFConversationDataAsset;
class USFNPCStateComponent;
class USFNPCGoalComponent;
class USFNPCNarrativeComponent;

USTRUCT(BlueprintType)
struct FSFNarrativeEventPayload
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative")
	FGameplayTag EventTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative")
	FGameplayTagContainer EventContextTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative")
	TObjectPtr<AActor> Instigator = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative")
	TObjectPtr<AActor> Subject = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative")
	float Intensity = 1.0f;
};

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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Identity")
	FText NPCName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Identity")
	FGameplayTagContainer IdentityTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Interaction")
	FText InteractionPromptText = FText::FromString(TEXT("Talk"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Interaction")
	FText InteractionSubPromptText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Interaction")
	float BaseInteractionRange = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Interaction")
	bool bShowPromptWhenUnavailable = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Interaction")
	ESFInteractionTriggerType InteractionTriggerType = ESFInteractionTriggerType::Press;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Interaction", meta = (ClampMin = "0.0"))
	float InteractionHoldDuration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Interaction")
	FName PrimaryInteractionOptionId = TEXT("Talk");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Interaction")
	FGameplayTagContainer RequiredInteractionTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Interaction")
	FGameplayTagContainer BlockedInteractionTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Interaction")
	FGameplayTagContainer AutoDisableInteractionTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Dialogue")
	FName DialogueSpeakerId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Dialogue")
	TObjectPtr<USFConversationDataAsset> DefaultConversationAsset = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Components")
	TObjectPtr<USFNPCStateComponent> StateComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Components")
	TObjectPtr<USFNPCGoalComponent> GoalComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Components")
	TObjectPtr<USFNPCNarrativeComponent> NarrativeComponent;

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintNativeEvent, Category = "NPC|Dialogue")
	USFConversationDataAsset* ResolveConversationAsset(
		const FSFInteractionContext& InteractionContext) const;
	virtual USFConversationDataAsset* ResolveConversationAsset_Implementation(
		const FSFInteractionContext& InteractionContext) const;

	UFUNCTION(BlueprintNativeEvent, Category = "NPC|Interaction")
	bool CanInteractByState(const FSFInteractionContext& InteractionContext) const;
	virtual bool CanInteractByState_Implementation(
		const FSFInteractionContext& InteractionContext) const;

	UFUNCTION(BlueprintNativeEvent, Category = "NPC|Interaction")
	TArray<FSFInteractionOption> BuildInteractionOptions(
		const FSFInteractionContext& InteractionContext) const;
	virtual TArray<FSFInteractionOption> BuildInteractionOptions_Implementation(
		const FSFInteractionContext& InteractionContext) const;

	UFUNCTION(BlueprintImplementableEvent, Category = "NPC|Interaction")
	void OnInteractionSucceeded(
		const FSFInteractionContext& InteractionContext,
		FName OptionId);

	UFUNCTION(BlueprintImplementableEvent, Category = "NPC|Narrative")
	void OnNarrativeEventApplied(const FSFNarrativeEventPayload& Payload);

public:
	UFUNCTION(BlueprintCallable, Category = "NPC")
	const USFNPCStateComponent* GetStateComponent() const { return StateComponent; }

	UFUNCTION(BlueprintCallable, Category = "NPC")
	const USFNPCGoalComponent* GetGoalComponent() const { return GoalComponent; }

	UFUNCTION(BlueprintCallable, Category = "NPC")
	const USFNPCNarrativeComponent* GetNarrativeComponent() const { return NarrativeComponent; }

	UFUNCTION(BlueprintCallable, Category = "NPC")
	bool HasStateTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable, Category = "NPC")
	bool HasAnyStateTags(const FGameplayTagContainer& Tags) const;

	UFUNCTION(BlueprintCallable, Category = "NPC")
	bool HasAllStateTags(const FGameplayTagContainer& Tags) const;

	UFUNCTION(BlueprintCallable, Category = "NPC")
	void ApplyNarrativeEvent(const FSFNarrativeEventPayload& Payload);

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