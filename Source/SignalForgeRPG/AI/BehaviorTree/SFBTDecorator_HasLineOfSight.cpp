#include "AI/BehaviorTree/SFBTDecorator_HasLineOfSight.h"

#include "AI/BehaviorTree/SFBTHelpers.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/SFCharacterBase.h"

USFBTDecorator_HasLineOfSight::USFBTDecorator_HasLineOfSight()
{
	NodeName = TEXT("SF Has Line Of Sight");
}

bool USFBTDecorator_HasLineOfSight::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/) const
{
	AAIController* AI = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	ASFCharacterBase* Self = SFBTHelpers::GetControlledCharacter(AI);
	if (!AI || !BB || !Self)
	{
		return false;
	}

	AActor* Target = SFBTHelpers::GetBBActor(BB, BlackboardKey.SelectedKeyName);
	if (!Target)
	{
		return false;
	}

	return SFBTHelpers::HasLineOfSight(Self, Target);
}

FString USFBTDecorator_HasLineOfSight::GetStaticDescription() const
{
	return FString::Printf(TEXT("Has LOS: %s"), *BlackboardKey.SelectedKeyName.ToString());
}
