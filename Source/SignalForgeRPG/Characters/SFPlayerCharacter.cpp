#include "Characters/SFPlayerCharacter.h"

#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/SFGameplayAbility.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "Core/SignalForgeDeveloperSettings.h"
#include "Core/SignalForgeGameplayTags.h"
#include "Dialogue/Data/SFDialogueComponent.h"
#include "Dialogue/SFDialogueCameraComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Input/SFPlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Components/SFEquipmentComponent.h"
#include "Components/SFInteractionComponent.h"
#include "Components/SFInventoryComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "UI/SFPlayerHUDWidgetController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Light.h"
#include "Combat/SFWeaponData.h"

ASFPlayerCharacter::ASFPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = true;
	}

	PlayerClassComponent = CreateDefaultSubobject<USFPlayerClassComponent>(TEXT("PlayerClassComponent"));
	DialogueComponent = CreateDefaultSubobject<USFDialogueComponent>(TEXT("DialogueComponent"));
	DialogueCameraComponent = CreateDefaultSubobject<USFDialogueCameraComponent>(TEXT("DialogueCameraComponent"));

	DialogueCameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DialogueCameraRoot"));
	DialogueCameraRoot->SetupAttachment(GetMesh());

	DialogueCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("DialogueCamera"));
	DialogueCamera->SetupAttachment(DialogueCameraRoot);
	DialogueCamera->bAutoActivate = false;

	GameplayCameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("GameplayCameraBoom"));
	GameplayCameraBoom->SetupAttachment(GetRootComponent());
	GameplayCameraBoom->TargetArmLength = 350.0f;
	GameplayCameraBoom->bUsePawnControlRotation = true;
	GameplayCameraBoom->bDoCollisionTest = true;
	GameplayCameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 70.0f));

	PortraitCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PortraitCapture"));
	PortraitCaptureComponent->SetupAttachment(GetMesh(), FName("head"));
	PortraitCaptureComponent->bAutoActivate = false;
	PortraitCaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
	PortraitCaptureComponent->bCaptureEveryFrame = false;
	PortraitCaptureComponent->bCaptureOnMovement = false;
	PortraitCaptureComponent->SetRelativeLocationAndRotation(
		FVector(25.0f, 0.0f, 5.0f),
		FRotator(-10.0f, 180.0f, 0.0f)
	);
}

void ASFPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	if (HasAuthority())
	{
		if (PlayerClassComponent && DefaultClassDefinition)
		{
			PlayerClassComponent->AssignClass(DefaultClassDefinition, /*bIsFirstTimeSetup=*/true);
		}

	}

	PortraitRenderTarget = NewObject<UTextureRenderTarget2D>(this);
	PortraitRenderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
	PortraitRenderTarget->ClearColor = FLinearColor(0.05f, 0.05f, 0.05f, 1.0f);
	PortraitRenderTarget->InitAutoFormat(256, 256);
	PortraitRenderTarget->UpdateResourceImmediate(true);

	if (PortraitCaptureComponent)
	{
		PortraitCaptureComponent->TextureTarget = PortraitRenderTarget;
		PortraitCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		PortraitCaptureComponent->ShowFlags.SetLighting(true);
		PortraitCaptureComponent->ShowFlags.SetDirectLighting(true);
		PortraitCaptureComponent->ShowFlags.SetDynamicShadows(false);
		PortraitCaptureComponent->ShowFlags.SetAmbientOcclusion(false);
		PortraitCaptureComponent->ShowFlags.SetPostProcessing(false);
		PortraitCaptureComponent->ShowFlags.SetMaterials(true);
		PortraitCaptureComponent->ShowFlags.SetSkeletalMeshes(true);
		PortraitCaptureComponent->ShowFlags.SetSkyLighting(true);
	}

	FTimerHandle PortraitCaptureHandle;
	GetWorldTimerManager().SetTimer(
		PortraitCaptureHandle,
		this,
		&ASFPlayerCharacter::CapturePortrait,
		0.1f,
		false);

	//if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
	//	LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	//{
	//	if (const USignalForgeDeveloperSettings* Settings = GetDefault<USignalForgeDeveloperSettings>())
	//	{
	//		if (Settings->DefaultInputMappingContext.IsValid())
	//		{
	//			DefaultInputMappingContext = Settings->DefaultInputMappingContext.Get();
	//		}
	//		else if (!Settings->DefaultInputMappingContext.ToSoftObjectPath().IsNull())
	//		{
	//			DefaultInputMappingContext = Settings->DefaultInputMappingContext.LoadSynchronous();
	//		}
	//	}

	//	/*if (DefaultInputMappingContext)
	//	{
	//		InputSubsystem->AddMappingContext(DefaultInputMappingContext, 0);
	//	}*/
	//}

	if (DialogueCameraComponent && DialogueComponent)
	{
		DialogueCameraComponent->InitializeFromDialogueComponent(DialogueComponent);
	}
}

void ASFPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->ProcessAbilityInput(DeltaTime, false);
	}

	// Smooth camera zoom
	if (GameplayCameraBoom)
	{
		GameplayCameraBoom->TargetArmLength = FMath::FInterpTo(
			GameplayCameraBoom->TargetArmLength,
			TargetZoomDistance,
			DeltaTime,
			ZoomInterpSpeed);
	}
}

void ASFPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent)
	{
		return;
	}

	if (MoveAction)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASFPlayerCharacter::Move);
	}

	if (LookAction)
	{
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASFPlayerCharacter::Look);
	}

	if (JumpAction)
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ASFPlayerCharacter::StartJump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ASFPlayerCharacter::StopJump);
	}

	if (CrouchAction)
	{
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &ASFPlayerCharacter::StartCrouchInput);
	}

	if (SprintAction)
	{
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &ASFPlayerCharacter::StartSprintInput);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ASFPlayerCharacter::StopSprintInput);
	}

	if (InteractAction)
	{
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Triggered, this, &ASFPlayerCharacter::HandleInteractInput);
	}

	if (TogglePlayerMenuAction)
	{
		EnhancedInputComponent->BindAction(
			TogglePlayerMenuAction,
			ETriggerEvent::Started,
			this,
			&ASFPlayerCharacter::HandleTogglePlayerMenuInput);
	}

	// Abilities
	if (Ability1Action)
	{
		EnhancedInputComponent->BindAction(Ability1Action, ETriggerEvent::Started, this, &ASFPlayerCharacter::OnAbility1Pressed);
		EnhancedInputComponent->BindAction(Ability1Action, ETriggerEvent::Completed, this, &ASFPlayerCharacter::OnAbility1Released);
	}
	if (Ability2Action)
	{
		EnhancedInputComponent->BindAction(Ability2Action, ETriggerEvent::Started, this, &ASFPlayerCharacter::OnAbility2Pressed);
		EnhancedInputComponent->BindAction(Ability2Action, ETriggerEvent::Completed, this, &ASFPlayerCharacter::OnAbility2Released);
	}
	if (Ability3Action)
	{
		EnhancedInputComponent->BindAction(Ability3Action, ETriggerEvent::Started, this, &ASFPlayerCharacter::OnAbility3Pressed);
		EnhancedInputComponent->BindAction(Ability3Action, ETriggerEvent::Completed, this, &ASFPlayerCharacter::OnAbility3Released);
	}
	if (Ability4Action)
	{
		EnhancedInputComponent->BindAction(Ability4Action, ETriggerEvent::Started, this, &ASFPlayerCharacter::OnAbility4Pressed);
		EnhancedInputComponent->BindAction(Ability4Action, ETriggerEvent::Completed, this, &ASFPlayerCharacter::OnAbility4Released);
	}
	if (Ability5Action)
	{
		EnhancedInputComponent->BindAction(Ability5Action, ETriggerEvent::Started, this, &ASFPlayerCharacter::OnAbility5Pressed);
		EnhancedInputComponent->BindAction(Ability5Action, ETriggerEvent::Completed, this, &ASFPlayerCharacter::OnAbility5Released);
	}
	if (Ability6Action)
	{
		EnhancedInputComponent->BindAction(Ability6Action, ETriggerEvent::Started, this, &ASFPlayerCharacter::OnAbility6Pressed);
		EnhancedInputComponent->BindAction(Ability6Action, ETriggerEvent::Completed, this, &ASFPlayerCharacter::OnAbility6Released);
	}
	if (Ability7Action)
	{
		EnhancedInputComponent->BindAction(Ability7Action, ETriggerEvent::Started, this, &ASFPlayerCharacter::OnAbility7Pressed);
		EnhancedInputComponent->BindAction(Ability7Action, ETriggerEvent::Completed, this, &ASFPlayerCharacter::OnAbility7Released);
	}
	if (Ability8Action)
	{
		EnhancedInputComponent->BindAction(Ability8Action, ETriggerEvent::Started, this, &ASFPlayerCharacter::OnAbility8Pressed);
		EnhancedInputComponent->BindAction(Ability8Action, ETriggerEvent::Completed, this, &ASFPlayerCharacter::OnAbility8Released);
	}
	if (Ability9Action)
	{
		EnhancedInputComponent->BindAction(Ability9Action, ETriggerEvent::Started, this, &ASFPlayerCharacter::OnAbility9Pressed);
		EnhancedInputComponent->BindAction(Ability9Action, ETriggerEvent::Completed, this, &ASFPlayerCharacter::OnAbility9Released);
	}
	if (Ability10Action)
	{
		EnhancedInputComponent->BindAction(Ability10Action, ETriggerEvent::Started, this, &ASFPlayerCharacter::OnAbility10Pressed);
		EnhancedInputComponent->BindAction(Ability10Action, ETriggerEvent::Completed, this, &ASFPlayerCharacter::OnAbility10Released);
	}

	if (CameraZoomAction)
	{
		EnhancedInputComponent->BindAction(CameraZoomAction, ETriggerEvent::Triggered, this, &ASFPlayerCharacter::HandleCameraZoom);
	}

	if (BlockAction)
	{
		EnhancedInputComponent->BindAction(
			BlockAction,
			ETriggerEvent::Started,
			this,
			&ASFPlayerCharacter::OnBlockPressed);

		EnhancedInputComponent->BindAction(
			BlockAction,
			ETriggerEvent::Completed,
			this,
			&ASFPlayerCharacter::OnBlockReleased);
	}
}

void ASFPlayerCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	if (!Controller)
	{
		return;
	}

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, MovementVector.Y);
	AddMovementInput(RightDirection, MovementVector.X);
}

void ASFPlayerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
}

void ASFPlayerCharacter::StartJump(const FInputActionValue& Value)
{
	Jump();
}

void ASFPlayerCharacter::StopJump(const FInputActionValue& Value)
{
	StopJumping();
}

void ASFPlayerCharacter::StartCrouchInput(const FInputActionValue& Value)
{
	bCrouchToggled = !bCrouchToggled;

	if (bCrouchToggled)
	{
		StartCrouch();
	}
	else
	{
		StopCrouch();
	}
}

void ASFPlayerCharacter::StopCrouchInput(const FInputActionValue& Value) { StopCrouch(); }

void ASFPlayerCharacter::StartSprintInput(const FInputActionValue& Value)
{
	ProcessAbilityInputPressed(FSignalForgeGameplayTags::Get().Input_Ability_Sprint);
}

void ASFPlayerCharacter::StopSprintInput(const FInputActionValue& Value)
{
	ProcessAbilityInputReleased(FSignalForgeGameplayTags::Get().Input_Ability_Sprint);
}

void ASFPlayerCharacter::OnBlockPressed(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("Player OnBlockPressed"));
	ProcessAbilityInputPressed(FSignalForgeGameplayTags::Get().Input_Ability_Block);
}

void ASFPlayerCharacter::OnBlockReleased(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("Player OnBlockReleased"));
	ProcessAbilityInputReleased(FSignalForgeGameplayTags::Get().Input_Ability_Block);
}

void ASFPlayerCharacter::HandleInteractInput(const FInputActionValue& Value)
{
	if (DialogueComponent && DialogueComponent->IsConversationActive())
	{
		DialogueComponent->AdvanceConversation();
		return;
	}

	if (InteractionComponent)
	{
		InteractionComponent->TryInteract();
	}
}

void ASFPlayerCharacter::ProcessAbilityInputPressed(const FGameplayTag& InputTag)
{
	if (!AbilitySystemComponent) { return; }
	AbilitySystemComponent->AbilityInputTagPressed(InputTag);
}

void ASFPlayerCharacter::OnAbility1Pressed(const FInputActionValue& Value)
{
	const FSignalForgeGameplayTags& GameplayTags = FSignalForgeGameplayTags::Get();
	ProcessAbilityInputPressed(GameplayTags.Input_Ability_1);
}

void ASFPlayerCharacter::OnAbility2Pressed(const FInputActionValue& Value)
{
	const FSignalForgeGameplayTags& GameplayTags = FSignalForgeGameplayTags::Get();
	ProcessAbilityInputPressed(GameplayTags.Input_Ability_2);
}

void ASFPlayerCharacter::OnAbility3Pressed(const FInputActionValue& Value)
{
	const FSignalForgeGameplayTags& GameplayTags = FSignalForgeGameplayTags::Get();
	ProcessAbilityInputPressed(GameplayTags.Input_Ability_3);
}

void ASFPlayerCharacter::OnAbility4Pressed(const FInputActionValue& Value)
{
	const FSignalForgeGameplayTags& GameplayTags = FSignalForgeGameplayTags::Get();
	ProcessAbilityInputPressed(GameplayTags.Input_Ability_4);
}

void ASFPlayerCharacter::OnAbility5Pressed(const FInputActionValue& Value)
{
	const FSignalForgeGameplayTags& GameplayTags = FSignalForgeGameplayTags::Get();
	ProcessAbilityInputPressed(GameplayTags.Input_Ability_5);
}

void ASFPlayerCharacter::OnAbility6Pressed(const FInputActionValue& Value)
{
	const FSignalForgeGameplayTags& GameplayTags = FSignalForgeGameplayTags::Get();
	ProcessAbilityInputPressed(GameplayTags.Input_Ability_6);
}

void ASFPlayerCharacter::OnAbility7Pressed(const FInputActionValue& Value)
{
	const FSignalForgeGameplayTags& GameplayTags = FSignalForgeGameplayTags::Get();
	ProcessAbilityInputPressed(GameplayTags.Input_Ability_7);
}

void ASFPlayerCharacter::OnAbility8Pressed(const FInputActionValue& Value)
{
	const FSignalForgeGameplayTags& GameplayTags = FSignalForgeGameplayTags::Get();
	ProcessAbilityInputPressed(GameplayTags.Input_Ability_8);
}

void ASFPlayerCharacter::OnAbility9Pressed(const FInputActionValue& Value)
{
	const FSignalForgeGameplayTags& GameplayTags = FSignalForgeGameplayTags::Get();
	ProcessAbilityInputPressed(GameplayTags.Input_Ability_9);
}

void ASFPlayerCharacter::OnAbility10Pressed(const FInputActionValue& Value)
{
	const FSignalForgeGameplayTags& GameplayTags = FSignalForgeGameplayTags::Get();
	ProcessAbilityInputPressed(GameplayTags.Input_Ability_10);
}

bool ASFPlayerCharacter::EquipInventoryItemAtIndex(int32 Index)
{
	if (!InventoryComponent || !EquipmentComponent)
	{
		return false;
	}

	USFItemDefinition* ItemDefinition = InventoryComponent->GetItemDefinitionAtIndex(Index);
	if (!ItemDefinition)
	{
		return false;
	}

	ESFEquipmentOpResult Result = EquipmentComponent->EquipItemDefinition(ItemDefinition);
	if (Result != ESFEquipmentOpResult::Success)
	{
		return false;
	}

	// If this item has a weapon definition, apply its animation profile
	if (const USFWeaponData* WeaponData = ItemDefinition->GetWeaponData())
	{
		ApplyWeaponAnimationFromData(WeaponData);
	}
	else
	{
		ClearWeaponAnimationProfile();
	}

	return true;
}

void ASFPlayerCharacter::HandleTogglePlayerMenuInput(const FInputActionValue& Value)
{
	if (ASFPlayerController* SFPlayerController = Cast<ASFPlayerController>(GetController()))
	{
		SFPlayerController->TogglePlayerMenu();
	}
}

void ASFPlayerCharacter::ProcessAbilityInputReleased(const FGameplayTag& InputTag)
{
	if (!AbilitySystemComponent) { return; }
	AbilitySystemComponent->AbilityInputTagReleased(InputTag);
}

void ASFPlayerCharacter::OnAbility1Released(const FInputActionValue& Value)
{
	ProcessAbilityInputReleased(FSignalForgeGameplayTags::Get().Input_Ability_1);
}
void ASFPlayerCharacter::OnAbility2Released(const FInputActionValue& Value)
{
	ProcessAbilityInputReleased(FSignalForgeGameplayTags::Get().Input_Ability_2);
}
void ASFPlayerCharacter::OnAbility3Released(const FInputActionValue& Value)
{
	ProcessAbilityInputReleased(FSignalForgeGameplayTags::Get().Input_Ability_3);
}
void ASFPlayerCharacter::OnAbility4Released(const FInputActionValue& Value)
{
	ProcessAbilityInputReleased(FSignalForgeGameplayTags::Get().Input_Ability_4);
}
void ASFPlayerCharacter::OnAbility5Released(const FInputActionValue& Value)
{
	ProcessAbilityInputReleased(FSignalForgeGameplayTags::Get().Input_Ability_5);
}
void ASFPlayerCharacter::OnAbility6Released(const FInputActionValue& Value)
{
	ProcessAbilityInputReleased(FSignalForgeGameplayTags::Get().Input_Ability_6);
}
void ASFPlayerCharacter::OnAbility7Released(const FInputActionValue& Value)
{
	ProcessAbilityInputReleased(FSignalForgeGameplayTags::Get().Input_Ability_7);
}
void ASFPlayerCharacter::OnAbility8Released(const FInputActionValue& Value)
{
	ProcessAbilityInputReleased(FSignalForgeGameplayTags::Get().Input_Ability_8);
}
void ASFPlayerCharacter::OnAbility9Released(const FInputActionValue& Value)
{
	ProcessAbilityInputReleased(FSignalForgeGameplayTags::Get().Input_Ability_9);
}
void ASFPlayerCharacter::OnAbility10Released(const FInputActionValue& Value)
{
	ProcessAbilityInputReleased(FSignalForgeGameplayTags::Get().Input_Ability_10);
}

void ASFPlayerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController) { return; }

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer) { return; }

	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		InputSubsystem->ClearAllMappings();

		if (const USignalForgeDeveloperSettings* Settings = GetDefault<USignalForgeDeveloperSettings>())
		{
			if (Settings->DefaultInputMappingContext.IsValid())
			{
				DefaultInputMappingContext = Settings->DefaultInputMappingContext.Get();
			}
			else if (!Settings->DefaultInputMappingContext.ToSoftObjectPath().IsNull())
			{
				DefaultInputMappingContext = Settings->DefaultInputMappingContext.LoadSynchronous();
			}
		}

		if (DefaultInputMappingContext)
		{
			InputSubsystem->AddMappingContext(DefaultInputMappingContext, 0);
		}
	}
}

void ASFPlayerCharacter::CapturePortrait()
{
	if (!PortraitCaptureComponent || !PortraitRenderTarget)
	{
		return;
	}

	PortraitCaptureComponent->CaptureScene();

	// Notify the HUD controller that the portrait is ready
	if (ASFPlayerController* PC = Cast<ASFPlayerController>(GetController()))
	{
		if (USFPlayerHUDWidgetController* HUDController = PC->GetPlayerHUDWidgetController())
		{
			HUDController->OnPortraitReady.Broadcast(PortraitRenderTarget);
		}
	}
}

void ASFPlayerCharacter::HandleCameraZoom(const FInputActionValue& Value)
{
	const float ScrollValue = Value.Get<float>();
	TargetZoomDistance = FMath::Clamp(
		TargetZoomDistance - (ScrollValue * ZoomSpeed),
		MinZoomDistance,
		MaxZoomDistance);
}

UCameraComponent* ASFPlayerCharacter::GetGameplayCamera() const
{
	// Find the Blueprint camera named "GameplayCamera"
	TArray<UCameraComponent*> Cameras;
	GetComponents<UCameraComponent>(Cameras);

	for (UCameraComponent* Cam : Cameras)
	{
		if (Cam && Cam->GetName() == TEXT("GameplayCamera"))
		{
			return Cam;
		}
	}

	return nullptr;
}