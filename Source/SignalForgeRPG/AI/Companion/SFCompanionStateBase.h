#pragma once

#include "CoreMinimal.h"
#include "AI/Companion/SFCompanionAIState.h"

class USFCompanionStateMachine;
class ASFCompanionAIController;
class ASFCompanionCharacter;
class USFCompanionTacticsComponent;

/**
 * Abstract base class for a single companion AI state. Plain C++ — not a
 * UObject. The state machine owns these via TUniquePtr.
 *
 * Lifetime contract:
 *   Enter(Machine)   — called once when the state becomes active.
 *   Tick(Machine, Dt)— called every controller tick while active.
 *   Exit(Machine)    — called once when the state is being replaced.
 *
 * States read/write the world through the controller, the controlled
 * companion, and the tactics component. They never touch the blackboard.
 */
class SIGNALFORGERPG_API FSFCompanionStateBase
{
public:
	virtual ~FSFCompanionStateBase() = default;

	/** Enum tag used for replication and AnimBP consumption. */
	virtual ESFCompanionAIState GetStateEnum() const = 0;

	virtual void Enter(USFCompanionStateMachine& Machine) {}
	virtual void Tick(USFCompanionStateMachine& Machine, float DeltaSeconds) {}
	virtual void Exit(USFCompanionStateMachine& Machine) {}

	/** Optional human-readable name for logs/debug HUDs. */
	virtual FString GetStateName() const { return TEXT("State"); }

protected:
	// -------------------------------------------------------------------------
	// Convenience accessors used by concrete states. Implemented inline against
	// the machine pointer to keep this header self-contained.
	// -------------------------------------------------------------------------

	ASFCompanionAIController*    GetController(USFCompanionStateMachine& Machine) const;
	ASFCompanionCharacter*       GetCompanion(USFCompanionStateMachine& Machine) const;
	USFCompanionTacticsComponent* GetTactics(USFCompanionStateMachine& Machine) const;
};
