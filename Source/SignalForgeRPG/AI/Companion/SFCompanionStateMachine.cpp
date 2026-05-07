#include "AI/Companion/SFCompanionStateMachine.h"

#include "AI/Companion/SFCompanionStateBase.h"
#include "AI/Companion/SFCompanionStates.h"
#include "AI/SFCompanionAIController.h"
#include "Characters/SFCharacterBase.h"
#include "Companions/SFCompanionCharacter.h"
#include "Companions/SFCompanionTacticsComponent.h"

USFCompanionStateMachine::USFCompanionStateMachine()
{
}

void USFCompanionStateMachine::Initialize(ASFCompanionAIController* InController)
{
	Controller = InController;

	// Allocate one instance per state. Order in the array does NOT matter —
	// FindState() does a linear search by enum tag.
	States.Reset();
	States.Add(MakeUnique<FSFCompanionState_Idle>());
	States.Add(MakeUnique<FSFCompanionState_Follow>());
	States.Add(MakeUnique<FSFCompanionState_HoldPosition>());
	States.Add(MakeUnique<FSFCompanionState_MoveToLocation>());
	States.Add(MakeUnique<FSFCompanionState_AttackTarget>());
	States.Add(MakeUnique<FSFCompanionState_FocusFire>());
	States.Add(MakeUnique<FSFCompanionState_UseAbility>());
	States.Add(MakeUnique<FSFCompanionState_Combat>());
	States.Add(MakeUnique<FSFCompanionState_RetreatToPlayer>());
	States.Add(MakeUnique<FSFCompanionState_Stunned>());
	States.Add(MakeUnique<FSFCompanionState_Dead>());

	// Boot into Idle. The first Tick() will pick the right state via
	// EvaluateDesiredState() based on the current order/threshold context.
	ActiveState = FindState(ESFCompanionAIState::Idle);
	CurrentStateEnum = ESFCompanionAIState::Idle;
	if (ActiveState)
	{
		ActiveState->Enter(*this);
	}
}

void USFCompanionStateMachine::Shutdown()
{
	if (ActiveState)
	{
		ActiveState->Exit(*this);
	}
	ActiveState = nullptr;
	States.Reset();
	Controller.Reset();
}

void USFCompanionStateMachine::Tick(float DeltaSeconds)
{
	if (!Controller.IsValid()) { return; }

	// Death is sticky — once we're Dead, we stay there.
	if (CurrentStateEnum != ESFCompanionAIState::Dead)
	{
		const ESFCompanionAIState Desired = EvaluateDesiredState();
		if (Desired != CurrentStateEnum)
		{
			TransitionTo(Desired);
		}
	}

	if (ActiveState)
	{
		ActiveState->Tick(*this, DeltaSeconds);
	}
}

bool USFCompanionStateMachine::RequestState(ESFCompanionAIState NewState)
{
	if (NewState == CurrentStateEnum) { return true; }
	if (!FindState(NewState))         { return false; }

	TransitionTo(NewState);
	return true;
}

void USFCompanionStateMachine::OnOrderChanged(const FSFCompanionOrder& Order)
{
	// Re-issued orders (same type, new sequence) should still re-enter the
	// state so per-state Enter() picks up the fresh target/location.
	const ESFCompanionAIState Mapped = OrderTypeToState(Order.Type);

	if (Mapped == CurrentStateEnum)
	{
		// Force a re-entry: exit + re-enter rather than swap to a different state.
		if (ActiveState)
		{
			ActiveState->Exit(*this);
			ActiveState->Enter(*this);
		}
		return;
	}

	TransitionTo(Mapped);
}

void USFCompanionStateMachine::OnThresholdChanged()
{
	// Just re-evaluate; transition rules in EvaluateDesiredState() already
	// cover bSelfLowHealth / bPlayerLowHealth.
	const ESFCompanionAIState Desired = EvaluateDesiredState();
	if (Desired != CurrentStateEnum)
	{
		TransitionTo(Desired);
	}
}

ASFCompanionCharacter* USFCompanionStateMachine::GetCompanion() const
{
	return Controller.IsValid() ? Controller->GetControlledCompanion() : nullptr;
}

USFCompanionTacticsComponent* USFCompanionStateMachine::GetTactics() const
{
	if (ASFCompanionCharacter* Companion = GetCompanion())
	{
		return Companion->GetTactics();
	}
	return nullptr;
}

ESFCompanionAIState USFCompanionStateMachine::EvaluateDesiredState() const
{
	ASFCompanionCharacter* Companion = GetCompanion();
	if (!Companion) { return ESFCompanionAIState::Idle; }

	// 1. Death first.
	if (Companion->IsDead())
	{
		return ESFCompanionAIState::Dead;
	}

	USFCompanionTacticsComponent* Tactics = Companion->GetTactics();

	// 2. Active order outranks autonomous behavior.
	if (Tactics)
	{
		const FSFCompanionOrder& Order = Tactics->GetActiveOrder();
		if (Order.IsValidOrder())
		{
			return OrderTypeToState(Order.Type);
		}

		// 3. Threshold-driven retreat. Aggression Passive companions just
		//    sit; Defensive/Aggressive will fall back to the player.
		if (Tactics->IsSelfLowHealth() &&
			Tactics->GetAggression() != ESFCompanionAggression::Passive)
		{
			return ESFCompanionAIState::RetreatToPlayer;
		}
	}

	// 4. Default — Follow if there's a player to follow, else Idle.
	// (Combat will be selected by perception when it's wired up; for now
	//  we never auto-enter Combat without an order.)
	return ESFCompanionAIState::Follow;
}

ESFCompanionAIState USFCompanionStateMachine::OrderTypeToState(ESFCompanionOrderType OrderType)
{
	switch (OrderType)
	{
		case ESFCompanionOrderType::Follow:           return ESFCompanionAIState::Follow;
		case ESFCompanionOrderType::HoldPosition:     return ESFCompanionAIState::HoldPosition;
		case ESFCompanionOrderType::MoveToLocation:   return ESFCompanionAIState::MoveToLocation;
		case ESFCompanionOrderType::AttackTarget:     return ESFCompanionAIState::AttackTarget;
		case ESFCompanionOrderType::FocusFire:        return ESFCompanionAIState::FocusFire;
		case ESFCompanionOrderType::UseAbility:       return ESFCompanionAIState::UseAbility;
		case ESFCompanionOrderType::RetreatToPlayer:  return ESFCompanionAIState::RetreatToPlayer;
		case ESFCompanionOrderType::RevivePlayer:     return ESFCompanionAIState::Follow; // Reserved — not implemented yet.
		case ESFCompanionOrderType::None:
		default:                                      return ESFCompanionAIState::Idle;
	}
}

void USFCompanionStateMachine::TransitionTo(ESFCompanionAIState NewState)
{
	if (NewState == CurrentStateEnum) { return; }

	FSFCompanionStateBase* Next = FindState(NewState);
	if (!Next) { return; }

	if (ActiveState)
	{
		ActiveState->Exit(*this);
	}

	CurrentStateEnum = NewState;
	ActiveState = Next;
	ActiveState->Enter(*this);

	// Tell the controller so it can mirror onto its replicated property.
	if (Controller.IsValid())
	{
		Controller->NotifyAIStateChanged(CurrentStateEnum);
	}
}

FSFCompanionStateBase* USFCompanionStateMachine::FindState(ESFCompanionAIState State) const
{
	for (const TUniquePtr<FSFCompanionStateBase>& Ptr : States)
	{
		if (Ptr.IsValid() && Ptr->GetStateEnum() == State)
		{
			return Ptr.Get();
		}
	}
	return nullptr;
}
