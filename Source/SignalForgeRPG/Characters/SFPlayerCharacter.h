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
class USFNarrativeComponent;
class USFQuestDefinition;
class USFQuestInstance;
class ASFPlayerState;

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

	// ----------------------------------------------------------------------
	// Narrative / Quest access
	// ----------------------------------------------------------------------

	/** Convenience proxy: pulls the narrative component off ASFPlayerState. */
	UFUNCTION(BlueprintPure, Category = "Narrative")
	USFNarrativeComponent* GetNarrativeComponent() const;

	/** Convenience cast for the SF-typed PlayerState. Null if not yet replicated. */
	UFUNCTION(BlueprintPure, Category = "Narrative")
	ASFPlayerState* GetSFPlayerState() const;

	/**
	 * Quests that auto-start on the server when this character first spawns.
	 * Soft references so unused quests don't get pulled into memory eagerly.
	 * Use this for the player's opening main-quest entry, tutorial quests, etc.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Quests")
	TArray<TSoftObjectPtr<USFQuestDefinition>> DefaultStartingQuests;

	/** Server-only: start a single quest by definition. Wraps the narrative component API. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Narrative|Quests")
	USFQuestInstance* StartQuestByDefinition(USFQuestDefinition* QuestDefinition, FName StartStateId = NAME_None);

	/** Server-only: start a quest by primary asset id (e.g. "Quest:Q_TutorialIntro"). */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Narrative|Quests")
	USFQuestInstance* StartQuestByAssetId(FPrimaryAssetId QuestAssetId, FName StartStateId = NAME_None);

	/** Server-only: restart a previously-finished quest from the start. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Narrative|Quests")
	bool RestartQuestByAssetId(FPrimaryAssetId QuestAssetId, FName StartStateId = NAME_None);

	/** Server-only: abandon an in-progress quest. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Narrative|Quests")
	bool AbandonQuestByAssetId(FPrimaryAssetId QuestAssetId);

	/** Cheat / debug: server-only exec to start a quest by primary-asset-id string. Usage in PIE: SF_StartQuest Quest:Q_Test */
	UFUNCTION(Exec, Category = "Narrative|Quests|Debug")
	void SF_StartQuest(const FString& AssetIdString);

	/** Cheat / debug: server-only exec to abandon a quest by primary-asset-id string. */
	UFUNCTION(Exec, Category = "Narrative|Quests|Debug")
	void SF_AbandonQuest(const FString& AssetIdString);

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

	/** Weapon input actions — routed via InputTag to whichever ability the equipped weapon granted. */

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Weapon")
	TObjectPtr<UInputAction> PrimaryFireAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Weapon")
	TObjectPtr<UInputAction> ADSAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Weapon")
	TObjectPtr<UInputAction> ReloadAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Weapon")
	TObjectPtr<UInputAction> GrenadeAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Weapon")
	TObjectPtr<UInputAction> SecondaryFireAction = nullptr;

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

	/** Weapon input handlers — dispatch through the InputTag system so the currently-equipped
	 *  weapon's granted ability (e.g. WeaponFire for ranged, AttackLight for melee) responds. */
	void OnPrimaryFirePressed(const FInputActionValue& Value);
	void OnPrimaryFireReleased(const FInputActionValue& Value);
	void OnADSPressed(const FInputActionValue& Value);
	void OnADSReleased(const FInputActionValue& Value);
	void OnReloadPressed(const FInputActionValue& Value);
	void OnReloadReleased(const FInputActionValue& Value);
	void OnGrenadePressed(const FInputActionValue& Value);
	void OnGrenadeReleased(const FInputActionValue& Value);
	void OnSecondaryFirePressed(const FInputActionValue& Value);
	void OnSecondaryFireReleased(const FInputActionValue& Value);

	virtual void PawnClientRestart() override;

private:
	void ProcessAbilityInputPressed(const FGameplayTag& InputTag);
	bool bCrouchToggled = false;
};