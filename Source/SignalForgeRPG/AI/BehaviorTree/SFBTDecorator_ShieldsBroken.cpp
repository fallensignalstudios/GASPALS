#include "AI/BehaviorTree/SFBTDecorator_ShieldsBroken.h"

#include "AI/BehaviorTree/SFBTHelpers.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Characters/SFCharacterBase.h"
#include "Core/SFAttributeSetBase.h"

USFBTDecorator_ShieldsBroken::USFBTDecorator_ShieldsBroken()
{
	NodeName = TEXT("SF Shields Broken");
}

bool USFBTDecorator_ShieldsBroken::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/) const
{
	const ASFCharacterBase* Self = SFBTHelpers::GetControlledCharacter(OwnerComp.GetAIOwner());
	if (!Self)
	{
		return false;
	}

	const USFAttributeSetBase* AttrSet = Self->GetAttributeSet();
	if (!AttrSet)
	{
		return false;
	}

	// Enemies authored without shields (MaxShields <= 0) should never report
	// broken shields -- otherwise every unshielded grunt would trigger the
	// cover branch from spawn.
	const float MaxShields = AttrSet->GetMaxShields();
	if (MaxShields <= 0.0f)
	{
		return false;
	}

	const float CurrentShields = AttrSet->GetShields();
	return CurrentShields <= ShieldsThreshold;
}

FString USFBTDecorator_ShieldsBroken::GetStaticDescription() const
{
	return FString::Printf(TEXT("Shields Broken (Shields <= %.1f)"), ShieldsThreshold);
}
