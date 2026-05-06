#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SFInteractableInterface.h"
#include "SFInteractionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnInteractableChangedSignature,
	AActor*, NewInteractableActor,
	FSFInteractionOption, PrimaryOption,
	const TArray<FSFInteractionOption>&, AllOptions);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SIGNALFORGERPG_API USFInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFInteractionComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY()
	TObjectPtr<class ACharacter> OwnerCharacter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	float InteractionTraceDistance = 250.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	float InteractionTraceRadius = 24.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	TEnumAsByte<ECollisionChannel> InteractionTraceChannel = ECC_Visibility;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	bool bInteractionEnabled = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction|Debug")
	bool bDrawDebugInteractionTrace = false;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<AActor> CurrentInteractableActor;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	FSFInteractionOption CurrentPrimaryOption;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	TArray<FSFInteractionOption> CurrentInteractionOptions;

public:
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TryInteract();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TryInteractWithOption(FName OptionId);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void SetInteractionEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetCurrentInteractableActor() const
	{
		return CurrentInteractableActor;
	}

	UFUNCTION(BlueprintPure, Category = "Interaction")
	FSFInteractionOption GetCurrentPrimaryOption() const
	{
		return CurrentPrimaryOption;
	}

	UFUNCTION(BlueprintPure, Category = "Interaction")
	const TArray<FSFInteractionOption>& GetCurrentInteractionOptions() const
	{
		return CurrentInteractionOptions;
	}

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteractableChangedSignature OnInteractableChanged;

protected:
	void UpdateCurrentInteractable();
	bool PerformInteractionTrace(FHitResult& OutHitResult) const;
	void SetCurrentInteractable(AActor* NewInteractableActor, const FSFInteractionContext& NewContext);

	bool IsLocallyControlled() const;
	bool IsActorInteractable(const FSFInteractionContext& Context) const;

	FSFInteractionContext BuildInteractionContextFromHit(const FHitResult& HitResult) const;
	FSFInteractionContext BuildInteractionContextForActor(AActor* Actor) const;

	FSFInteractionOption ResolvePrimaryOption(const FSFInteractionContext& Context) const;
	TArray<FSFInteractionOption> ResolveInteractionOptions(const FSFInteractionContext& Context) const;

	bool TryStartConversation(AActor* TargetActor);
};