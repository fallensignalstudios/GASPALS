#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SFGameModeBase.generated.h"

class APlayerController;

UCLASS()
class SIGNALFORGERPG_API ASFGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASFGameModeBase();

	/**
	 * Called by the player controller after the death screen finishes. If
	 * bRestartFromCheckpoint is true, respawns at the game state's active
	 * checkpoint (used for dark-zone deaths). Otherwise respawns near the
	 * supplied death location, Destiny-style "pick yourself up."
	 */
	void HandlePlayerRespawnRequest(APlayerController* PC, bool bRestartFromCheckpoint, FVector DeathLocation);

protected:
	/**
	 * Radius (cm) searched around the death location for a navmesh-projected
	 * safe respawn point. Destiny-style "pick yourself up nearby" lands
	 * inside this radius; smaller = closer to where you died, larger = more
	 * forgiving when you die on a ledge or in cluttered geometry.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Respawn")
	float InPlaceRespawnSearchRadius = 600.0f;

	/**
	 * Minimum lateral distance the projected respawn point must be from the
	 * death location so the new capsule doesn't spawn on top of the ragdoll.
	 * If projection lands closer than this, we re-roll with a forced offset.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Respawn")
	float MinDistanceFromDeathLocation = 200.0f;

	/** Z-up offset applied to any respawn point to avoid floor clipping. */
	UPROPERTY(EditDefaultsOnly, Category = "Respawn")
	float RespawnHeightLift = 60.0f;

	/** Used only as the absolute last resort if no navmesh point can be found. */
	UPROPERTY(EditDefaultsOnly, Category = "Respawn")
	float FallbackLateralOffset = 200.0f;
};