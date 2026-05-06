#pragma once

#include "CoreMinimal.h"
#include "Characters/SFCharacterBase.h"
#include "InputActionValue.h"
#include "GameplayTagContainer.h"
#include "Core/SignalForgeGameplayTags.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Classes/SFPlayerClassComponent.h"
#include "SFPlayerCharacter.generated.h"

class USFInteractionComponent;
class USFDialogueComponent;
class USFDialogueCameraComponent;
class USceneComponent;
class UCameraComponent;
class USpringArmComponent;
class UInputMappingContext;
class UInputAction;
class USFWeaponData;

UCLASS()
class SIGNALFORGERPG_API ASFPlayerCharacter : public ASFCharacterBase
{
	GENERATED_BODY()

public:
	ASFPlayerCharacter();

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	USFDialogueComponent* GetDialogueComponent() const { return DialogueComponent; }

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	USFDialogueCameraComponent* GetDialogueCameraComponent() const { return DialogueCameraComponent; }

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	USceneComponent* GetDialogueCameraRoot() const { return DialogueCameraRoot; }

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	UCameraComponent* GetDialogueCamera() const { return DialogueCamera; }

	UFUNCTION(BlueprintPure, Category = "Camera")
	UCameraComponent* GetGameplayCamera() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portrait")
	TObjectPtr<USceneCaptureComponent2D> PortraitCaptureComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Portrait")
	TObjectPtr<UTextureRenderTarget2D> PortraitRenderTarget;

	UFUNCTION(BlueprintPure, Category = "Portrait")
	UTextureRenderTarget2D* GetPortraitRenderTarget() const { return PortraitRenderTarget; }

	UFUNCTION(BlueprintCallable, Category = "Portrait")
	void CapturePortrait();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USFPlayerClassComponent> PlayerClassComponent = nullptr;

	UFUNCTION(BlueprintPure, Category = "Class")
	USFPlayerClassComponent* GetPlayerClassComponent() const { return PlayerClassComponent; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Class")
	TObjectPtr<USFClassDefinition> DefaultClassDefinition = nullptr;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void Tick(float DeltaTime) override;

	/** Enhanced Input */

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultInputMappingContext = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CrouchAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> TogglePlayerMenuAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> Ability1Action = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> Ability2Action = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> Ability3Action = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> Ability4Action = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> Ability5Action = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> Ability6Action = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> Ability7Action = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> Ability8Action = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> Ability9Action = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> Ability10Action = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CameraZoomAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> BlockAction = nullptr;

	/** Components */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USFDialogueComponent> DialogueComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USFDialogueCameraComponent> DialogueCameraComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> DialogueCameraRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> DialogueCamera = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> GameplayCameraBoom = nullptr;

	/*UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> PlayerGameplayCamera = nullptr;*/

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float ZoomSpeed = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float MinZoomDistance = 150.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float MaxZoomDistance = 700.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
	float ZoomInterpSpeed = 8.0f;

private:
	float TargetZoomDistance = 350.0f; // matches your default TargetArmLength

	/** Equipment */

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool EquipInventoryItemAtIndex(int32 Index);


	/** Input handlers */

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	void StartJump(const FInputActionValue& Value);
	void StopJump(const FInputActionValue& Value);

	void StartSprintInput(const FInputActionValue& Value);
	void StopSprintInput(const FInputActionValue& Value);

	void StartCrouchInput(const FInputActionValue& Value);
	void StopCrouchInput(const FInputActionValue& Value);

	void HandleInteractInput(const FInputActionValue& Value);
	void HandleTogglePlayerMenuInput(const FInputActionValue& Value);

	void OnAbility1Pressed(const FInputActionValue& Value);
	void OnAbility2Pressed(const FInputActionValue& Value);
	void OnAbility3Pressed(const FInputActionValue& Value);
	void OnAbility4Pressed(const FInputActionValue& Value);
	void OnAbility5Pressed(const FInputActionValue& Value);
	void OnAbility6Pressed(const FInputActionValue& Value);
	void OnAbility7Pressed(const FInputActionValue& Value);
	void OnAbility8Pressed(const FInputActionValue& Value);
	void OnAbility9Pressed(const FInputActionValue& Value);
	void OnAbility10Pressed(const FInputActionValue& Value);
	void ProcessAbilityInputReleased(const FGameplayTag& InputTag);
	void OnAbility1Released(const FInputActionValue& Value);
	void OnAbility2Released(const FInputActionValue& Value);
	void OnAbility3Released(const FInputActionValue& Value);
	void OnAbility4Released(const FInputActionValue& Value);
	void OnAbility5Released(const FInputActionValue& Value);
	void OnAbility6Released(const FInputActionValue& Value);
	void OnAbility7Released(const FInputActionValue& Value);
	void OnAbility8Released(const FInputActionValue& Value);
	void OnAbility9Released(const FInputActionValue& Value);
	void OnAbility10Released(const FInputActionValue& Value);
	void HandleCameraZoom(const FInputActionValue& Value);
	void OnBlockPressed(const FInputActionValue& Value);
	void OnBlockReleased(const FInputActionValue& Value);
	virtual void PawnClientRestart() override;

private:
	void ProcessAbilityInputPressed(const FGameplayTag& InputTag);
	bool bCrouchToggled = false;
};