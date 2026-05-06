#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Core/SFAttributeSetBase.h"
#include "UI/SFAttributeBarWidget.h"
#include "Components/SFInteractableInterface.h"
#include "SFPlayerHUDWidgetController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWidgetControllerAttributeChanged, float, NewValue, float, MaxValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTrackedEnemyChangedSignature, bool, bHasTrackedEnemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHUDVisibilityChangedSignature, bool, bVisible);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPortraitReadySignature, UTextureRenderTarget2D*, RenderTarget);

// Legacy-style prompt delegate (still useful for simple widgets)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnInteractionPromptChangedSignature,
	bool, bShowPrompt,
	FText, PromptText);

// New delegate for full interaction options if you want HUD widgets to bind directly.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnInteractionOptionsChangedSignature,
	bool, bHasInteractable,
	FSFInteractionOption, PrimaryOption,
	const TArray<FSFInteractionOption>&, AllOptions);

class UTextureRenderTarget2D;

UCLASS(BlueprintType, Blueprintable)
class SIGNALFORGERPG_API USFPlayerHUDWidgetController : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UI")
	void Initialize(class ASFCharacterBase* InPlayerCharacter);

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<class ASFCharacterBase> PlayerCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	float CurrentHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	float MaxHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	float CurrentEcho = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	float MaxEcho = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	float CurrentShields = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	float MaxShields = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	float CurrentStamina = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	float MaxStamina = 0.0f;

	UPROPERTY(BlueprintAssignable, Category = "UI|Portrait")
	FOnPortraitReadySignature OnPortraitReady;

	UFUNCTION()
	void HandleHealthChanged(float NewValue, float InMaxValue);

	UFUNCTION()
	void HandleEchoChanged(float NewValue, float InMaxValue);

	UFUNCTION(BlueprintPure, Category = "UI")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	float GetEchoPercent() const;

	UFUNCTION()
	void HandleShieldsChanged(float NewValue, float InMaxValue);

	UFUNCTION(BlueprintPure, Category = "UI")
	float GetShieldsPercent() const;

	UFUNCTION()
	void HandleStaminaChanged(float NewValue, float InMaxValue);

	UFUNCTION(BlueprintPure, Category = "UI")
	float GetStaminaPercent() const;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnWidgetControllerAttributeChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnWidgetControllerAttributeChanged OnEchoChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnWidgetControllerAttributeChanged OnShieldsChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnWidgetControllerAttributeChanged OnStaminaChanged;

	UFUNCTION(BlueprintPure, Category = "UI")
	float GetTrackedAttributeValue(ESFTrackedAttribute Attribute) const;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<class ASFCharacterBase> TrackedEnemyCharacter;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetTrackedEnemy(class ASFCharacterBase* InEnemyCharacter);

	UFUNCTION()
	void HandleTrackedEnemyHealthChanged(float NewValue, float InMaxValue);

	UFUNCTION()
	void HandleTrackedEnemyShieldsChanged(float NewValue, float InMaxValue);

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnWidgetControllerAttributeChanged OnTrackedEnemyHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnWidgetControllerAttributeChanged OnTrackedEnemyShieldsChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnTrackedEnemyChangedSignature OnTrackedEnemyChanged;

	// Updated to match new interaction component delegate
	UFUNCTION()
	void HandleInteractableChanged(
		AActor* NewInteractableActor,
		FSFInteractionOption PrimaryOption,
		const TArray<FSFInteractionOption>& AllOptions);

	UFUNCTION(BlueprintPure, Category = "UI")
	bool HasInteractionPrompt() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	FText GetInteractionPromptText() const;

	// Basic prompt state for simple “Press E to …” widgets
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	bool bHasInteractionPrompt = false;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	FText CurrentInteractionPromptText;

	// Richer interaction state
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	FSFInteractionOption CurrentInteractionPrimaryOption;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TArray<FSFInteractionOption> CurrentInteractionOptions;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnInteractionPromptChangedSignature OnInteractionPromptChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnInteractionOptionsChangedSignature OnInteractionOptionsChanged;

	UFUNCTION(BlueprintPure, Category = "UI")
	bool HasTrackedEnemy() const;

	// Broadcasts true when HUD should be visible, false when it should hide
	UPROPERTY(BlueprintAssignable, Category = "UI|Visibility")
	FOnHUDVisibilityChangedSignature OnHUDVisibilityChanged;

	// How long after the last activity before the HUD hides (seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Visibility")
	float HUDInactivityTimeout = 5.0f;

	UFUNCTION(BlueprintCallable, Category = "UI|Visibility")
	void NotifyHUDActivity();

	UFUNCTION(BlueprintPure, Category = "UI|Visibility")
	bool IsHUDVisible() const { return bHUDVisible; }

	UFUNCTION(BlueprintPure, Category = "UI|Portrait")
	UTextureRenderTarget2D* GetPortraitRenderTarget() const;

	UFUNCTION(BlueprintPure, Category = "UI")
	class USFEquipmentComponent* GetEquipmentComponent() const;


private:

	FTimerHandle HUDInactivityTimerHandle;
	bool bHUDVisible = true;

	void OnInactivityTimerExpired();
	void ResetInactivityTimer();

	};