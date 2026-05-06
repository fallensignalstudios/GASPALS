#include "Core/SFGameModeBase.h"
#include "Characters/SFPlayerCharacter.h"
#include "Core/SFGameStateBase.h"
#include "Core/SFPlayerState.h"
#include "Input/SFPlayerController.h"

ASFGameModeBase::ASFGameModeBase()
{
	DefaultPawnClass = ASFPlayerCharacter::StaticClass();
	PlayerControllerClass = ASFPlayerController::StaticClass();
	PlayerStateClass = ASFPlayerState::StaticClass();
	GameStateClass = ASFGameStateBase::StaticClass();
}