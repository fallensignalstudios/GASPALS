#include "Characters/SFEnemyCharacter.h"

#include "AI/SFEnemyAIController.h"
#include "Core/SignalForgeGameplayTags.h"
#include "Faction/SFFactionComponent.h"

ASFEnemyCharacter::ASFEnemyCharacter()
{
	AIControllerClass = ASFEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ASFEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Default enemy faction to Faction.Bandit if BP didn't set anything.
	if (HasAuthority())
	{
		if (USFFactionComponent* Faction = GetFactionComponent())
		{
			if (!Faction->GetFactionTag().IsValid())
			{
				Faction->SetFactionTag(FSignalForgeGameplayTags::Get().Faction_Bandit);
			}
		}
	}
}
