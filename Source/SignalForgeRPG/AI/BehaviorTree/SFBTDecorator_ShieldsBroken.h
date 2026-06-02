#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "SFBTDecorator_ShieldsBroken.generated.h"

/**
 * Returns true when the AI's Shields attribute is at or below ShieldsThreshold
 * AND the pawn was authored with a positive MaxShields (so unshielded enemies
 * never report "shields broken"). Pair this with SFBTTask_MoveToCover to gate
 * a "duck behind something while my shields regen" branch:
 *
 *   Selector
 *   ├── [ShieldsBroken]  MoveToCover -> wait (no body, just sit)
 *   ├── [NeedsReload]    MoveToCover -> Reload
 *   ├── Combat subtree (FaceTarget -> FireWeapon)
 *   └── ...
 *
 * Default ShieldsThreshold = 0 (only triggers when shields are fully depleted).
 */
UCLASS()
class SIGNALFORGERPG_API USFBTDecorator_ShieldsBroken : public UBTDecorator
{
	GENERATED_BODY()

public:
	USFBTDecorator_ShieldsBroken();

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;

protected:
	/** Trigger when current Shields <= this value. 0 = only when fully broken. */
	UPROPERTY(EditAnywhere, Category = "Shields", meta = (ClampMin = "0.0"))
	float ShieldsThreshold = 0.0f;
};
