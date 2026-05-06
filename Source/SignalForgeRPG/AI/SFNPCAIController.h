#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "SFNPCAIController.generated.h"

class ASFNPCBase;
class UAIPerceptionComponent;
class UBehaviorTreeComponent;
class UBlackboardComponent;
class UBehaviorTree;

/**
 * ASFNPCAIController
 *
 * Default AI controller for non-hostile / civilian / quest-giver NPCs.
 * Provides:
 *   - AI Perception component (sight + hearing) for awareness of nearby
 *     actors so dispositions can react (e.g. flee on Hostile, greet on
 *     Friendly approach).
 *   - Behavior tree + blackboard slots that designers can wire per-NPC.
 *   - Disposition-aware perception forwarding: when the perception system
 *     reports a hostile target, the controller flips the NPC's
 *     disposition through the narrative identity component.
 *
 * Combat NPCs should keep using ASFEnemyAIController; this class is
 * deliberately scoped to the social/dialogue path.
 */
UCLASS()
class SIGNALFORGERPG_API ASFNPCAIController : public AAIController
{
	GENERATED_BODY()

public:
	ASFNPCAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UFUNCTION(BlueprintPure, Category = "AI")
	ASFNPCBase* GetControlledNPC() const { return ControlledNPC; }

	UFUNCTION(BlueprintPure, Category = "AI")
	UBlackboardComponent* GetBlackboardComponent() const { return BlackboardComponent; }

	UFUNCTION(BlueprintPure, Category = "AI")
	UBehaviorTreeComponent* GetBehaviorTreeComponent() const { return BehaviorTreeComponent; }

	/** Renamed accessor to avoid hiding AAIController::GetPerceptionComponent() (non-const virtual). */
	UFUNCTION(BlueprintPure, Category = "AI")
	UAIPerceptionComponent* GetNPCPerceptionComponent() const { return Perception; }

protected:
	UPROPERTY(Transient)
	TObjectPtr<ASFNPCBase> ControlledNPC = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBlackboardComponent> BlackboardComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBehaviorTreeComponent> BehaviorTreeComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAIPerceptionComponent> Perception = nullptr;

	/** Default behavior tree to run on possess. NPCs can override per-instance. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> DefaultBehaviorTree = nullptr;

	UFUNCTION()
	void HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
};
