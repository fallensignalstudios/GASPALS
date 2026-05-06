#include "Companions/SFCompanionCharacter.h"

#include "AI/SFCompanionAIController.h"
#include "Companions/SFCompanionBarkComponent.h"
#include "Companions/SFCompanionTacticsComponent.h"

ASFCompanionCharacter::ASFCompanionCharacter()
{
	Tactics = CreateDefaultSubobject<USFCompanionTacticsComponent>(TEXT("Tactics"));
	Barks   = CreateDefaultSubobject<USFCompanionBarkComponent>(TEXT("Barks"));

	AIControllerClass = ASFCompanionAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ASFCompanionCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Forward our own health changes into the tactics threshold gate so
	// the BT (or any listener) sees Companion.Threat.SelfLowHealth flip
	// when we cross the configured percentage.
	OnHealthChanged.AddDynamic(this, &ASFCompanionCharacter::HandleSelfHealthChanged);
}

void ASFCompanionCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	OnHealthChanged.RemoveDynamic(this, &ASFCompanionCharacter::HandleSelfHealthChanged);
	Super::EndPlay(EndPlayReason);
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

void ASFCompanionCharacter::HandleSelfHealthChanged(float NewValue, float MaxValue)
{
	if (!Tactics) return;

	const float SelfPct = (MaxValue > 0.f) ? (NewValue / MaxValue) : -1.f;

	// PlayerPct is left as -1 ("no info"); a Healer-stance subtree should
	// query the player's attribute set directly when the Companion.Stance.Healer
	// tag is active. This keeps the threshold evaluator clean and avoids
	// per-frame polling of foreign attribute sets here.
	Tactics->EvaluateHealthThresholds(SelfPct, /*PlayerPct=*/ -1.f);
}

void ASFCompanionCharacter::PushHealthThresholdsToTactics()
{
	// Reserved for future use (manual re-evaluation).
}
