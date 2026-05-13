#include "AI/BehaviorTree/SFBTDecorator_IsHostile.h"

#include "AI/BehaviorTree/SFBTHelpers.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/SFCharacterBase.h"

USFBTDecorator_IsHostile::USFBTDecorator_IsHostile()
{
	NodeName = TEXT("SF Is Hostile");
}

bool USFBTDecorator_IsHostile::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/) const
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

	return SFBTHelpers::IsHostile(Self, Target);
}

FString USFBTDecorator_IsHostile::GetStaticDescription() const
{
	return FString::Printf(TEXT("Is Hostile: %s"), *BlackboardKey.SelectedKeyName.ToString());
}
