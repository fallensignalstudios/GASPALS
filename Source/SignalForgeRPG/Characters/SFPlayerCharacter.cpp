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
#include "Core/SFPlayerState.h"
#include "Narrative/SFNarrativeComponent.h"
#include "Narrative/SFNarrativeTypes.h"
#include "Narrative/SFQuestDatabase.h"
#include "Narrative/SFQuestDefinition.h"
#include "Narrative/SFQuestInstance.h"
#include "Narrative/SFQuestRuntime.h"

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

	// Server-only: kick off any quests the player should always have on spawn.
	// Deferred slightly so the PlayerState/NarrativeComponent are fully
	// initialized (PlayerState replication is up by BeginPlay on the server,
	// but the narrative subsystems may still be wiring themselves in PIE).
	if (HasAuthority() && DefaultStartingQuests.Num() > 0)
	{
		FTimerHandle StartingQuestsHandle;
		GetWorldTimerManager().SetTimer(
			StartingQuestsHandle,
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				USFNarrativeComponent* NarrativeComponent = GetNarrativeComponent();
				if (!NarrativeComponent)
				{
					UE_LOG(LogTemp, Warning, TEXT("[SFPlayerCharacter] Cannot start default quests — NarrativeComponent unavailable. Verify the active GameMode's PlayerStateClass is ASFPlayerState (or a subclass)."));
					return;
				}

				for (const TSoftObjectPtr<USFQuestDefinition>& SoftDef : DefaultStartingQuests)
				{
					if (SoftDef.IsNull())
					{
						continue;
					}

					USFQuestDefinition* QuestDef = SoftDef.LoadSynchronous();
					if (!QuestDef)
					{
						UE_LOG(LogTemp, Warning, TEXT("[SFPlayerCharacter] DefaultStartingQuests entry %s failed to load."), *SoftDef.ToString());
						continue;
					}

					const FPrimaryAssetId AssetId = QuestDef->GetPrimaryAssetId();
					if (NarrativeComponent->GetQuestInstanceByAssetId(AssetId) != nullptr)
					{
						// Already started (e.g. carried over from save). Skip silently.
						continue;
					}

					// We already have the loaded definition in hand — skip
					// StartQuestByAssetId so we don't depend on the QuestDatabase
					// or Asset Manager being able to resolve the id. Go straight
					// to the runtime.
					USFQuestRuntime* QuestRuntime = NarrativeComponent->GetQuestRuntime();
					if (!QuestRuntime)
					{
						UE_LOG(LogTemp, Warning,
							TEXT("[SFPlayerCharacter] Default quest %s skipped: NarrativeComponent has no QuestRuntime yet."),
							*AssetId.ToString());
						continue;
					}

					// Pre-validate the most common designer-side mistakes so we can
					// emit a precise warning instead of a generic "failed to start".
					if (QuestDef->InitialStateId.IsNone())
					{
						UE_LOG(LogTemp, Warning,
							TEXT("[SFPlayerCharacter] Default quest %s has an empty InitialStateId on its data asset (%s). Open the asset and set InitialStateId to the StateId of the state the quest should start in."),
							*AssetId.ToString(),
							*GetNameSafe(QuestDef));
						continue;
					}
					if (QuestDef->States.Num() == 0)
					{
						UE_LOG(LogTemp, Warning,
							TEXT("[SFPlayerCharacter] Default quest %s has zero entries in its States array (%s). Add at least one FSFQuestStateDefinition (its StateId must match InitialStateId)."),
							*AssetId.ToString(),
							*GetNameSafe(QuestDef));
						continue;
					}

					// Verify the InitialStateId actually matches one of the
					// FSFQuestStateDefinition.StateId values in the array. This
					// is the most common reason EnterState fails silently.
					bool bInitialStateExists = false;
					FString StateIdsList;
					for (const FSFQuestStateDefinition& StateDef : QuestDef->States)
					{
						if (!StateIdsList.IsEmpty()) { StateIdsList += TEXT(", "); }
						StateIdsList += StateDef.StateId.IsNone() ? FString(TEXT("<None>")) : StateDef.StateId.ToString();
						if (StateDef.StateId == QuestDef->InitialStateId)
						{
							bInitialStateExists = true;
						}
					}
					if (!bInitialStateExists)
					{
						UE_LOG(LogTemp, Warning,
							TEXT("[SFPlayerCharacter] Default quest %s: InitialStateId '%s' does not match any StateId in the States array of %s. Existing StateIds: [%s]. Fix the State entry's StateId field on the data asset so it equals '%s' (or change InitialStateId to one of the existing ids)."),
							*AssetId.ToString(),
							*QuestDef->InitialStateId.ToString(),
							*GetNameSafe(QuestDef),
							*StateIdsList,
							*QuestDef->InitialStateId.ToString());
						continue;
					}

					USFQuestInstance* Started = QuestRuntime->StartQuestByDefinition(QuestDef);
					if (!Started)
					{
						// Surface the remaining reasons in one log line so the
						// designer can act without digging into the runtime source.
						USFQuestDatabase* DB = NarrativeComponent->GetQuestDatabase();
						const bool bDbHasQuest = DB && DB->GetQuestById(AssetId) != nullptr;
						UE_LOG(LogTemp, Warning,
							TEXT("[SFPlayerCharacter] Failed to start default quest %s. Asset=%s, InitialStateId=%s, NumStates=%d, StateIds=[%s], HasAuthority=%d, Runtime=%s, QuestDatabase=%s, DBHasQuest=%d."),
							*AssetId.ToString(),
							*GetNameSafe(QuestDef),
							*QuestDef->InitialStateId.ToString(),
							QuestDef->States.Num(),
							*StateIdsList,
							HasAuthority() ? 1 : 0,
							*GetNameSafe(QuestRuntime),
							*GetNameSafe(DB),
							bDbHasQuest ? 1 : 0);
					}
					else
					{
						UE_LOG(LogTemp, Log,
							TEXT("[SFPlayerCharacter] Auto-started default quest %s (Asset=%s)."),
							*AssetId.ToString(),
							*GetNameSafe(QuestDef));
					}
				}
			}),
			0.25f,
			false);
	}
}

// ----------------------------------------------------------------------
// Narrative / Quest API
// ----------------------------------------------------------------------

ASFPlayerState* ASFPlayerCharacter::GetSFPlayerState() const
{
	return GetPlayerState<ASFPlayerState>();
}

USFNarrativeComponent* ASFPlayerCharacter::GetNarrativeComponent() const
{
	if (ASFPlayerState* SFPS = GetSFPlayerState())
	{
		return SFPS->GetNarrativeComponent();
	}
	return nullptr;
}

USFQuestInstance* ASFPlayerCharacter::StartQuestByDefinition(USFQuestDefinition* QuestDefinition, FName StartStateId)
{
	if (!QuestDefinition)
	{
		return nullptr;
	}

	// Prefer the direct runtime path: we already have the definition, no need
	// to round-trip through the QuestDatabase / Asset Manager resolver.
	if (USFNarrativeComponent* NarrativeComponent = GetNarrativeComponent())
	{
		if (USFQuestRuntime* QuestRuntime = NarrativeComponent->GetQuestRuntime())
		{
			return QuestRuntime->StartQuestByDefinition(QuestDefinition, StartStateId);
		}
	}

	// Fallback for completeness if the runtime isn't available.
	return StartQuestByAssetId(QuestDefinition->GetPrimaryAssetId(), StartStateId);
}

USFQuestInstance* ASFPlayerCharacter::StartQuestByAssetId(FPrimaryAssetId QuestAssetId, FName StartStateId)
{
	USFNarrativeComponent* NarrativeComponent = GetNarrativeComponent();
	if (!NarrativeComponent || !QuestAssetId.IsValid())
	{
		return nullptr;
	}
	return NarrativeComponent->StartQuestByAssetId(QuestAssetId, StartStateId);
}

bool ASFPlayerCharacter::RestartQuestByAssetId(FPrimaryAssetId QuestAssetId, FName StartStateId)
{
	USFNarrativeComponent* NarrativeComponent = GetNarrativeComponent();
	if (!NarrativeComponent || !QuestAssetId.IsValid())
	{
		return false;
	}
	return NarrativeComponent->RestartQuestByAssetId(QuestAssetId, StartStateId);
}

bool ASFPlayerCharacter::AbandonQuestByAssetId(FPrimaryAssetId QuestAssetId)
{
	USFNarrativeComponent* NarrativeComponent = GetNarrativeComponent();
	if (!NarrativeComponent || !QuestAssetId.IsValid())
	{
		return false;
	}
	return NarrativeComponent->AbandonQuestByAssetId(QuestAssetId);
}

void ASFPlayerCharacter::SF_StartQuest(const FString& AssetIdString)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SFPlayerCharacter] SF_StartQuest must be run on the server (authority)."));
		return;
	}

	const FPrimaryAssetId AssetId = FPrimaryAssetId::FromString(AssetIdString);
	if (!AssetId.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SFPlayerCharacter] SF_StartQuest: '%s' is not a valid PrimaryAssetId. Expected form: 'Quest:Q_MyQuestId'."), *AssetIdString);
		return;
	}

	USFQuestInstance* Started = StartQuestByAssetId(AssetId);
	if (Started)
	{
		UE_LOG(LogTemp, Log, TEXT("[SFPlayerCharacter] SF_StartQuest: started %s."), *AssetId.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SFPlayerCharacter] SF_StartQuest: failed to start %s. Confirm the quest is registered with the QuestDatabase and the asset is loaded."), *AssetId.ToString());
	}
}

void ASFPlayerCharacter::SF_AbandonQuest(const FString& AssetIdString)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SFPlayerCharacter] SF_AbandonQuest must be run on the server (authority)."));
		return;
	}

	const FPrimaryAssetId AssetId = FPrimaryAssetId::FromString(AssetIdString);
	if (!AssetId.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SFPlayerCharacter] SF_AbandonQuest: '%s' is not a valid PrimaryAssetId."), *AssetIdString);
		return;
	}

	const bool bAbandoned = AbandonQuestByAssetId(AssetId);
	UE_CLOG(bAbandoned, LogTemp, Log, TEXT("[SFPlayerCharacter] SF_AbandonQuest: abandoned %s."), *AssetId.ToString());
	UE_CLOG(!bAbandoned, LogTemp, Warning, TEXT("[SFPlayerCharacter] SF_AbandonQuest: no in-progress quest with id %s."), *AssetId.ToString());
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

	UpdateRecoil(DeltaTime);
}

void ASFPlayerCharacter::ApplyRecoilKick(
	float PitchDegrees,
	float YawDegrees,
	float InterpSpeed,
	float RecoverySpeed,
	float RecoveryFraction)
{
	// Accumulate the new kick on top of whatever is still being absorbed from the
	// previous shot (rapid fire stacks). Pitch is stored as a positive "upward"
	// value; UpdateRecoil converts it to the negative-pitch input UE expects.
	RecoilPitchPending += FMath::Max(0.0f, PitchDegrees);
	RecoilYawPending += YawDegrees;

	// Latest weapon wins for the smoothing curve — keeps per-weapon tuning honest.
	RecoilInterpSpeedCached = FMath::Max(0.01f, InterpSpeed);
	RecoilRecoverySpeedCached = FMath::Max(0.0f, RecoverySpeed);
	RecoilRecoveryFractionCached = FMath::Clamp(RecoveryFraction, 0.0f, 1.0f);
}

void ASFPlayerCharacter::UpdateRecoil(float DeltaTime)
{
	if (DeltaTime <= 0.0f)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	// --- Kick phase: bleed the pending pitch/yaw into the controller this frame ---
	if (!FMath::IsNearlyZero(RecoilPitchPending, 0.0001f))
	{
		const float Step = FMath::FInterpTo(0.0f, RecoilPitchPending, DeltaTime, RecoilInterpSpeedCached);
		const float ClampedStep = FMath::Min(Step, RecoilPitchPending);
		if (ClampedStep > 0.0f)
		{
			PC->AddPitchInput(-ClampedStep); // negative pitch = camera up
			RecoilPitchPending -= ClampedStep;
			RecoilPitchAbsorbed += ClampedStep;
		}
	}
	else if (!FMath::IsNearlyZero(RecoilPitchAbsorbed, 0.0001f) && RecoilRecoverySpeedCached > 0.0f)
	{
		// Recovery phase: push the camera back down by a fraction of what we kicked it up.
		const float Target = -RecoilPitchAbsorbed * RecoilRecoveryFractionCached;
		const float Step = FMath::FInterpTo(0.0f, -Target, DeltaTime, RecoilRecoverySpeedCached);
		const float Applied = FMath::Min(Step, RecoilPitchAbsorbed);
		if (Applied > 0.0f)
		{
			PC->AddPitchInput(Applied * RecoilRecoveryFractionCached); // positive pitch = camera down
			RecoilPitchAbsorbed -= Applied;
		}
	}

	if (!FMath::IsNearlyZero(RecoilYawPending, 0.0001f))
	{
		const float Direction = FMath::Sign(RecoilYawPending);
		const float Magnitude = FMath::Abs(RecoilYawPending);
		const float StepMag = FMath::FInterpTo(0.0f, Magnitude, DeltaTime, RecoilInterpSpeedCached);
		const float Step = FMath::Min(StepMag, Magnitude) * Direction;
		if (!FMath::IsNearlyZero(Step))
		{
			PC->AddYawInput(Step);
			RecoilYawPending -= Step;
			RecoilYawAbsorbed += Step;
		}
	}
	else if (!FMath::IsNearlyZero(RecoilYawAbsorbed, 0.0001f) && RecoilRecoverySpeedCached > 0.0f)
	{
		const float Magnitude = FMath::Abs(RecoilYawAbsorbed);
		const float Direction = FMath::Sign(RecoilYawAbsorbed);
		const float StepMag = FMath::FInterpTo(0.0f, Magnitude, DeltaTime, RecoilRecoverySpeedCached);
		const float Applied = FMath::Min(StepMag, Magnitude);
		if (Applied > 0.0f)
		{
			// Push yaw the opposite way to absorb part of the lateral kick.
			PC->AddYawInput(-Direction * Applied * RecoilRecoveryFractionCached);
			RecoilYawAbsorbed -= Direction * Applied;
		}
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

	// Weapon inputs — routed through InputTag so the equipped weapon's granted ability responds.
	// Each binding pairs Started + Completed so WhileInputActive abilities (e.g. ADS) receive
	// release events as well.
	if (PrimaryFireAction)
	{
		EnhancedInputComponent->BindAction(PrimaryFireAction, ETriggerEvent::Started, this, &ASFPlayerCharacter::OnPrimaryFirePressed);
		EnhancedInputComponent->BindAction(PrimaryFireAction, ETriggerEvent::Completed, this, &ASFPlayerCharacter::OnPrimaryFireReleased);
	}

	if (ADSAction)
	{
		EnhancedInputComponent->BindAction(ADSAction, ETriggerEvent::Started, this, &ASFPlayerCharacter::OnADSPressed);
		EnhancedInputComponent->BindAction(ADSAction, ETriggerEvent::Completed, this, &ASFPlayerCharacter::OnADSReleased);
	}

	if (ReloadAction)
	{
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ASFPlayerCharacter::OnReloadPressed);
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Completed, this, &ASFPlayerCharacter::OnReloadReleased);
	}

	if (GrenadeAction)
	{
		EnhancedInputComponent->BindAction(GrenadeAction, ETriggerEvent::Started, this, &ASFPlayerCharacter::OnGrenadePressed);
		EnhancedInputComponent->BindAction(GrenadeAction, ETriggerEvent::Completed, this, &ASFPlayerCharacter::OnGrenadeReleased);
	}

	if (SecondaryFireAction)
	{
		EnhancedInputComponent->BindAction(SecondaryFireAction, ETriggerEvent::Started, this, &ASFPlayerCharacter::OnSecondaryFirePressed);
		EnhancedInputComponent->BindAction(SecondaryFireAction, ETriggerEvent::Completed, this, &ASFPlayerCharacter::OnSecondaryFireReleased);
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

// --- Weapon input handlers ------------------------------------------------------------------
// These do not know or care which weapon is equipped. They translate a physical button into
// an InputTag and hand it to the ASC; whichever ability the currently-equipped weapon granted
// with a matching InputTag (e.g. USFGameplayAbility_WeaponFire for a rifle, or
// USFGameplayAbility_AttackLight for a sword) is the one that fires.

void ASFPlayerCharacter::OnPrimaryFirePressed(const FInputActionValue& Value)
{
	ProcessAbilityInputPressed(FSignalForgeGameplayTags::Get().Input_PrimaryFire);
}

void ASFPlayerCharacter::OnPrimaryFireReleased(const FInputActionValue& Value)
{
	ProcessAbilityInputReleased(FSignalForgeGameplayTags::Get().Input_PrimaryFire);
}

void ASFPlayerCharacter::OnADSPressed(const FInputActionValue& Value)
{
	ProcessAbilityInputPressed(FSignalForgeGameplayTags::Get().Input_ADS);
}

void ASFPlayerCharacter::OnADSReleased(const FInputActionValue& Value)
{
	ProcessAbilityInputReleased(FSignalForgeGameplayTags::Get().Input_ADS);
}

void ASFPlayerCharacter::OnReloadPressed(const FInputActionValue& Value)
{
	ProcessAbilityInputPressed(FSignalForgeGameplayTags::Get().Input_Reload);
}

void ASFPlayerCharacter::OnReloadReleased(const FInputActionValue& Value)
{
	ProcessAbilityInputReleased(FSignalForgeGameplayTags::Get().Input_Reload);
}

void ASFPlayerCharacter::OnGrenadePressed(const FInputActionValue& Value)
{
	ProcessAbilityInputPressed(FSignalForgeGameplayTags::Get().Input_Grenade);
}

void ASFPlayerCharacter::OnGrenadeReleased(const FInputActionValue& Value)
{
	ProcessAbilityInputReleased(FSignalForgeGameplayTags::Get().Input_Grenade);
}

void ASFPlayerCharacter::OnSecondaryFirePressed(const FInputActionValue& Value)
{
	ProcessAbilityInputPressed(FSignalForgeGameplayTags::Get().Input_SecondaryFire);
}

void ASFPlayerCharacter::OnSecondaryFireReleased(const FInputActionValue& Value)
{
	ProcessAbilityInputReleased(FSignalForgeGameplayTags::Get().Input_SecondaryFire);
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