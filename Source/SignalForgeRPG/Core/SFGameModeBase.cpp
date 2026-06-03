#include "Core/SFGameModeBase.h"
#include "Characters/SFPlayerCharacter.h"
#include "Core/SFGameStateBase.h"
#include "Core/SFPlayerState.h"
#include "Input/SFPlayerController.h"
#include "World/SFCheckpoint.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

ASFGameModeBase::ASFGameModeBase()
{
	DefaultPawnClass = ASFPlayerCharacter::StaticClass();
	PlayerControllerClass = ASFPlayerController::StaticClass();
	PlayerStateClass = ASFPlayerState::StaticClass();
	GameStateClass = ASFGameStateBase::StaticClass();
}

void ASFGameModeBase::HandlePlayerRespawnRequest(APlayerController* PC, bool bRestartFromCheckpoint, FVector DeathLocation)
{
	if (!PC)
	{
		return;
	}

	FTransform SpawnTransform;

	if (bRestartFromCheckpoint)
	{
		if (ASFGameStateBase* GS = GetGameState<ASFGameStateBase>())
		{
			if (ASFCheckpoint* CP = GS->GetActiveCheckpoint())
			{
				SpawnTransform = CP->GetRespawnTransform();
			}
		}

		// No checkpoint yet recorded -- fall through to in-place behavior
		// rather than dropping the player at world origin. This keeps the
		// early game playable before the first checkpoint is touched.
		if (SpawnTransform.GetLocation().IsNearlyZero())
		{
			SpawnTransform.SetLocation(DeathLocation + FVector(InPlaceRespawnOffset, 0.0f, RespawnHeightLift));
		}
	}
	else
	{
		// In-place respawn: nudge laterally so the new capsule doesn't spawn
		// inside the dead ragdoll. The lift handles floor-clip cases.
		SpawnTransform.SetLocation(DeathLocation + FVector(InPlaceRespawnOffset, 0.0f, RespawnHeightLift));
	}

	// Tear down the dead pawn explicitly. RestartPlayer*() will spawn a
	// fresh one from DefaultPawnClass; relying on SetLifeSpan from
	// HandleDeath would race against the respawn.
	if (APawn* DeadPawn = PC->GetPawn())
	{
		PC->UnPossess();
		DeadPawn->Destroy();
	}

	RestartPlayerAtTransform(PC, SpawnTransform);
}