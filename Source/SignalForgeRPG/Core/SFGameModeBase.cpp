#include "Core/SFGameModeBase.h"
#include "Characters/SFPlayerCharacter.h"
#include "Core/SFGameStateBase.h"
#include "Core/SFPlayerState.h"
#include "Input/SFPlayerController.h"
#include "World/SFCheckpoint.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "NavigationSystem.h"
#include "NavigationData.h"

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
	bool bResolved = false;

	if (bRestartFromCheckpoint)
	{
		if (ASFGameStateBase* GS = GetGameState<ASFGameStateBase>())
		{
			if (ASFCheckpoint* CP = GS->GetActiveCheckpoint())
			{
				SpawnTransform = CP->GetRespawnTransform();
				bResolved = true;
			}
		}
		// If no checkpoint has been touched yet, fall through to the in-place
		// projection path below so the early game stays playable.
	}

	if (!bResolved)
	{
		// Destiny-style "pick yourself up nearby" -- find the closest
		// navmesh-projected point to the death location, so respawn always
		// lands on walkable geometry instead of inside a wall, off a ledge,
		// or on top of the ragdoll.
		UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
		FVector ChosenLocation = FVector::ZeroVector;
		bool bProjected = false;

		if (NavSys)
		{
			const FVector Extent(InPlaceRespawnSearchRadius, InPlaceRespawnSearchRadius, InPlaceRespawnSearchRadius * 0.5f);

			FNavLocation NavPt;
			if (NavSys->ProjectPointToNavigation(DeathLocation, NavPt, Extent))
			{
				ChosenLocation = NavPt.Location;
				bProjected = true;

				// If projection snapped right on top of the ragdoll, push out
				// to a random point in the search annulus and re-project. We
				// try a handful of directions before giving up on the
				// minimum-distance constraint.
				const float MinDistSq = MinDistanceFromDeathLocation * MinDistanceFromDeathLocation;
				if (FVector::DistSquared2D(ChosenLocation, DeathLocation) < MinDistSq)
				{
					for (int32 Attempt = 0; Attempt < 8; ++Attempt)
					{
						const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
						const FVector Probe = DeathLocation + FVector(
							FMath::Cos(Angle) * MinDistanceFromDeathLocation * 1.5f,
							FMath::Sin(Angle) * MinDistanceFromDeathLocation * 1.5f,
							0.0f);

						FNavLocation Retry;
						if (NavSys->ProjectPointToNavigation(Probe, Retry, Extent)
							&& FVector::DistSquared2D(Retry.Location, DeathLocation) >= MinDistSq)
						{
							ChosenLocation = Retry.Location;
							break;
						}
					}
				}
			}
		}

		if (!bProjected)
		{
			// Navmesh not present (untracked level / preview map / dev sandbox)
			// or no walkable surface within the search radius -- last-resort
			// lateral nudge so we still respawn somewhere reasonable rather
			// than dropping to world origin.
			ChosenLocation = DeathLocation + FVector(FallbackLateralOffset, 0.0f, 0.0f);
		}

		ChosenLocation.Z += RespawnHeightLift;
		SpawnTransform.SetLocation(ChosenLocation);

		// Face the camera-forward direction (approximated as the death-time
		// view) so the respawned pawn isn't looking at a wall.
		FRotator ControlRot = PC->GetControlRotation();
		if (!ControlRot.IsZero())
		{
			ControlRot.Pitch = 0.0f;
			ControlRot.Roll = 0.0f;
			SpawnTransform.SetRotation(ControlRot.Quaternion());
		}
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