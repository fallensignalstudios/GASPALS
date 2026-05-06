#include "UI/SFMenuPreviewCharacter.h"

#include "Components/SFEquipmentComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

ASFMenuPreviewCharacter::ASFMenuPreviewCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bUseControllerRotationYaw = false;

	PreviewEquipmentComponent = CreateDefaultSubobject<USFEquipmentComponent>(TEXT("PreviewEquipmentComponent"));
}

void ASFMenuPreviewCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ASFMenuPreviewCharacter::SyncFromSourceCharacter(ACharacter* SourceCharacter)
{
	if (!SourceCharacter)
	{
		return;
	}

	USkeletalMeshComponent* SourceMesh = SourceCharacter->GetMesh();
	USkeletalMeshComponent* PreviewMesh = GetMesh();


	if (!SourceMesh || !PreviewMesh)
	{
		return;
	}

	PreviewMesh->SetSkeletalMesh(SourceMesh->GetSkeletalMeshAsset());
	PreviewMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	PreviewMesh->SetAnimInstanceClass(SourceMesh->GetAnimClass());

	for (int32 MaterialIndex = 0; MaterialIndex < SourceMesh->GetNumMaterials(); ++MaterialIndex)
	{
		PreviewMesh->SetMaterial(MaterialIndex, SourceMesh->GetMaterial(MaterialIndex));
	}

	SyncEquipmentFromSourceCharacter(SourceCharacter);

	BP_OnPreviewSynced();
}

void ASFMenuPreviewCharacter::SyncEquipmentFromSourceCharacter(ACharacter* SourceCharacter)
{
	if (!SourceCharacter || !PreviewEquipmentComponent)
	{
		return;
	}

	USFEquipmentComponent* SourceEquipmentComponent = SourceCharacter->FindComponentByClass<USFEquipmentComponent>();
	if (!SourceEquipmentComponent)
	{
		PreviewEquipmentComponent->ClearEquippedWeapon();
		return;
	}

	USFWeaponData* SourceWeaponData = SourceEquipmentComponent->GetCurrentWeaponData();
	if (!SourceWeaponData)
	{
		PreviewEquipmentComponent->ClearEquippedWeapon();
		return;
	}

	PreviewEquipmentComponent->EquipWeaponData(SourceWeaponData);
}

void ASFMenuPreviewCharacter::SetPreviewRotation(float YawDegrees)
{
	SetActorRotation(FRotator(0.f, YawDegrees, 0.f));
}