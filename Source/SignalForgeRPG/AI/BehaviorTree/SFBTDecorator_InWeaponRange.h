#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Decorators/BTDecorator_BlackboardBase.h"
#include "SFBTDecorator_InWeaponRange.generated.h"

/**
 * Returns true if the distance from the AI to the target stored in the BB
 * Actor key is within the AI's currently-equipped weapon range, optionally
 * scaled by RangeMultiplier. A multiplier of 0.85 means "fire only when the
 * target is comfortably inside effective range".
 *
 * Returns false if there is no equipped weapon (i.e. weapon range == 0).
 *
 * Use to gate the "fire" subtree so enemies don't waste shots at extreme range.
 */
UCLASS()
class SIGNALFORGERPG_API USFBTDecorator_InWeaponRange : public UBTDecorator_BlackboardBase
{
	GENERATED_BODY()

public:
	USFBTDecorator_InWeaponRange();

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Range", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float RangeMultiplier = 0.85f;
};
