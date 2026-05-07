#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AI/Companion/SFCompanionAIState.h"
#include "Companions/SFCompanionTypes.h"
#include "SFCompanionStateMachine.generated.h"

class FSFCompanionStateBase;
class ASFCompanionAIController;
class ASFCompanionCharacter;
class USFCompanionTacticsComponent;

/**
 * USFCompanionStateMachine
 *
 * Server-authoritative finite state machine that drives a companion's
 * high-level AI. Owned by ASFCompanionAIController as a UPROPERTY so
 * Unreal's GC keeps it alive.
 *
 * Hybrid event/poll model:
 *   - Tick() is called every frame and re-evaluates transition rules
 *     (poll path: handles target acquisition, distance checks, death).
 *   - RequestState() is called from the controller's delegate handlers
 *     when an external event needs to force a transition immediately
 *     (event path: orders, stance changes, threshold crosses).
 *
 * State storage is plain C++ via TUniquePtr — states are not UObjects.
 * The set of states is fixed at Initialize() time; transitions are
 * implemented by swapping which one is active.
 */
UCLASS()
class SIGNALFORGERPG_API USFCompanionStateMachine : public UObject
{
	GENERATED_BODY()

public:
	USFCompanionStateMachine();

	/** Construct all state instances and bind to the owning controller. */
	void Initialize(ASFCompanionAIController* InController);

	/** Tear down state instances + controller pointer. Safe to call twice. */
	void Shutdown();

	/** Server-side per-frame tick. Re-evaluates transitions then ticks the active state. */
	void Tick(float DeltaSeconds);

	/**
	 * External request to transition to a specific state. Returns true if
	 * the request was honored. The current state's CanTransitionTo is not
	 * consulted here — the FSM trusts the requester (typically the
	 * controller responding to a tactics delegate).
	 */
	bool RequestState(ESFCompanionAIState NewState);

	/** Re-evaluate the order-driven transition rules (called on OnOrderIssued). */
	void OnOrderChanged(const FSFCompanionOrder& Order);

	/** Re-evaluate threshold-driven rules (low health, etc). */
	void OnThresholdChanged();

	// -------------------------------------------------------------------------
	// Accessors used by individual state classes
	// -------------------------------------------------------------------------

	ASFCompanionAIController*     GetController() const { return Controller.Get(); }
	ASFCompanionCharacter*        GetCompanion()  const;
	USFCompanionTacticsComponent* GetTactics()    const;

	ESFCompanionAIState GetCurrentStateEnum() const { return CurrentStateEnum; }

	/** Last sequence number we acted on — lets state classes detect re-issued orders. */
	int32 GetLastHandledOrderSequence() const { return LastHandledOrderSequence; }
	void  SetLastHandledOrderSequence(int32 Seq) { LastHandledOrderSequence = Seq; }

protected:
	/** Picks the highest-priority state given current world/order/threshold context. */
	ESFCompanionAIState EvaluateDesiredState() const;

	/** Maps an order type to its corresponding FSM state. */
	static ESFCompanionAIState OrderTypeToState(ESFCompanionOrderType OrderType);

	/** Performs the actual state swap. No-op if NewState == CurrentStateEnum. */
	void TransitionTo(ESFCompanionAIState NewState);

	/** Looks up the state instance for an enum value. */
	FSFCompanionStateBase* FindState(ESFCompanionAIState State) const;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<ASFCompanionAIController> Controller;

	/** Owned state instances, one per ESFCompanionAIState entry. */
	TArray<TUniquePtr<FSFCompanionStateBase>> States;

	FSFCompanionStateBase* ActiveState = nullptr;
	ESFCompanionAIState    CurrentStateEnum = ESFCompanionAIState::Idle;

	int32 LastHandledOrderSequence = 0;
};
