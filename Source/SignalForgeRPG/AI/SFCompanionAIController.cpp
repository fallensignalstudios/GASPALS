#include "AI/SFCompanionAIController.h"

#include "AI/Companion/SFCompanionStateMachine.h"
#include "Characters/SFCharacterBase.h"
#include "Characters/SFEnemyCharacter.h"
#include "Characters/SFNPCNarrativeIdentityComponent.h"
#include "Companions/SFCompanionCharacter.h"
#include "Companions/SFCompanionTacticsComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AIPerceptionComponent.h"

ASFCompanionAIController::ASFCompanionAIController()
{
	// FSM needs to tick every frame for poll-driven transitions / re-paths.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// IMPORTANT: leave DefaultBehaviorTree null. The parent ASFNPCAIController
	// only starts a BT when DefaultBehaviorTree is set, so leaving it unset
	// is enough to keep the BT layer dormant. We don't override OnPossess
	// just to skip BT init — we still want the perception bind from Super.
}

void ASFCompanionAIController::OnPossess(APawn* InPawn)
{
	// Super handles perception binding + (no-op) BT start because we
	// leave DefaultBehaviorTree null on this controller.
	Super::OnPossess(InPawn);

	ControlledCompanion = Cast<ASFCompanionCharacter>(InPawn);
	if (!ControlledCompanion)
	{
		return;
	}

	// Subscribe to tactics-driven events. The FSM stays passive on clients
	// — server alone owns the brain.
	if (USFCompanionTacticsComponent* Tactics = ControlledCompanion->GetTactics())
	{
		Tactics->OnOrderIssued.AddDynamic(this, &ASFCompanionAIController::HandleOrderIssued);
		Tactics->OnStanceChanged.AddDynamic(this, &ASFCompanionAIController::HandleStanceChanged);
		Tactics->OnThresholdCrossed.AddDynamic(this, &ASFCompanionAIController::HandleThresholdCrossed);
	}

	// Subscribe to AIPerception. The parent's HandlePerceptionUpdated is
	// kept bound (it's a no-op for companions since they're not
	// bIsHostileByDefault) — we just add our own handler alongside it.
	if (UAIPerceptionComponent* PerceptionComp = GetNPCPerceptionComponent())
	{
		if (!PerceptionComp->OnTargetPerceptionUpdated.IsAlreadyBound(this, &ASFCompanionAIController::HandleCompanionPerceptionUpdated))
		{
			PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &ASFCompanionAIController::HandleCompanionPerceptionUpdated);
		}
	}

	// Spin up the state machine on the server only.
	if (HasAuthority())
	{
		StateMachine = NewObject<USFCompanionStateMachine>(this, TEXT("CompanionStateMachine"));
		if (StateMachine)
		{
			StateMachine->Initialize(this);
		}
	}
}

void ASFCompanionAIController::OnUnPossess()
{
	if (ControlledCompanion)
	{
		if (USFCompanionTacticsComponent* Tactics = ControlledCompanion->GetTactics())
		{
			Tactics->OnOrderIssued.RemoveDynamic(this, &ASFCompanionAIController::HandleOrderIssued);
			Tactics->OnStanceChanged.RemoveDynamic(this, &ASFCompanionAIController::HandleStanceChanged);
			Tactics->OnThresholdCrossed.RemoveDynamic(this, &ASFCompanionAIController::HandleThresholdCrossed);
		}
	}

	if (UAIPerceptionComponent* PerceptionComp = GetNPCPerceptionComponent())
	{
		if (PerceptionComp->OnTargetPerceptionUpdated.IsAlreadyBound(this, &ASFCompanionAIController::HandleCompanionPerceptionUpdated))
		{
			PerceptionComp->OnTargetPerceptionUpdated.RemoveDynamic(this, &ASFCompanionAIController::HandleCompanionPerceptionUpdated);
		}
	}

	PerceivedHostiles.Reset();

	if (StateMachine)
	{
		StateMachine->Shutdown();
		StateMachine = nullptr;
	}

	ControlledCompanion = nullptr;

	Super::OnUnPossess();
}

void ASFCompanionAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority() && StateMachine)
	{
		StateMachine->Tick(DeltaSeconds);
	}
}

void ASFCompanionAIController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASFCompanionAIController, CurrentAIState);
}

void ASFCompanionAIController::NotifyAIStateChanged(ESFCompanionAIState NewState)
{
	if (HasAuthority())
	{
		// Mirror the FSM's internal state onto the replicated property.
		CurrentAIState = NewState;
	}
}

// =========================================================================
// Tactics delegate handlers
// =========================================================================
void ASFCompanionAIController::HandleStanceChanged(ESFCompanionStance /*OldStance*/, ESFCompanionStance /*NewStance*/)
{
	// Stance is a tactics-layer flavor knob; it doesn't directly drive the
	// FSM today. Hook in here later if a specific stance needs to force a
	// state change (e.g. Tank → forced Combat scan).
	if (HasAuthority() && StateMachine)
	{
		StateMachine->OnThresholdChanged();
	}
}

void ASFCompanionAIController::HandleOrderIssued(const FSFCompanionOrder& NewOrder)
{
	if (HasAuthority() && StateMachine)
	{
		StateMachine->OnOrderChanged(NewOrder);
	}
}

void ASFCompanionAIController::HandleThresholdCrossed(FGameplayTag /*ThresholdTag*/, bool /*bNowActive*/)
{
	if (HasAuthority() && StateMachine)
	{
		StateMachine->OnThresholdChanged();
	}
}

// =========================================================================
// Perception
// =========================================================================
void ASFCompanionAIController::HandleCompanionPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!HasAuthority() || !Actor)
	{
		return;
	}

	const bool bSensed = Stimulus.WasSuccessfullySensed();
	const bool bHostile = IsActorHostileToCompanion(Actor, ControlledCompanion);

	if (bSensed && bHostile)
	{
		PerceivedHostiles.AddUnique(Actor);
	}
	else
	{
		// Lost sight or actor isn't hostile (anymore) — drop it.
		PerceivedHostiles.RemoveAll([Actor](const TWeakObjectPtr<AActor>& Entry)
		{
			return Entry.Get() == Actor;
		});
	}

	// Event-driven engagement: poke the FSM immediately rather than waiting
	// for the next tick. OnThresholdChanged re-runs EvaluateDesiredState.
	if (StateMachine)
	{
		StateMachine->OnThresholdChanged();
	}
}

bool ASFCompanionAIController::HasPerceivedHostile() const
{
	for (const TWeakObjectPtr<AActor>& Entry : PerceivedHostiles)
	{
		if (IsActorHostileToCompanion(Entry.Get(), ControlledCompanion))
		{
			return true;
		}
	}
	return false;
}

AActor* ASFCompanionAIController::GetClosestPerceivedHostile() const
{
	if (!ControlledCompanion) { return nullptr; }

	const FVector Origin = ControlledCompanion->GetActorLocation();

	AActor* Best = nullptr;
	float   BestDistSq = TNumericLimits<float>::Max();

	for (const TWeakObjectPtr<AActor>& Entry : PerceivedHostiles)
	{
		AActor* Actor = Entry.Get();
		if (!IsActorHostileToCompanion(Actor, ControlledCompanion))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(Origin, Actor->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Actor;
		}
	}

	return Best;
}

bool ASFCompanionAIController::IsActorHostileToCompanion(const AActor* Candidate, const ASFCompanionCharacter* Self)
{
	if (!IsValid(Candidate))                  { return false; }
	if (Candidate == Self)                    { return false; }

	// Never target the local player pawn — companions are player-aligned.
	if (const UWorld* World = Candidate->GetWorld())
	{
		if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0))
		{
			if (Candidate == PlayerPawn) { return false; }
		}
	}

	// Don't engage corpses.
	if (const ASFCharacterBase* AsChar = Cast<ASFCharacterBase>(Candidate))
	{
		if (AsChar->IsDead()) { return false; }
	}

	// Definitive hostile: anything authored as ASFEnemyCharacter.
	if (Candidate->IsA<ASFEnemyCharacter>())
	{
		return true;
	}

	// Disposition-driven hostile: NPC whose narrative identity says Hostile.
	if (const USFNPCNarrativeIdentityComponent* Identity =
			Candidate->FindComponentByClass<USFNPCNarrativeIdentityComponent>())
	{
		return Identity->GetDisposition() == ESFNPCDisposition::Hostile;
	}

	return false;
}
