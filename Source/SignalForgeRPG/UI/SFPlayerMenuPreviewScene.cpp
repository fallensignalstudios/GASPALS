#include "UI/SFPlayerMenuPreviewScene.h"
#include "UI/SFMenuPreviewCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

ASFPlayerMenuPreviewScene::ASFPlayerMenuPreviewScene()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	PreviewCharacterAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("PreviewCharacterAnchor"));
	PreviewCharacterAnchor->SetupAttachment(Root);

	PreviewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PreviewCamera"));
	PreviewCamera->SetupAttachment(Root);
}

ASFMenuPreviewCharacter* ASFPlayerMenuPreviewScene::GetOrSpawnPreviewCharacter()
{
	if (SpawnedPreviewCharacter)
	{
		return SpawnedPreviewCharacter;
	}

	if (!PreviewCharacterClass)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	SpawnedPreviewCharacter = World->SpawnActor<ASFMenuPreviewCharacter>(
		PreviewCharacterClass,
		PreviewCharacterAnchor->GetComponentTransform(),
		SpawnParams);

	return SpawnedPreviewCharacter;
}