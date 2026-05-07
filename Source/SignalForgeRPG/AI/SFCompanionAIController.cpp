#include "AI/SFCompanionAIController.h"

#include "AI/Companion/SFCompanionStateMachine.h"
#include "Companions/SFCompanionCharacter.h"
#include "Companions/SFCompanionTacticsComponent.h"
#include "Net/UnrealNetwork.h"

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
