#pragma once

#include "CoreMinimal.h"
#include "AI/SFNPCAIController.h"
#include "Companions/SFCompanionTypes.h"
#include "SFCompanionAIController.generated.h"

class ASFCompanionCharacter;

/**
 * ASFCompanionAIController
 *
 * Reads the companion's tactics component (stance, aggression, engagement
 * range, active order) and pushes the relevant data onto the blackboard
 * each frame the BT needs it. Designers wire BT services/decorators that
 * branch on the blackboard keys; the controller stays free of behavior
 * hard-coding.
 *
 * Standard blackboard keys (designer-overridable in BT asset):
 *   - PlayerActor       (Object)  — current player pawn to follow / regroup
 *   - OrderType         (Enum)    — active order type
 *   - OrderTargetActor  (Object)
 *   - OrderTargetLoc    (Vector)
 *   - OrderAbilityTag   (Name)    — tag stringified for BT comparison
 *   - StanceTag         (Name)    — first stance tag, for quick-switching subtrees
 */
UCLASS()
class SIGNALFORGERPG_API ASFCompanionAIController : public ASFNPCAIController
{
	GENERATED_BODY()

public:
	ASFCompanionAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UFUNCTION(BlueprintPure, Category = "Companion|AI")
	ASFCompanionCharacter* GetControlledCompanion() const { return ControlledCompanion; }

protected:
	UPROPERTY(Transient)
	TObjectPtr<ASFCompanionCharacter> ControlledCompanion = nullptr;

	UFUNCTION()
	void HandleStanceChanged(ESFCompanionStance OldStance, ESFCompanionStance NewStance);

	UFUNCTION()
	void HandleOrderIssued(const FSFCompanionOrder& NewOrder);

	void PushOrderToBlackboard(const FSFCompanionOrder& Order);
	void PushStanceToBlackboard();
	void PushPlayerToBlackboard();
};
