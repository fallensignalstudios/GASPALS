#include "Characters/SFEnemyCharacter.h"
#include "AI/SFEnemyAIController.h"
#include "Components/SFProgressionComponent.h"

ASFEnemyCharacter::ASFEnemyCharacter()
{
	AIControllerClass = ASFEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ASFEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ASFEnemyCharacter::SetLastDamagingCharacter(ASFCharacterBase* InCharacter)
{
	LastDamagingCharacter = InCharacter;
}

int32 ASFEnemyCharacter::GetXPReward() const
{
	if (const USFProgressionComponent* Progression = GetProgressionComponent())
	{
		return Progression->GetEnemyXPRewardForCurrentLevel();
	}

	return 0;
}

void ASFEnemyCharacter::HandleDeath()
{
	if (bIsDead)
	{
		return;
	}

	Super::HandleDeath();

	if (!LastDamagingCharacter)
	{
		return;
	}

	if (USFProgressionComponent* Progression = LastDamagingCharacter->GetProgressionComponent())
	{
		const int32 XPReward = GetXPReward();
		if (XPReward > 0)
		{
			Progression->AddXP(XPReward);
		}
	}
}