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
	/** Lateral offset applied to in-place respawns so the new pawn doesn't spawn on top of the ragdoll. */
	UPROPERTY(EditDefaultsOnly, Category = "Respawn")
	float InPlaceRespawnOffset = 200.0f;

	/** Z-up offset for any respawn to avoid floor clipping. */
	UPROPERTY(EditDefaultsOnly, Category = "Respawn")
	float RespawnHeightLift = 60.0f;
};