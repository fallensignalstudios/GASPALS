#include "Companions/SFCompanionCharacter.h"

#include "AI/SFCompanionAIController.h"
#include "Companions/SFCompanionTacticsComponent.h"

ASFCompanionCharacter::ASFCompanionCharacter()
{
	Tactics = CreateDefaultSubobject<USFCompanionTacticsComponent>(TEXT("Tactics"));

	AIControllerClass = ASFCompanionAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ASFCompanionCharacter::HandleDeath()
{
	if (bIsDead)
	{
		return;
	}

	// Companions inherit NPC death (writes Fact.NPC.Killed by ContextId).
	// On top of that, future passes can:
	//   - Mark the companion as Wounded rather than dead
	//   - Trigger a recovery quest
	//   - Lock them out of the active party
	// For now we rely on the NPC pipeline.
	Super::HandleDeath();
}
