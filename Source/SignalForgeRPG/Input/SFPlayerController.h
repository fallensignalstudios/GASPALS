#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SFPlayerController.generated.h"

class ASFPlayerMenuPreviewScene;
class ASFMenuPreviewCharacter;
class USFUserWidgetBase;
class USFPlayerHUDWidgetController;
class USFAbilityBarWidgetController;
class USFInventoryWidgetController;
class USFEquipmentWidgetController;
class ASFCharacterBase;

UCLASS()
class SIGNALFORGERPG_API ASFPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASFPlayerController();

	virtual void BeginPlay() override;

	/** Tracking / HUD helpers */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetTrackedEnemy(ASFCharacterBase* InEnemyCharacter);

	UFUNCTION(BlueprintPure, Category = "UI")
	USFUserWidgetBase* GetPlayerHUDWidget() const { return PlayerHUDWidget; }

	UFUNCTION(BlueprintPure, Category = "UI")
	USFPlayerHUDWidgetController* GetPlayerHUDWidgetController() const { return PlayerHUDWidgetController; }

	UFUNCTION(BlueprintPure, Category = "UI")
	USFAbilityBarWidgetController* GetAbilityBarWidgetController() const { return AbilityBarWidgetController; }

	UFUNCTION(BlueprintPure, Category = "UI")
	USFInventoryWidgetController* GetInventoryWidgetController() const { return InventoryWidgetController; }

	UFUNCTION(BlueprintPure, Category = "UI")
	USFEquipmentWidgetController* GetEquipmentWidgetController() const { return EquipmentWidgetController; }

	UFUNCTION(BlueprintCallable, Category = "UI")
	void RefreshAbilityBar();

	/** Main menu toggle */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void TogglePlayerMenu();

	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsPlayerMenuOpen() const { return bIsPlayerMenuOpen; }

protected:
	/** Creates and wires all HUD-related widget controllers for the local player. */
	void InitializeUIControllers(ASFCharacterBase* PlayerCharacter);

	/** Spawns/creates the main HUD widget and binds its controllers. */
	void InitializeHUDWidget();

	/** Optionally locates a preview scene actor in the world for the player menu. */
	void InitializePlayerMenuPreviewScene();

	/** Opens the player menu immediately (no animation dependency). */
	void OpenPlayerMenu();

	/** Closes the player menu immediately (no animation dependency). */
	void ClosePlayerMenu();

	/** Updates the preview character for the menu, if any. */
	void RefreshPlayerMenuPreview();

protected:
	/** HUD widget type and instance */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<USFUserWidgetBase> PlayerHUDWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<USFUserWidgetBase> PlayerHUDWidget = nullptr;

	/** Core HUD / ability / inventory / equipment controllers */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<USFPlayerHUDWidgetController> PlayerHUDWidgetController = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<USFAbilityBarWidgetController> AbilityBarWidgetController = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<USFInventoryWidgetController> InventoryWidgetController = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<USFEquipmentWidgetController> EquipmentWidgetController = nullptr;

	/** Optional preview scene for menu camera & character */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "UI|Preview")
	TObjectPtr<ASFPlayerMenuPreviewScene> PlayerMenuPreviewScene = nullptr;

	UPROPERTY()
	TObjectPtr<ASFMenuPreviewCharacter> CachedPreviewCharacter = nullptr;

	/** Player menu widget type and instance */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UUserWidget> PlayerMenuWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<class UUserWidget> PlayerMenuWidget = nullptr;

	/** Simple open/closed flag */
	UPROPERTY(BlueprintReadOnly, Category = "UI|Preview")
	bool bIsPlayerMenuOpen = false;

	/** Optional polling timer; can be disabled if ASC/UI events are used instead. */
	UPROPERTY()
	FTimerHandle AbilityBarPollingTimerHandle;
};