#include "World/SFDarkZoneVolume.h"

#include "Components/BrushComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

ASFDarkZoneVolume::ASFDarkZoneVolume()
{
	// Trigger boxes default to bGenerateOverlapEvents = true on their brush
	// component, but we don't actually need overlap events -- the death-time
	// check is a point-in-volume query via EncompassesPoint. Leaving overlaps
	// on is harmless and keeps designer-side debugging easy.
	if (UBrushComponent* Brush = GetBrushComponent())
	{
		Brush->SetCollisionResponseToAllChannels(ECR_Ignore);
		Brush->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		Brush->SetGenerateOverlapEvents(true);
	}
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
		if (!Volume)
		{
			continue;
		}

		// EncompassesPoint runs an actual brush check, so we get the
		// designer-authored volume shape (boxes today, but works for any
		// brush) without ad-hoc bounds math.
		if (Volume->EncompassesPoint(WorldLocation))
		{
			OutZone = Volume;
			return true;
		}
	}

	return false;
}
