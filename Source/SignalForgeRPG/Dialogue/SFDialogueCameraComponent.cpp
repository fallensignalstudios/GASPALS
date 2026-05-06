#include "Dialogue/SFDialogueCameraComponent.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Characters/SFPlayerCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Dialogue/Data/SFDialogueComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "DefaultLevelSequenceInstanceData.h"

USFDialogueCameraComponent::USFDialogueCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USFDialogueCameraComponent::BeginPlay()
{
	Super::BeginPlay();

	CachePlayerController();

	if (const ASFPlayerCharacter* PlayerCharacter = Cast<ASFPlayerCharacter>(GetOwner()))
	{
		DialogueCameraRoot = PlayerCharacter->GetDialogueCameraRoot();
		DialogueCamera = PlayerCharacter->GetDialogueCamera();
	}
}

void USFDialogueCameraComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopActiveSequence();
	Super::EndPlay(EndPlayReason);
}

void USFDialogueCameraComponent::InitializeFromDialogueComponent(USFDialogueComponent* InDialogueComponent)
{
	if (DialogueComponent == InDialogueComponent)
	{
		return;
	}

	if (DialogueComponent)
	{
		DialogueComponent->OnDialogueCameraShotChanged.RemoveDynamic(this, &USFDialogueCameraComponent::HandleDialogueCameraShotChanged);
		DialogueComponent->OnConversationEnded.RemoveDynamic(this, &USFDialogueCameraComponent::HandleConversationEnded);
	}

	DialogueComponent = InDialogueComponent;

	if (!DialogueComponent)
	{
		return;
	}

	DialogueComponent->OnDialogueCameraShotChanged.RemoveDynamic(this, &USFDialogueCameraComponent::HandleDialogueCameraShotChanged);
	DialogueComponent->OnConversationEnded.RemoveDynamic(this, &USFDialogueCameraComponent::HandleConversationEnded);

	DialogueComponent->OnDialogueCameraShotChanged.AddDynamic(this, &USFDialogueCameraComponent::HandleDialogueCameraShotChanged);
	DialogueComponent->OnConversationEnded.AddDynamic(this, &USFDialogueCameraComponent::HandleConversationEnded);
}

void USFDialogueCameraComponent::HandleDialogueCameraShotChanged(const FSFDialogueCameraShot& CameraShot)
{
	if (!CameraShot.IsValidShot())
	{
		RestoreGameplayCamera();
		return;
	}

	ApplyShot(CameraShot);
}

void USFDialogueCameraComponent::HandleConversationEnded()
{
	RestoreGameplayCamera();
}

void USFDialogueCameraComponent::HandleSequenceFinished()
{
	/*if (ActiveCameraMode == ESFActiveDialogueCameraMode::LevelSequence)
	{
		RestoreGameplayCamera();
	}*/
}

void USFDialogueCameraComponent::CachePlayerController()
{
	if (CachedPlayerController)
	{
		return;
	}

	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		CachedPlayerController = Cast<APlayerController>(OwnerPawn->GetController());
	}
}

void USFDialogueCameraComponent::StopActiveSequence()
{
	if (ActiveSequencePlayer)
	{
		ActiveSequencePlayer->OnFinished.RemoveDynamic(this, &USFDialogueCameraComponent::HandleSequenceFinished);
		ActiveSequencePlayer->Stop();
	}

	if (ActiveSequenceActor)
	{
		ActiveSequenceActor->Destroy();
	}

	ActiveSequencePlayer = nullptr;
	ActiveSequenceActor = nullptr;
}

AActor* USFDialogueCameraComponent::ResolveShotTarget(const FSFDialogueCameraShot& CameraShot) const
{
	if (!DialogueComponent)
	{
		return nullptr;
	}

	switch (CameraShot.TargetType)
	{
	case ESFDialogueShotTargetType::SourceActor:
		return DialogueComponent->GetActiveSourceActor();

	case ESFDialogueShotTargetType::PlayerActor:
		return GetOwner();

	case ESFDialogueShotTargetType::CustomActor:
		return CameraShot.CustomTargetActor.Get();

	case ESFDialogueShotTargetType::None:
	default:
		return nullptr;
	}
}

bool USFDialogueCameraComponent::ResolveSocketTransform(AActor* TargetActor, const FName SocketName, FTransform& OutTransform) const
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	if (SocketName == NAME_None)
	{
		OutTransform = TargetActor->GetActorTransform();
		return true;
	}

	if (const ACharacter* Character = Cast<ACharacter>(TargetActor))
	{
		if (USkeletalMeshComponent* MeshComp = Character->GetMesh())
		{
			if (MeshComp->DoesSocketExist(SocketName))
			{
				OutTransform = MeshComp->GetSocketTransform(SocketName, RTS_World);
				return true;
			}
		}
	}

	TArray<USceneComponent*> SceneComponents;
	TargetActor->GetComponents<USceneComponent>(SceneComponents);

	for (USceneComponent* SceneComp : SceneComponents)
	{
		if (SceneComp && SceneComp->DoesSocketExist(SocketName))
		{
			OutTransform = SceneComp->GetSocketTransform(SocketName, RTS_World);
			return true;
		}
	}

	OutTransform = TargetActor->GetActorTransform();
	return true;
}

void USFDialogueCameraComponent::ApplyShot(const FSFDialogueCameraShot& CameraShot)
{
	CachePlayerController();

	if (!CachedPlayerController)
	{
		return;
	}

	switch (CameraShot.SourceMode)
	{
	case ESFDialogueCameraSourceMode::CameraActor:
		ApplyCameraActorShot(CameraShot);
		break;

	case ESFDialogueCameraSourceMode::LevelSequence:
		ApplyLevelSequenceShot(CameraShot);
		break;

	case ESFDialogueCameraSourceMode::Procedural:
	default:
		ApplyProceduralShot(CameraShot);
		break;
	}
}

void USFDialogueCameraComponent::ApplyProceduralShot(const FSFDialogueCameraShot& CameraShot)
{
	StopActiveSequence();

	if (!CachedPlayerController || !DialogueCameraRoot || !DialogueCamera)
	{
		return;
	}

	AActor* TargetActor = ResolveShotTarget(CameraShot);
	if (!IsValid(TargetActor))
	{
		return;
	}

	if (const ASFPlayerCharacter* Player = Cast<ASFPlayerCharacter>(GetOwner()))
	{
		if (UCameraComponent* GameplayCam = Player->GetGameplayCamera())
		{
			GameplayCam->Deactivate();
		}
	}

	FTransform TargetBaseTransform;
	if (!ResolveSocketTransform(TargetActor, CameraShot.SocketName, TargetBaseTransform))
	{
		return;
	}

	const FTransform RelativeTransform(CameraShot.RelativeRotation, CameraShot.RelativeLocation);
	const FTransform FinalTransform = RelativeTransform * TargetBaseTransform;

	DialogueCameraRoot->SetWorldLocationAndRotation(
		FinalTransform.GetLocation(),
		FinalTransform.GetRotation().Rotator()
	);

	DialogueCamera->SetFieldOfView(CameraShot.FOV);
	DialogueCamera->Activate();

	CachedPlayerController->SetViewTargetWithBlend(GetOwner(), CameraShot.BlendTime);

	ActiveCameraMode = ESFActiveDialogueCameraMode::ProceduralCamera;
	bUsingDialogueCamera = true;
}

void USFDialogueCameraComponent::ApplyCameraActorShot(const FSFDialogueCameraShot& CameraShot)
{
	StopActiveSequence();

	CachePlayerController();

	if (!CachedPlayerController)
	{
		return;
	}

	if (!IsValid(CameraShot.CameraActor))
	{
		ApplyProceduralShot(CameraShot);
		return;
	}

	if (const ASFPlayerCharacter* Player = Cast<ASFPlayerCharacter>(GetOwner()))
	{
		if (UCameraComponent* GameplayCam = Player->GetGameplayCamera())
		{
			GameplayCam->Deactivate();
		}
	}

	if (DialogueCamera)
	{
		DialogueCamera->Deactivate();
	}

	CachedPlayerController->SetViewTargetWithBlend(CameraShot.CameraActor, CameraShot.BlendTime);

	ActiveCameraMode = ESFActiveDialogueCameraMode::CameraActor;
	bUsingDialogueCamera = true;
}

void USFDialogueCameraComponent::ApplyLevelSequenceShot(const FSFDialogueCameraShot& CameraShot)
{
	CachePlayerController();

	if (!CachedPlayerController)
	{
		return;
	}

	if (CameraShot.LevelSequence.IsNull())
	{
		ApplyProceduralShot(CameraShot);
		return;
	}

	ULevelSequence* SequenceAsset = CameraShot.LevelSequence.LoadSynchronous();
	if (!IsValid(SequenceAsset))
	{
		ApplyProceduralShot(CameraShot);
		return;
	}

	StopActiveSequence();

	if (const ASFPlayerCharacter* Player = Cast<ASFPlayerCharacter>(GetOwner()))
	{
		if (UCameraComponent* GameplayCam = Player->GetGameplayCamera())
		{
			GameplayCam->Deactivate();
		}
	}

	if (DialogueCamera)
	{
		DialogueCamera->Deactivate();
	}

	FMovieSceneSequencePlaybackSettings PlaybackSettings;
	PlaybackSettings.bAutoPlay = false;
	PlaybackSettings.bPauseAtEnd = true;

	ALevelSequenceActor* SpawnedSequenceActor = nullptr;

	ActiveSequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(
		GetWorld(),
		SequenceAsset,
		PlaybackSettings,
		SpawnedSequenceActor
	);

	ActiveSequenceActor = SpawnedSequenceActor;

	if (!ActiveSequencePlayer || !ActiveSequenceActor)
	{
		ActiveSequencePlayer = nullptr;
		ActiveSequenceActor = nullptr;
		ApplyProceduralShot(CameraShot);
		return;
	}

	AActor* SourceActor = DialogueComponent ? DialogueComponent->GetActiveSourceActor() : nullptr;
	AActor* PlayerActor = GetOwner();
	AActor* CustomActor = CameraShot.CustomTargetActor.Get();
	ActiveSequenceActor->ResetBindings();

	if (PlayerActor && !CameraShot.PlayerBindingTag.IsNone())
	{
		ActiveSequenceActor->SetBindingByTag(CameraShot.PlayerBindingTag, { PlayerActor }, false);
	}

	if (SourceActor && !CameraShot.SourceBindingTag.IsNone())
	{
		ActiveSequenceActor->SetBindingByTag(CameraShot.SourceBindingTag, { SourceActor }, false);
	}

	if (CustomActor && !CameraShot.CustomBindingTag.IsNone())
	{
		ActiveSequenceActor->SetBindingByTag(CameraShot.CustomBindingTag, { CustomActor }, false);
	}

	/*if (SourceActor && ActiveSequenceActor)
	{
		ActiveSequenceActor->bOverrideInstanceData = true;

		UDefaultLevelSequenceInstanceData* InstanceData =
			NewObject<UDefaultLevelSequenceInstanceData>(ActiveSequenceActor);

		if (InstanceData)
		{
			InstanceData->TransformOriginActor = SourceActor;
			ActiveSequenceActor->DefaultInstanceData = InstanceData;
		}
	}*/

	ActiveSequencePlayer->OnFinished.RemoveDynamic(this, &USFDialogueCameraComponent::HandleSequenceFinished);
	ActiveSequencePlayer->OnFinished.AddDynamic(this, &USFDialogueCameraComponent::HandleSequenceFinished);

	CachedPlayerController->SetViewTargetWithBlend(ActiveSequenceActor, CameraShot.BlendTime);
	ActiveSequencePlayer->Play();

	ActiveCameraMode = ESFActiveDialogueCameraMode::LevelSequence;
	bUsingDialogueCamera = true;
}

void USFDialogueCameraComponent::RestoreGameplayCamera()
{
	CachePlayerController();
	StopActiveSequence();

	if (const ASFPlayerCharacter* Player = Cast<ASFPlayerCharacter>(GetOwner()))
	{
		if (UCameraComponent* GameplayCam = Player->GetGameplayCamera())
		{
			GameplayCam->Activate();
		}
	}

	if (DialogueCamera)
	{
		DialogueCamera->Deactivate();
	}

	if (!CachedPlayerController)
	{
		ActiveCameraMode = ESFActiveDialogueCameraMode::None;
		bUsingDialogueCamera = false;
		return;
	}

	CachedPlayerController->SetViewTargetWithBlend(GetOwner(), 0.25f);

	ActiveCameraMode = ESFActiveDialogueCameraMode::None;
	bUsingDialogueCamera = false;
}