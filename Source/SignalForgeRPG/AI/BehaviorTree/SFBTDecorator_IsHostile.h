#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Decorators/BTDecorator_BlackboardBase.h"
#include "SFBTDecorator_IsHostile.generated.h"

/**
 * Returns true if the actor stored in the configured BB Actor key is hostile
 * to the AI's controlled pawn per the global faction relationship table.
 * Returns false if the key is empty.
 *
 * Use to gate combat branches so that allied / neutral perceived actors don't
 * accidentally trigger fire-on-friendlies behavior.
 */
UCLASS()
class SIGNALFORGERPG_API USFBTDecorator_IsHostile : public UBTDecorator_BlackboardBase
{
	GENERATED_BODY()

public:
	USFBTDecorator_IsHostile();

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;
};
