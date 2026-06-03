#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SFDarkZoneVolume.generated.h"

class UBoxComponent;

/**
 * A volume that flags an area as a "dark zone" -- death inside this volume
 * forces a restart from the most recently activated checkpoint, in the spirit
 * of Destiny's high-risk regions. Outside any dark zone, the player respawns
 * near the death location instead.
 *
 * Designers drop one or more of these into the level (overlapping is fine --
 * any overlap counts as dark). Set DarkZoneDisplayName to surface a friendly
 * name on the death screen. Resize via the BoxExtent on the root box.
 *
 * Implementation note: this used to inherit from ATriggerBox, but the brush
 * include chain was fragile under IncludeOrderVersion=Unreal5_4. A plain
 * UBoxComponent is friendlier to designers anyway (scale handles in editor)
 * and avoids the brush API entirely.
 */
UCLASS()
class SIGNALFORGERPG_API ASFDarkZoneVolume : public AActor
{
	GENERATED_BODY()

public:
	ASFDarkZoneVolume();

	/** Display name surfaced on the death screen (e.g. "The Cosmodrome"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dark Zone")
	FText DarkZoneDisplayName;

	/** Root box -- defines the dark zone's volume. Resize in editor. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dark Zone")
	TObjectPtr<UBoxComponent> ZoneBox;

	/**
	 * Returns true if WorldLocation is inside this dark zone's box.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dark Zone")
	bool ContainsLocation(const FVector& WorldLocation) const;

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
