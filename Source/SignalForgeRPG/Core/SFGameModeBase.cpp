#include "Core/SFGameModeBase.h"
#include "Characters/SFPlayerCharacter.h"
#include "Input/SFPlayerController.h"

ASFGameModeBase::ASFGameModeBase()
{
	DefaultPawnClass = ASFPlayerCharacter::StaticClass();
	PlayerControllerClass = ASFPlayerController::StaticClass();
}