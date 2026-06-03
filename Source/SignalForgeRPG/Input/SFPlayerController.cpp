#include "Input/SFPlayerController.h"

#include "Characters/SFCharacterBase.h"
#include "Components/SFEquipmentComponent.h"
#include "Components/SFInventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UI/SFAbilityBarWidgetController.h"
#include "UI/SFEquipmentWidgetController.h"
#include "UI/SFMenuPreviewCharacter.h"
#include "UI/SFPlayerHUDWidgetController.h"
#include "UI/SFPlayerMenuPreviewScene.h"
#include "UI/SFPlayerMenuWidget.h"
#include "UI/SFUserWidgetBase.h"
#include "UI/SFDeathScreenWidget.h"
#include "Inventory/SFInventoryWidgetController.h"
#include "Core/SFGameModeBase.h"
#include "World/SFDarkZoneVolume.h"
#include "Blueprint/UserWidget.h"

namespace
{
	static const FName LogCategoryName(TEXT("SFPlayerController"));
}

ASFPlayerController::ASFPlayerController()
{
	bReplicates = true;
}

void ASFPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	InitializePlayerMenuPreviewScene();

	ASFCharacterBase* PlayerCharacter = Cast<ASFCharacterBase>(GetPawn());
	if (!PlayerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] BeginPlay: No ASFCharacterBase pawn on local controller."), *GetName());
		return;
	}

	InitializeUIControllers(PlayerCharacter);
	InitializeHUDWidget();

	// Bind death; use AddUnique so OnPossess (which also binds, to cover the
	// respawn pawn) doesn't double-fire on the initial possession.
	PlayerCharacter->OnCharacterDied.AddUniqueDynamic(this, &ASFPlayerController::HandlePawnDied);

	// Optional polling if you want it back later.
	// if (AbilityBarWidgetController && !AbilityBarPollingTimerHandle.IsValid())
	// {
	// 	GetWorldTimerManager().SetTimer(
	// 		AbilityBarPollingTimerHandle,
	// 		this,
	// 		&ASFPlayerController::RefreshAbilityBar,
	// 		0.15f,
	// 		true);
	// }
}

void ASFPlayerController::InitializePlayerMenuPreviewScene()
{
	if (PlayerMenuPreviewScene)
	{
		return;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(this, ASFPlayerMenuPreviewScene::StaticClass(), FoundActors);

	if (FoundActors.Num() > 0)
	{
		PlayerMenuPreviewScene = Cast<ASFPlayerMenuPreviewScene>(FoundActors[0]);
	}
}

void ASFPlayerController::InitializeUIControllers(ASFCharacterBase* PlayerCharacter)
{
	check(PlayerCharacter);

	// Reuse existing controllers when possible so the PlayerHUDWidget (which
	// was handed a controller pointer in InitializeHUDWidget) keeps pointing
	// at the SAME object across respawn. Each controller's Initialize() is
	// idempotent and unbinds from the previous owner internally before binding
	// to the new one -- see USFPlayerHUDWidgetController::Initialize.
	if (!PlayerHUDWidgetController)
	{
		PlayerHUDWidgetController = NewObject<USFPlayerHUDWidgetController>(this);
	}
	if (PlayerHUDWidgetController)
	{
		PlayerHUDWidgetController->Initialize(PlayerCharacter);
	}

	if (!AbilityBarWidgetController)
	{
		AbilityBarWidgetController = NewObject<USFAbilityBarWidgetController>(this);
	}
	if (AbilityBarWidgetController)
	{
		AbilityBarWidgetController->Initialize(PlayerCharacter);
	}

	USFInventoryComponent* InventoryComponent = PlayerCharacter->FindComponentByClass<USFInventoryComponent>();
	USFEquipmentComponent* EquipmentComponent = PlayerCharacter->FindComponentByClass<USFEquipmentComponent>();

	if (!InventoryComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Could not find InventoryComponent on %s"),
			*GetNameSafe(this), *GetNameSafe(PlayerCharacter));
	}

	if (!EquipmentComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Could not find EquipmentComponent on %s"),
			*GetNameSafe(this), *GetNameSafe(PlayerCharacter));
	}

	if (InventoryComponent && EquipmentComponent)
	{
		if (!InventoryWidgetController)
		{
			InventoryWidgetController = NewObject<USFInventoryWidgetController>(this);
		}
		if (InventoryWidgetController)
		{
			InventoryWidgetController->Initialize(InventoryComponent, EquipmentComponent);
		}
	}

	if (EquipmentComponent)
	{
		if (!EquipmentWidgetController)
		{
			EquipmentWidgetController = NewObject<USFEquipmentWidgetController>(this);
		}
		if (EquipmentWidgetController)
		{
			EquipmentWidgetController->Initialize(EquipmentComponent);
		}
	}
}

void ASFPlayerController::InitializeHUDWidget()
{
	if (!PlayerHUDWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] PlayerHUDWidgetClass is not set."), *GetName());
		return;
	}

	if (PlayerHUDWidget)
	{
		return;
	}

	PlayerHUDWidget = CreateWidget<USFUserWidgetBase>(this, PlayerHUDWidgetClass);
	if (!PlayerHUDWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] Failed to create PlayerHUDWidget."), *GetName());
		return;
	}

	PlayerHUDWidget->SetPlayerHUDWidgetController(PlayerHUDWidgetController);
	PlayerHUDWidget->AddToViewport();

	RefreshAbilityBar();
}

void ASFPlayerController::SetTrackedEnemy(ASFCharacterBase* InEnemyCharacter)
{
	UE_LOG(LogTemp, Verbose, TEXT("[%s] SetTrackedEnemy: %s"),
		*GetName(),
		InEnemyCharacter ? *InEnemyCharacter->GetName() : TEXT("None"));

	if (PlayerHUDWidgetController)
	{
		PlayerHUDWidgetController->SetTrackedEnemy(InEnemyCharacter);
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("[%s] PlayerHUDWidgetController was null in SetTrackedEnemy."), *GetName());
	}
}

void ASFPlayerController::RefreshAbilityBar()
{
	if (AbilityBarWidgetController)
	{
		AbilityBarWidgetController->RefreshAbilitySlots();
	}
}

void ASFPlayerController::TogglePlayerMenu()
{
	if (bIsPlayerMenuOpen)
	{
		ClosePlayerMenu();
	}
	else
	{
		OpenPlayerMenu();
	}
}

void ASFPlayerController::OpenPlayerMenu()
{
	if (bIsPlayerMenuOpen)
	{
		return;
	}

	if (!PlayerMenuWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] OpenPlayerMenu called but PlayerMenuWidgetClass is null."), *GetName());
		return;
	}

	// Hide HUD while menu is open
	if (PlayerHUDWidget)
	{
		PlayerHUDWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Create the menu widget if needed
	if (!PlayerMenuWidget)
	{
		PlayerMenuWidget = CreateWidget<UUserWidget>(this, PlayerMenuWidgetClass);

		if (USFPlayerMenuWidget* Menu = Cast<USFPlayerMenuWidget>(PlayerMenuWidget))
		{
			Menu->InitializeMenu(PlayerHUDWidgetController);
		}
	}
	else if (USFPlayerMenuWidget* ExistingMenu = Cast<USFPlayerMenuWidget>(PlayerMenuWidget))
	{
		// Menu was created earlier (e.g. before a quest was started). Re-run
		// initialization so per-panel controllers re-pull their data sources
		// (narrative component, equipment, inventory).
		ExistingMenu->InitializeMenu(PlayerHUDWidgetController);
	}

	if (!PlayerMenuWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("[%s] Failed to create PlayerMenuWidget."), *GetName());
		return;
	}

	RefreshPlayerMenuPreview();

	// Switch camera to preview, if available
	if (PlayerMenuPreviewScene && PlayerMenuPreviewScene->GetPreviewCamera())
	{
		SetViewTargetWithBlend(PlayerMenuPreviewScene, 0.2f);
	}

	// Add to viewport if not already visible
	if (!PlayerMenuWidget->IsInViewport())
	{
		PlayerMenuWidget->AddToViewport();
	}

	// Input mode: Game & UI so controller still gets IA_PlayerMenu
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	if (PlayerMenuWidget)
	{
		InputMode.SetWidgetToFocus(PlayerMenuWidget->TakeWidget());
	}

	SetInputMode(InputMode);
	bShowMouseCursor = true;

	bIsPlayerMenuOpen = true;
}

void ASFPlayerController::ClosePlayerMenu()
{
	if (!bIsPlayerMenuOpen)
	{
		return;
	}

	// Show HUD again
	if (PlayerHUDWidget)
	{
		PlayerHUDWidget->SetVisibility(ESlateVisibility::Visible);
	}

	// Remove the menu from viewport (we keep the instance cached for reuse)
	if (PlayerMenuWidget && PlayerMenuWidget->IsInViewport())
	{
		PlayerMenuWidget->RemoveFromParent();
	}

	// Restore camera to pawn
	if (GetPawn())
	{
		SetViewTargetWithBlend(GetPawn(), 0.2f);
	}

	// Restore pure game input
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;

	bIsPlayerMenuOpen = false;
}

void ASFPlayerController::RefreshPlayerMenuPreview()
{
	if (!PlayerMenuPreviewScene)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[%s] PlayerMenuPreviewScene is not assigned."), *GetName());
		return;
	}

	ACharacter* ControlledCharacter = Cast<ACharacter>(GetPawn());
	if (!ControlledCharacter)
	{
		return;
	}

	CachedPreviewCharacter = PlayerMenuPreviewScene->GetOrSpawnPreviewCharacter();
	if (!CachedPreviewCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Failed to get or spawn preview character."), *GetName());
		return;
	}

	CachedPreviewCharacter->SyncFromSourceCharacter(ControlledCharacter);
}
void ASFPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ASFCharacterBase* SFChar = Cast<ASFCharacterBase>(InPawn);
	if (!SFChar)
	{
		return;
	}

	// New pawn = fresh OnCharacterDied delegate. AddUnique is idempotent so
	// the initial possession (where BeginPlay also binds) doesn't double-fire.
	SFChar->OnCharacterDied.AddUniqueDynamic(this, &ASFPlayerController::HandlePawnDied);

	// If we're coming back from a death, the HUD widget controllers are still
	// pointing at the destroyed pawn. Re-initialize them against the fresh
	// pawn so health/echo/shields/stamina/ability/equipment bindings come
	// from the new attribute set + components. Initialize() is idempotent
	// and unbinds from the previous owner internally.
	if (PendingRespawnLoadout.bHasSnapshot && IsLocalController())
	{
		InitializeUIControllers(SFChar);
	}

	// If we're coming back from a death, re-apply the loadout that was
	// captured at the moment of death. Equipment/inventory live on the pawn,
	// which got destroyed during RestartPlayerAtTransform; the controller is
	// the only thing that survives, so it holds the snapshot.
	if (PendingRespawnLoadout.bHasSnapshot)
	{
		RestoreLoadoutAfterRespawn(SFChar);
	}
}

void ASFPlayerController::SnapshotLoadoutForRespawn(ASFCharacterBase* DyingCharacter)
{
	PendingRespawnLoadout.Reset();

	if (!DyingCharacter)
	{
		return;
	}

	USFEquipmentComponent* Equipment = DyingCharacter->GetEquipmentComponent();
	if (!Equipment)
	{
		return;
	}

	// Pull every populated slot. We use FSFWeaponInstanceData because that's
	// what carries the perks/rolls -- equipping by raw WeaponData would lose
	// the player's god-roll on respawn.
	for (const TPair<ESFEquipmentSlot, FSFEquipmentSlotEntry>& Pair : Equipment->GetEquippedSlots())
	{
		if (!Pair.Value.bHasItemEquipped)
		{
			continue;
		}

		if (!Pair.Value.WeaponInstance.IsValid())
		{
			continue;
		}

		FSFRespawnLoadoutEntry Entry;
		Entry.Slot = Pair.Key;
		Entry.WeaponInstance = Pair.Value.WeaponInstance;
		PendingRespawnLoadout.Entries.Add(Entry);
	}

	PendingRespawnLoadout.ActiveSlot = Equipment->GetActiveWeaponSlot();

	// Inventory snapshot. Mirrors SFPlayerSaveService's save path so the
	// instance metadata (perks, rolls, stack counts) survives respawn rather
	// than being silently reset on the fresh DefaultPawnClass instance.
	if (USFInventoryComponent* Inventory = DyingCharacter->GetInventoryComponent())
	{
		PendingRespawnLoadout.InventoryEntries = Inventory->GetInventoryEntries();
	}

	PendingRespawnLoadout.bHasSnapshot =
		PendingRespawnLoadout.Entries.Num() > 0
		|| PendingRespawnLoadout.InventoryEntries.Num() > 0;
}

void ASFPlayerController::RestoreLoadoutAfterRespawn(ASFCharacterBase* FreshCharacter)
{
	if (!FreshCharacter || !PendingRespawnLoadout.bHasSnapshot)
	{
		return;
	}

	USFInventoryComponent* Inventory = FreshCharacter->GetInventoryComponent();
	if (!FreshCharacter->GetEquipmentComponent() || !Inventory)
	{
		// Pawn doesn't have equipment/inventory yet (probably mid-construction).
		// Defer one tick and try again. We hold onto the snapshot until it lands.
		TWeakObjectPtr<ASFPlayerController> WeakSelf(this);
		TWeakObjectPtr<ASFCharacterBase> WeakChar(FreshCharacter);
		GetWorldTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateLambda([WeakSelf, WeakChar]()
			{
				if (WeakSelf.IsValid() && WeakChar.IsValid())
				{
					WeakSelf->RestoreLoadoutAfterRespawn(WeakChar.Get());
				}
			}));
		return;
	}

	// Restore inventory FIRST so equipment slots that reference inventory
	// entries (via InventoryEntryId) line up with live entries. Inventory
	// restore is safe to run immediately -- it doesn't touch animation.
	if (PendingRespawnLoadout.InventoryEntries.Num() > 0)
	{
		Inventory->SetInventoryEntriesFromSave(PendingRespawnLoadout.InventoryEntries);
	}

	// Defer the equip pass by one tick. EquipWeaponInstance calls
	// LinkAnimClassLayers on the mesh's AnimInstance via
	// SetOverlayLinkedAnimLayer; if we run during OnPossess the AnimInstance
	// may not yet have completed InitAnim, in which case the link silently
	// no-ops and the upper-body overlay never engages on the fresh pawn.
	// One tick is enough for the mesh's InitAnim to land.
	TWeakObjectPtr<ASFPlayerController> WeakSelf(this);
	TWeakObjectPtr<ASFCharacterBase> WeakChar(FreshCharacter);
	GetWorldTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateLambda([WeakSelf, WeakChar]()
		{
			if (!WeakSelf.IsValid() || !WeakChar.IsValid())
			{
				return;
			}
			WeakSelf->ApplyEquipmentSnapshotToFreshPawn(WeakChar.Get());
		}));
}

void ASFPlayerController::ApplyEquipmentSnapshotToFreshPawn(ASFCharacterBase* FreshCharacter)
{
	if (!FreshCharacter || !PendingRespawnLoadout.bHasSnapshot)
	{
		return;
	}

	USFEquipmentComponent* Equipment = FreshCharacter->GetEquipmentComponent();
	if (!Equipment)
	{
		PendingRespawnLoadout.Reset();
		return;
	}

	// Re-equip every slot. EquipWeaponInstance internally calls
	// OnWeaponEquipped on the character, which runs ApplyWeaponAnimationFromData
	// and re-establishes CurrentOverlayMode + the linked upper-body anim layer
	// the anim instance reads from in NativeUpdateAnimation -- that is what
	// restores the overlays.
	const ESFEquipmentSlot SavedActiveSlot = PendingRespawnLoadout.ActiveSlot;

	for (const FSFRespawnLoadoutEntry& Entry : PendingRespawnLoadout.Entries)
	{
		if (Entry.Slot == ESFEquipmentSlot::None || !Entry.WeaponInstance.IsValid())
		{
			continue;
		}

		Equipment->EquipWeaponInstance(Entry.WeaponInstance, Entry.Slot);
	}

	// Make sure the slot the player had drawn at the time of death is the one
	// drawn now -- otherwise the overlay/abilities would reflect whichever
	// slot was equipped last in the loop. SetActiveWeaponSlot routes through
	// EquipWeaponInstance again, which will re-fire OnWeaponEquipped with the
	// active slot's data, so ApplyWeaponAnimationFromData lands on the slot
	// the player actually had drawn.
	if (SavedActiveSlot != ESFEquipmentSlot::None
		&& SavedActiveSlot != Equipment->GetActiveWeaponSlot())
	{
		Equipment->SetActiveWeaponSlot(SavedActiveSlot);
	}
	else if (SavedActiveSlot != ESFEquipmentSlot::None)
	{
		// Active slot already matches (e.g. the loop ended on it). Re-apply
		// animation explicitly from the equipped weapon data so the freshly
		// initialized AnimInstance picks up the overlay even if the equip
		// call short-circuited because the instance id matched.
		if (const USFWeaponData* ActiveWeaponData = Equipment->GetCurrentWeaponData())
		{
			FreshCharacter->ApplyWeaponAnimationFromData(ActiveWeaponData);
		}
	}

	// Belt-and-suspenders: even after the equip pass above, force a refresh
	// of the linked anim layer against whatever ended up active. This catches
	// the edge case where the mesh's AnimInstance was rebuilt between the
	// equip call and now (e.g. a BP-driven mesh swap on possess) which would
	// otherwise drop the LinkAnimClassLayers state.
	FreshCharacter->RefreshOverlayLinkedAnimLayer();

	// Consumed -- a non-death possess later (e.g. a future cinematic body-swap)
	// shouldn't replay this snapshot.
	PendingRespawnLoadout.Reset();
}

void ASFPlayerController::HandlePawnDied(ASFCharacterBase* DeadCharacter, ASFCharacterBase* /*Killer*/)
{
	if (!DeadCharacter || DeadCharacter != GetPawn())
	{
		return;
	}

	LastDeathLocation = DeadCharacter->GetActorLocation();

	// Snapshot the loadout NOW, before the death screen runs and the game mode
	// destroys the pawn. The fresh pawn is built from DefaultPawnClass with no
	// memory of what was equipped, so anim overlays (driven by the equipped
	// weapon profile) would otherwise reset to Unarmed on respawn.
	SnapshotLoadoutForRespawn(DeadCharacter);

	// Dark-zone status is sticky for the death: we resolve it once here at
	// the moment of death so a body sliding across the dark-zone boundary
	// during ragdoll doesn't flip the respawn rules under the player.
	ASFDarkZoneVolume* ContainingZone = nullptr;
	const bool bIsDarkZone = ASFDarkZoneVolume::IsLocationInDarkZone(this, LastDeathLocation, ContainingZone);

	FText ZoneOrCheckpointName;
	if (bIsDarkZone && ContainingZone)
	{
		ZoneOrCheckpointName = ContainingZone->DarkZoneDisplayName;
	}

	if (!DeathScreenWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SFPlayerController] HandlePawnDied: DeathScreenWidgetClass is not set on the PlayerController BP; respawning immediately."));
		RespawnFromDeathScreen(bIsDarkZone);
		return;
	}

	if (!DeathScreenWidget)
	{
		DeathScreenWidget = CreateWidget<USFDeathScreenWidget>(this, DeathScreenWidgetClass);
	}

	if (DeathScreenWidget)
	{
		DeathScreenWidget->InitializeDeathScreen(bIsDarkZone, ZoneOrCheckpointName);

		if (!DeathScreenWidget->IsInViewport())
		{
			DeathScreenWidget->AddToViewport(100);
		}

		// Mouse + UI input so the player can click the respawn button.
		// Game input stays off until the pawn is back; respawn flow toggles
		// it via the inherited input mode helpers.
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(DeathScreenWidget->TakeWidget());
		SetInputMode(Mode);
		bShowMouseCursor = true;
	}
}

void ASFPlayerController::RespawnFromDeathScreen(bool bRestartFromCheckpoint)
{
	if (DeathScreenWidget)
	{
		DeathScreenWidget->RemoveFromParent();
		DeathScreenWidget = nullptr;
	}

	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;

	if (ASFGameModeBase* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ASFGameModeBase>() : nullptr)
	{
		GM->HandlePlayerRespawnRequest(this, bRestartFromCheckpoint, LastDeathLocation);
	}
}
