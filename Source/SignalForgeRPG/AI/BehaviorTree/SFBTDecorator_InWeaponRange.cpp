#include "AI/BehaviorTree/SFBTDecorator_InWeaponRange.h"

#include "AI/BehaviorTree/SFBTHelpers.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/SFCharacterBase.h"

USFBTDecorator_InWeaponRange::USFBTDecorator_InWeaponRange()
{
	NodeName = TEXT("SF In Weapon Range");
}

bool USFBTDecorator_InWeaponRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/) const
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

	const float WeaponRange = SFBTHelpers::GetEquippedWeaponRange(Self);
	if (WeaponRange <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const float EffectiveRange = WeaponRange * RangeMultiplier;
	const float DistSq = FVector::DistSquared(Self->GetActorLocation(), Target->GetActorLocation());
	return DistSq <= (EffectiveRange * EffectiveRange);
}

FString USFBTDecorator_InWeaponRange::GetStaticDescription() const
{
	return FString::Printf(TEXT("In Weapon Range (%s, x%.2f)"),
		*BlackboardKey.SelectedKeyName.ToString(), RangeMultiplier);
}
