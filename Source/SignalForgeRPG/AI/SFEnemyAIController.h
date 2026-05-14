#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "SFEnemyAIController.generated.h"

class ASFEnemyCharacter;
class UBlackboardComponent;
class UBehaviorTreeComponent;
class UAIPerceptionComponent;

/**
 * ASFEnemyAIController
 *
 * Combat AI controller. Now uses AI Perception (sight + hearing) with
 * IGenericTeamAgentInterface-driven affiliation, so the behavior tree
 * only receives perception updates for actors the faction system
 * considers hostile to the controlled enemy.
 *
 * Wiring contract:
 *   - Possessed pawn must be an ASFEnemyCharacter (or subclass) with a
 *     BehaviorTreeAsset and a USFFactionComponent on its base.
 *   - The BT is expected to have an Object blackboard key named
 *     "TargetActor" -- the controller writes the most recently seen
 *     hostile into it. Designers can rename the key in their BB asset.
 */
UCLASS()
class SIGNALFORGERPG_API ASFEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ASFEnemyAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UFUNCTION(BlueprintPure, Category = "AI")
	ASFEnemyCharacter* GetControlledEnemy() const { return ControlledEnemy; }

	UFUNCTION(BlueprintPure, Category = "AI")
	UBlackboardComponent* GetBlackboardComponent() const { return BlackboardComponent; }

	/** Renamed accessor to avoid hiding AAIController::GetPerceptionComponent() (non-const virtual). */
	UFUNCTION(BlueprintPure, Category = "AI")
	UAIPerceptionComponent* GetEnemyPerceptionComponent() const { return Perception; }

protected:
	UPROPERTY(Transient)
	TObjectPtr<ASFEnemyCharacter> ControlledEnemy = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBlackboardComponent> BlackboardComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBehaviorTreeComponent> BehaviorTreeComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UAIPerceptionComponent> Perception = nullptr;

	/** Blackboard key the controller writes the perceived target into. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	FName TargetActorKeyName = TEXT("TargetActor");

	/** Blackboard Vector key written once on possess so patrol/return-home logic has an anchor. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	FName HomeLocationKeyName = TEXT("HomeLocation");

	UFUNCTION()
	void HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

private:
	/**
	 * Per-actor record of the last perception-update verdict we logged for them
	 * (0 = none/cleared, 1 = sensed-not-hostile, 2 = sensed-hostile-written).
	 * Used so the warnings in HandlePerceptionUpdated fire on edges only and
	 * don't spam the log while an enemy continuously sees the same actor.
	 */
	TMap<uint32, uint8> PerceptionLogStatePerActor;
};
