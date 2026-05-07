#pragma once

#include "CoreMinimal.h"
#include "AI/SFNPCAIController.h"
#include "AI/Companion/SFCompanionAIState.h"
#include "Companions/SFCompanionTypes.h"
#include "SFCompanionAIController.generated.h"

class ASFCompanionCharacter;
class USFCompanionStateMachine;

/**
 * ASFCompanionAIController
 *
 * Drives a companion pawn via a C++ finite state machine instead of a
 * behavior tree. Subscribes to the tactics component's order / stance /
 * threshold delegates and forwards them as event-driven transitions to
 * the FSM, while ticking the FSM every frame for poll-driven checks
 * (target acquisition, distance, repathing).
 *
 * Server-authoritative: only the server runs the FSM. CurrentAIState is
 * replicated so client AnimBPs / UI can read the high-level state
 * without round-tripping the tactics component.
 */
UCLASS()
class SIGNALFORGERPG_API ASFCompanionAIController : public ASFNPCAIController
{
	GENERATED_BODY()

public:
	ASFCompanionAIController();

	//~ Begin AAIController
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End AAIController

	UFUNCTION(BlueprintPure, Category = "Companion|AI")
	ASFCompanionCharacter* GetControlledCompanion() const { return ControlledCompanion; }

	/** Replicated; safe to read on client AnimBPs. */
	UFUNCTION(BlueprintPure, Category = "Companion|AI")
	ESFCompanionAIState GetCurrentAIState() const { return CurrentAIState; }

	/** Called by the state machine after a transition completes (server-side). */
	void NotifyAIStateChanged(ESFCompanionAIState NewState);

protected:
	UPROPERTY(Transient)
	TObjectPtr<ASFCompanionCharacter> ControlledCompanion = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USFCompanionStateMachine> StateMachine = nullptr;

	/** Replicated to all clients so AnimBP can drive locomotion blends. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Companion|AI")
	ESFCompanionAIState CurrentAIState = ESFCompanionAIState::Idle;

	// -------------------------------------------------------------------------
	// Tactics delegate handlers (event-driven half of the hybrid model)
	// -------------------------------------------------------------------------

	UFUNCTION()
	void HandleStanceChanged(ESFCompanionStance OldStance, ESFCompanionStance NewStance);

	UFUNCTION()
	void HandleOrderIssued(const FSFCompanionOrder& NewOrder);

	UFUNCTION()
	void HandleThresholdCrossed(FGameplayTag ThresholdTag, bool bNowActive);
};
