#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerBox.h"
#include "SFDarkZoneVolume.generated.h"

/**
 * A volume that flags an area as a "dark zone" -- death inside this volume
 * forces a restart from the most recently activated checkpoint, in the spirit
 * of Destiny's high-risk regions. Outside any dark zone, the player respawns
 * near the death location instead.
 *
 * Designers drop one or more of these into the level (overlapping is fine --
 * any overlap counts as dark). Set DarkZoneDisplayName to surface a friendly
 * name on the death screen.
 */
UCLASS()
class SIGNALFORGERPG_API ASFDarkZoneVolume : public ATriggerBox
{
	GENERATED_BODY()

public:
	ASFDarkZoneVolume();

	/** Display name surfaced on the death screen (e.g. "The Cosmodrome"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dark Zone")
	FText DarkZoneDisplayName;

	/**
	 * World-wide query: is the given location inside ANY dark zone volume?
	 * Returns the first matching volume in OutZone (may be null on false).
	 *
	 * Cheap-ish: iterates all ASFDarkZoneVolume actors in the world (there
	 * should be a small handful). If you ever scale to hundreds, swap this
	 * for a registry maintained in BeginPlay/EndPlay.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dark Zone", meta = (WorldContext = "WorldContextObject"))
	static bool IsLocationInDarkZone(const UObject* WorldContextObject, FVector WorldLocation, ASFDarkZoneVolume*& OutZone);
};
