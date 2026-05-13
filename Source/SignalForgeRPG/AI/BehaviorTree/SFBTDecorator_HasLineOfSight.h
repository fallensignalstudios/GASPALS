#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Decorators/BTDecorator_BlackboardBase.h"
#include "SFBTDecorator_HasLineOfSight.generated.h"

/**
 * Returns true if the AI has unobstructed line-of-sight to the actor stored
 * in the configured BB Actor key. Uses an eye-to-chest trace (ECC_Visibility)
 * ignoring both self and target.
 *
 * Gate any "fire" / "throw grenade" / "cast" subtree with this so enemies
 * don't shoot through walls.
 */
UCLASS()
class SIGNALFORGERPG_API USFBTDecorator_HasLineOfSight : public UBTDecorator_BlackboardBase
{
	GENERATED_BODY()

public:
	USFBTDecorator_HasLineOfSight();

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;
};
