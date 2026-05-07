#pragma once

#include "CoreMinimal.h"
#include "AI/Companion/SFCompanionStateBase.h"

/**
 * Concrete companion FSM states. Plain C++ — no UObject, no reflection.
 * Each state is a thin policy class that reads tactics + world data and
 * issues controller-level commands (MoveTo, StopMovement, ability fire).
 *
 * Persistent vs consumed orders:
 *   - HoldPosition, FocusFire — state must NOT call ClearOrder.
 *   - Follow, MoveToLocation, AttackTarget, UseAbility, RetreatToPlayer —
 *     state calls Tactics->ClearOrder() on success (authority-only).
 */

class FSFCompanionState_Idle : public FSFCompanionStateBase
{
public:
	virtual ESFCompanionAIState GetStateEnum() const override { return ESFCompanionAIState::Idle; }
	virtual FString             GetStateName() const override { return TEXT("Idle"); }

	virtual void Enter(USFCompanionStateMachine& Machine) override;
	virtual void Tick(USFCompanionStateMachine& Machine, float DeltaSeconds) override;
};

class FSFCompanionState_Follow : public FSFCompanionStateBase
{
public:
	virtual ESFCompanionAIState GetStateEnum() const override { return ESFCompanionAIState::Follow; }
	virtual FString             GetStateName() const override { return TEXT("Follow"); }

	virtual void Enter(USFCompanionStateMachine& Machine) override;
	virtual void Tick(USFCompanionStateMachine& Machine, float DeltaSeconds) override;
	virtual void Exit(USFCompanionStateMachine& Machine) override;

private:
	float RepathTimer = 0.f;
	FVector LastGoal = FVector::ZeroVector;
	bool bHasLastGoal = false;
};

class FSFCompanionState_HoldPosition : public FSFCompanionStateBase
{
public:
	virtual ESFCompanionAIState GetStateEnum() const override { return ESFCompanionAIState::HoldPosition; }
	virtual FString             GetStateName() const override { return TEXT("HoldPosition"); }

	virtual void Enter(USFCompanionStateMachine& Machine) override;
	virtual void Tick(USFCompanionStateMachine& Machine, float DeltaSeconds) override;
	virtual void Exit(USFCompanionStateMachine& Machine) override;

private:
	FVector HoldAnchor = FVector::ZeroVector;
	bool    bAnchorValid = false;
};

class FSFCompanionState_MoveToLocation : public FSFCompanionStateBase
{
public:
	virtual ESFCompanionAIState GetStateEnum() const override { return ESFCompanionAIState::MoveToLocation; }
	virtual FString             GetStateName() const override { return TEXT("MoveToLocation"); }

	virtual void Enter(USFCompanionStateMachine& Machine) override;
	virtual void Tick(USFCompanionStateMachine& Machine, float DeltaSeconds) override;
	virtual void Exit(USFCompanionStateMachine& Machine) override;

private:
	FVector Destination = FVector::ZeroVector;
	int32   OrderSequence = 0;
};

class FSFCompanionState_AttackTarget : public FSFCompanionStateBase
{
public:
	virtual ESFCompanionAIState GetStateEnum() const override { return ESFCompanionAIState::AttackTarget; }
	virtual FString             GetStateName() const override { return TEXT("AttackTarget"); }

	virtual void Enter(USFCompanionStateMachine& Machine) override;
	virtual void Tick(USFCompanionStateMachine& Machine, float DeltaSeconds) override;
	virtual void Exit(USFCompanionStateMachine& Machine) override;

private:
	TWeakObjectPtr<AActor> Target;
	float AttackTimer = 0.f;
	int32 OrderSequence = 0;
};

class FSFCompanionState_FocusFire : public FSFCompanionStateBase
{
public:
	virtual ESFCompanionAIState GetStateEnum() const override { return ESFCompanionAIState::FocusFire; }
	virtual FString             GetStateName() const override { return TEXT("FocusFire"); }

	virtual void Enter(USFCompanionStateMachine& Machine) override;
	virtual void Tick(USFCompanionStateMachine& Machine, float DeltaSeconds) override;
	virtual void Exit(USFCompanionStateMachine& Machine) override;

private:
	TWeakObjectPtr<AActor> Target;
	float AttackTimer = 0.f;
};

class FSFCompanionState_UseAbility : public FSFCompanionStateBase
{
public:
	virtual ESFCompanionAIState GetStateEnum() const override { return ESFCompanionAIState::UseAbility; }
	virtual FString             GetStateName() const override { return TEXT("UseAbility"); }

	virtual void Enter(USFCompanionStateMachine& Machine) override;
	virtual void Tick(USFCompanionStateMachine& Machine, float DeltaSeconds) override;
};

class FSFCompanionState_Combat : public FSFCompanionStateBase
{
public:
	virtual ESFCompanionAIState GetStateEnum() const override { return ESFCompanionAIState::Combat; }
	virtual FString             GetStateName() const override { return TEXT("Combat"); }

	virtual void Enter(USFCompanionStateMachine& Machine) override;
	virtual void Tick(USFCompanionStateMachine& Machine, float DeltaSeconds) override;
	virtual void Exit(USFCompanionStateMachine& Machine) override;

private:
	TWeakObjectPtr<AActor> CurrentTarget;
	float TargetScanTimer = 0.f;
	float AttackTimer = 0.f;
};

class FSFCompanionState_RetreatToPlayer : public FSFCompanionStateBase
{
public:
	virtual ESFCompanionAIState GetStateEnum() const override { return ESFCompanionAIState::RetreatToPlayer; }
	virtual FString             GetStateName() const override { return TEXT("RetreatToPlayer"); }

	virtual void Enter(USFCompanionStateMachine& Machine) override;
	virtual void Tick(USFCompanionStateMachine& Machine, float DeltaSeconds) override;
	virtual void Exit(USFCompanionStateMachine& Machine) override;

private:
	float RepathTimer = 0.f;
};

class FSFCompanionState_Stunned : public FSFCompanionStateBase
{
public:
	virtual ESFCompanionAIState GetStateEnum() const override { return ESFCompanionAIState::Stunned; }
	virtual FString             GetStateName() const override { return TEXT("Stunned"); }

	virtual void Enter(USFCompanionStateMachine& Machine) override;
	virtual void Exit(USFCompanionStateMachine& Machine) override;
};

class FSFCompanionState_Dead : public FSFCompanionStateBase
{
public:
	virtual ESFCompanionAIState GetStateEnum() const override { return ESFCompanionAIState::Dead; }
	virtual FString             GetStateName() const override { return TEXT("Dead"); }

	virtual void Enter(USFCompanionStateMachine& Machine) override;
};
