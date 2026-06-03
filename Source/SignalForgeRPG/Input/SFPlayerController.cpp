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

	// Hook death once at startup. The pawn pointer can change across
	// possess/unpossess but for the local single-player session we only have
	// one pawn; if you add seamless pawn swapping later, rebind in OnPossess
	// instead.
	PlayerCharacter->OnCharacterDied.AddDynamic(this, &ASFPlayerController::HandlePawnDied);

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

	PlayerHUDWidgetController = NewObject<USFPlayerHUDWidgetController>(this);
	if (PlayerHUDWidgetController)
	{
		PlayerHUDWidgetController->Initialize(PlayerCharacter);
	}

	AbilityBarWidgetController = NewObject<USFAbilityBarWidgetController>(this);
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
		InventoryWidgetController = NewObject<USFInventoryWidgetController>(this);
		if (InventoryWidgetController)
		{
			InventoryWidgetController->Initialize(InventoryComponent, EquipmentComponent);
		}
	}

	if (EquipmentComponent)
	{
		EquipmentWidgetController = NewObject<USFEquipmentWidgetController>(this);
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
void ASFPlayerController::HandlePawnDied(ASFCharacterBase* DeadCharacter, ASFCharacterBase* /*Killer*/)
{
	if (!DeadCharacter || DeadCharacter != GetPawn())
	{
		return;
	}

	LastDeathLocation = DeadCharacter->GetActorLocation();

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
