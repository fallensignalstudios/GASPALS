#include "World/SFDarkZoneVolume.h"

#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

ASFDarkZoneVolume::ASFDarkZoneVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	ZoneBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ZoneBox"));
	RootComponent = ZoneBox;

	// A nice default size -- designers will resize per-volume in the editor.
	ZoneBox->SetBoxExtent(FVector(500.0f, 500.0f, 200.0f));

	// We don't need overlap events for the death-time check (it's a point-in-
	// box query), but leaving Pawn overlap on keeps in-editor visualization
	// useful and lets designers wire BP overlap hooks if they want them.
	ZoneBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ZoneBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	ZoneBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ZoneBox->SetGenerateOverlapEvents(true);

#if WITH_EDITORONLY_DATA
	// Translucent-ish editor visualization -- the default box wireframe is
	// hard to see in busy scenes.
	ZoneBox->ShapeColor = FColor(200, 40, 40, 255);
#endif
}

bool ASFDarkZoneVolume::ContainsLocation(const FVector& WorldLocation) const
{
	if (!ZoneBox)
	{
		return false;
	}

	// Transform world point into box-local space and test against half-extent.
	// This honors actor rotation/scale, unlike a naive AABB check.
	const FTransform BoxTransform = ZoneBox->GetComponentTransform();
	const FVector LocalPoint = BoxTransform.InverseTransformPosition(WorldLocation);
	const FVector HalfExtent = ZoneBox->GetUnscaledBoxExtent();

	return FMath::Abs(LocalPoint.X) <= HalfExtent.X
		&& FMath::Abs(LocalPoint.Y) <= HalfExtent.Y
		&& FMath::Abs(LocalPoint.Z) <= HalfExtent.Z;
}

bool ASFDarkZoneVolume::IsLocationInDarkZone(const UObject* WorldContextObject, FVector WorldLocation, ASFDarkZoneVolume*& OutZone)
{
	OutZone = nullptr;
	if (!WorldContextObject)
	{
		return false;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return false;
	}

	for (TActorIterator<ASFDarkZoneVolume> It(World); It; ++It)
	{
		ASFDarkZoneVolume* Volume = *It;
		if (Volume && Volume->ContainsLocation(WorldLocation))
		{
			OutZone = Volume;
			return true;
		}
	}

	return false;
}
