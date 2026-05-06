#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InputActionValue.h"
#include "SFInteractableInterface.generated.h"

class UActorComponent;

UENUM(BlueprintType)
enum class ESFInteractionAvailability : uint8
{
	Available,
	OutOfRange,
	Blocked,
	Disabled,
	Invalid
};

UENUM(BlueprintType)
enum class ESFInteractionTriggerType : uint8
{
	Press,
	Hold,
	Release
};

USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFInteractionContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Interaction")
	TObjectPtr<AActor> InteractingActor = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Interaction")
	TObjectPtr<UActorComponent> InteractingComponent = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Interaction")
	FVector InteractionOrigin = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Interaction")
	FVector InteractionDirection = FVector::ForwardVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Interaction")
	float DistanceToTarget = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Interaction")
	FHitResult HitResult;

	bool IsValid() const
	{
		return InteractingActor != nullptr;
	}
};

USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFInteractionOption
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Interaction")
	FName OptionId = NAME_None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Interaction")
	FText PromptText;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Interaction")
	FText SubPromptText;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Interaction")
	ESFInteractionAvailability Availability = ESFInteractionAvailability::Invalid;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Interaction")
	ESFInteractionTriggerType TriggerType = ESFInteractionTriggerType::Press;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Interaction", meta = (ClampMin = "0.0"))
	float HoldDuration = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Interaction")
	bool bShowPromptWhenUnavailable = false;

	bool IsAvailable() const
	{
		return Availability == ESFInteractionAvailability::Available;
	}
};

USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFInteractionExecutionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Interaction")
	bool bSucceeded = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Interaction")
	ESFInteractionAvailability Availability = ESFInteractionAvailability::Invalid;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Interaction")
	FText FailureReason;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Interaction")
	FName ExecutedOptionId = NAME_None;

	static FSFInteractionExecutionResult Success(FName InOptionId = NAME_None)
	{
		FSFInteractionExecutionResult Result;
		Result.bSucceeded = true;
		Result.Availability = ESFInteractionAvailability::Available;
		Result.ExecutedOptionId = InOptionId;
		return Result;
	}

	static FSFInteractionExecutionResult Failure(
		ESFInteractionAvailability InAvailability,
		const FText& InFailureReason = FText::GetEmpty(),
		FName InOptionId = NAME_None)
	{
		FSFInteractionExecutionResult Result;
		Result.bSucceeded = false;
		Result.Availability = InAvailability;
		Result.FailureReason = InFailureReason;
		Result.ExecutedOptionId = InOptionId;
		return Result;
	}
};

UINTERFACE(BlueprintType)
class SIGNALFORGERPG_API USFInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class SIGNALFORGERPG_API ISFInteractableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	ESFInteractionAvailability GetInteractionAvailability(const FSFInteractionContext& InteractionContext) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FSFInteractionOption GetPrimaryInteractionOption(const FSFInteractionContext& InteractionContext) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	TArray<FSFInteractionOption> GetInteractionOptions(const FSFInteractionContext& InteractionContext) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FSFInteractionExecutionResult Interact(const FSFInteractionContext& InteractionContext);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FSFInteractionExecutionResult InteractWithOption(const FSFInteractionContext& InteractionContext, FName OptionId);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void BeginInteractionFocus(const FSFInteractionContext& InteractionContext);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void EndInteractionFocus(const FSFInteractionContext& InteractionContext);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FVector GetInteractionLocation(const FSFInteractionContext& InteractionContext) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	float GetInteractionRange(const FSFInteractionContext& InteractionContext) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(const FSFInteractionContext& InteractionContext) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FText GetInteractionPromptText(const FSFInteractionContext& InteractionContext) const;
};