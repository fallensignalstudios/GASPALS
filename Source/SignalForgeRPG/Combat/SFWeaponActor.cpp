#include "Combat/SFWeaponActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Combat/SFWeaponData.h"

ASFWeaponActor::ASFWeaponActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(RootSceneComponent);

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	StaticMeshComponent->SetupAttachment(RootSceneComponent);
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaticMeshComponent->SetGenerateOverlapEvents(false);
	StaticMeshComponent->SetHiddenInGame(true);

	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
	SkeletalMeshComponent->SetupAttachment(RootSceneComponent);
	SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMeshComponent->SetGenerateOverlapEvents(false);
	SkeletalMeshComponent->SetHiddenInGame(true);
}

void ASFWeaponActor::InitializeFromWeaponData(USFWeaponData* InWeaponData)
{
	WeaponInstanceData = FSFWeaponInstanceData{};
	WeaponData = InWeaponData;
	ApplyWeaponVisuals();
}

void ASFWeaponActor::InitializeFromWeaponInstance(const FSFWeaponInstanceData& InInstanceData)
{
	WeaponInstanceData = InInstanceData;
	WeaponData = InInstanceData.WeaponDefinition.LoadSynchronous();
	ApplyWeaponVisuals();
}

void ASFWeaponActor::ApplyOffhandMeshOverride(USFWeaponData* InWeaponData)
{
	if (!InWeaponData)
	{
		return;
	}

	// If the data has no offhand-specific mesh, keep the mainhand mesh (symmetric pair).
	const bool bHasSkeletalOverride = (InWeaponData->OffhandSkeletalWeaponMesh != nullptr);
	const bool bHasStaticOverride = (InWeaponData->OffhandStaticWeaponMesh != nullptr);
	if (!bHasSkeletalOverride && !bHasStaticOverride)
	{
		return;
	}

	// Pick the offhand transform if the designer set one; otherwise fall through to the
	// mainhand RelativeAttachTransform so the offhand mesh attaches identically.
	const FTransform& OffhandXform = InWeaponData->OffhandMeshRelativeTransform.Equals(FTransform::Identity)
		? InWeaponData->RelativeAttachTransform
		: InWeaponData->OffhandMeshRelativeTransform;

	ClearWeaponVisuals();

	if (bHasSkeletalOverride && SkeletalMeshComponent)
	{
		SkeletalMeshComponent->SetSkeletalMesh(InWeaponData->OffhandSkeletalWeaponMesh);
		SkeletalMeshComponent->SetRelativeTransform(OffhandXform);
		SkeletalMeshComponent->SetHiddenInGame(false);
		return;
	}

	if (bHasStaticOverride && StaticMeshComponent)
	{
		StaticMeshComponent->SetStaticMesh(InWeaponData->OffhandStaticWeaponMesh);
		StaticMeshComponent->SetRelativeTransform(OffhandXform);
		StaticMeshComponent->SetHiddenInGame(false);
	}
}

void ASFWeaponActor::ClearWeaponVisuals()
{
	if (StaticMeshComponent)
	{
		StaticMeshComponent->SetStaticMesh(nullptr);
		StaticMeshComponent->SetHiddenInGame(true);
		StaticMeshComponent->SetRelativeTransform(FTransform::Identity);
	}

	if (SkeletalMeshComponent)
	{
		SkeletalMeshComponent->SetSkeletalMesh(nullptr);
		SkeletalMeshComponent->SetHiddenInGame(true);
		SkeletalMeshComponent->SetRelativeTransform(FTransform::Identity);
	}
}

void ASFWeaponActor::ApplyWeaponVisuals()
{
	ClearWeaponVisuals();

	if (!WeaponData)
	{
		return;
	}

	if (WeaponData->SkeletalWeaponMesh)
	{
		SkeletalMeshComponent->SetSkeletalMesh(WeaponData->SkeletalWeaponMesh);
		SkeletalMeshComponent->SetRelativeTransform(WeaponData->RelativeAttachTransform);
		SkeletalMeshComponent->SetHiddenInGame(false);
		return;
	}

	if (WeaponData->StaticWeaponMesh)
	{
		StaticMeshComponent->SetStaticMesh(WeaponData->StaticWeaponMesh);
		StaticMeshComponent->SetRelativeTransform(WeaponData->RelativeAttachTransform);
		StaticMeshComponent->SetHiddenInGame(false);
	}
}

bool ASFWeaponActor::HasValidTraceSockets() const
{
	if (SkeletalMeshComponent &&
		SkeletalMeshComponent->DoesSocketExist(TraceStartSocketName) &&
		SkeletalMeshComponent->DoesSocketExist(TraceEndSocketName))
	{
		return true;
	}

	if (StaticMeshComponent &&
		StaticMeshComponent->DoesSocketExist(TraceStartSocketName) &&
		StaticMeshComponent->DoesSocketExist(TraceEndSocketName))
	{
		return true;
	}

	return false;
}

FVector ASFWeaponActor::GetTraceStartLocation() const
{
	if (SkeletalMeshComponent && SkeletalMeshComponent->DoesSocketExist(TraceStartSocketName))
	{
		return SkeletalMeshComponent->GetSocketLocation(TraceStartSocketName);
	}

	if (StaticMeshComponent && StaticMeshComponent->DoesSocketExist(TraceStartSocketName))
	{
		return StaticMeshComponent->GetSocketLocation(TraceStartSocketName);
	}

	return GetActorLocation();
}

FVector ASFWeaponActor::GetTraceEndLocation() const
{
	if (SkeletalMeshComponent && SkeletalMeshComponent->DoesSocketExist(TraceEndSocketName))
	{
		return SkeletalMeshComponent->GetSocketLocation(TraceEndSocketName);
	}

	if (StaticMeshComponent && StaticMeshComponent->DoesSocketExist(TraceEndSocketName))
	{
		return StaticMeshComponent->GetSocketLocation(TraceEndSocketName);
	}

	return GetActorLocation();
}

bool ASFWeaponActor::HasValidMuzzleSocket() const
{
	if (SkeletalMeshComponent && SkeletalMeshComponent->DoesSocketExist(MuzzleSocketName))
	{
		return true;
	}

	if (StaticMeshComponent && StaticMeshComponent->DoesSocketExist(MuzzleSocketName))
	{
		return true;
	}

	return false;
}

FVector ASFWeaponActor::GetMuzzleLocation() const
{
	if (SkeletalMeshComponent && SkeletalMeshComponent->DoesSocketExist(MuzzleSocketName))
	{
		return SkeletalMeshComponent->GetSocketLocation(MuzzleSocketName);
	}

	if (StaticMeshComponent && StaticMeshComponent->DoesSocketExist(MuzzleSocketName))
	{
		return StaticMeshComponent->GetSocketLocation(MuzzleSocketName);
	}

	return GetActorLocation();
}

FRotator ASFWeaponActor::GetMuzzleRotation() const
{
	if (SkeletalMeshComponent && SkeletalMeshComponent->DoesSocketExist(MuzzleSocketName))
	{
		return SkeletalMeshComponent->GetSocketRotation(MuzzleSocketName);
	}

	if (StaticMeshComponent && StaticMeshComponent->DoesSocketExist(MuzzleSocketName))
	{
		return StaticMeshComponent->GetSocketRotation(MuzzleSocketName);
	}

	return GetActorRotation();
}

bool ASFWeaponActor::HasValidMuzzleFXSocket() const
{
	if (SkeletalMeshComponent && SkeletalMeshComponent->DoesSocketExist(MuzzleFXSocketName))
	{
		return true;
	}

	if (StaticMeshComponent && StaticMeshComponent->DoesSocketExist(MuzzleFXSocketName))
	{
		return true;
	}

	return false;
}

UPrimitiveComponent* ASFWeaponActor::GetActiveVisualComponent() const
{
	if (SkeletalMeshComponent && !SkeletalMeshComponent->bHiddenInGame && SkeletalMeshComponent->GetSkeletalMeshAsset())
	{
		return SkeletalMeshComponent;
	}

	if (StaticMeshComponent && !StaticMeshComponent->bHiddenInGame && StaticMeshComponent->GetStaticMesh())
	{
		return StaticMeshComponent;
	}

	return nullptr;
}