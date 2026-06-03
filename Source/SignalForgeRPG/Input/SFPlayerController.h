#pragma once

#include "CoreMinimal.h"
#include "Combat/SFWeaponInstanceTypes.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/SFInventoryTypes.h"
#include "Inventory/SFSlotTypes.h"
#include "SFPlayerController.generated.h"

class ASFPlayerMenuPreviewScene;
class ASFMenuPreviewCharacter;
class USFUserWidgetBase;
class USFPlayerHUDWidgetController;
class USFAbilityBarWidgetController;
class USFInventoryWidgetController;
class USFEquipmentWidgetController;
class ASFCharacterBase;
class USFDamageNumberWidget;
class USFDeathScreenWidget;

/**
 * Snapshot of the player's equipment loadout taken at the moment of death so
 * it can be re-applied to the freshly spawned pawn after respawn. The pawn
 * itself is destroyed during RestartPlayerAtTransform, which drops the
 * EquipmentComponent and its anim-overlay-driving state with it; the
 * controller survives, so the snapshot lives here.
 */
USTRUCT(BlueprintType)
struct FSFRespawnLoadoutEntry
{
	GENERATED_BODY()

	UPROPERTY()
	ESFEquipmentSlot Slot = ESFEquipmentSlot::None;

	UPROPERTY()
	FSFWeaponInstanceData WeaponInstance;
};

USTRUCT(BlueprintType)
struct FSFRespawnLoadout
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FSFRespawnLoadoutEntry> Entries;

	UPROPERTY()
	ESFEquipmentSlot ActiveSlot = ESFEquipmentSlot::None;

	/** Full inventory contents at the moment of death (items, stacks, instance metadata). */
	UPROPERTY()
	TArray<FSFInventoryEntry> InventoryEntries;

	UPROPERTY()
	bool bHasSnapshot = false;

	void Reset()
	{
		Entries.Reset();
		InventoryEntries.Reset();
		ActiveSlot = ESFEquipmentSlot::None;
		bHasSnapshot = false;
	}
};

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

	/**
	 * Death-screen entry. Called by the death widget after the designer's
	 * exit animation completes. Forwards to the game mode to do the actual
	 * pawn respawn at either a checkpoint or near the death location.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI|Death")
	void RespawnFromDeathScreen(bool bRestartFromCheckpoint);

	/** Designer-assigned WBP_DeathScreen class. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Death")
	TSubclassOf<USFDeathScreenWidget> DeathScreenWidgetClass;

	//~ Begin APlayerController interface
	virtual void OnPossess(APawn* InPawn) override;
	//~ End APlayerController interface

protected:
	/** Bound to the possessed pawn's OnCharacterDied; spawns the death widget. */
	UFUNCTION()
	void HandlePawnDied(ASFCharacterBase* DeadCharacter, ASFCharacterBase* Killer);

	/**
	 * Snapshot the dying pawn's equipped slots + active slot into PendingRespawnLoadout
	 * so OnPossess can re-apply it to the fresh pawn (and the anim overlays come back).
	 */
	void SnapshotLoadoutForRespawn(ASFCharacterBase* DyingCharacter);

	/**
	 * Re-apply PendingRespawnLoadout onto the freshly possessed pawn and rebind
	 * death/UI hooks. Safe to call when no snapshot is pending.
	 */
	void RestoreLoadoutAfterRespawn(ASFCharacterBase* FreshCharacter);

	/**
	 * Last snapshot of the player's loadout, captured in HandlePawnDied and
	 * consumed in OnPossess. Lives on the controller because the pawn (and its
	 * equipment component) is destroyed during respawn.
	 */
	UPROPERTY()
	FSFRespawnLoadout PendingRespawnLoadout;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Death")
	TObjectPtr<USFDeathScreenWidget> DeathScreenWidget = nullptr;

	/** Cached so we can hand it back to the game mode for non-dark-zone respawn. */
	FVector LastDeathLocation = FVector::ZeroVector;

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

public:
	/**
	 * Widget class used by USFDamageNumberSubsystem to spawn Destiny-style
	 * damage floaters. Configure on the PC Blueprint; subsystem reads it via
	 * GetOwningPlayerController().
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|DamageNumbers")
	TSubclassOf<USFDamageNumberWidget> DamageNumberWidgetClass;

protected:

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