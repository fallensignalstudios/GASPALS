#include "AI/BehaviorTree/SFBTTask_StopFiring.h"

#include "AI/BehaviorTree/SFBTHelpers.h"
#include "AIController.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Characters/SFCharacterBase.h"
#include "Core/SignalForgeGameplayTags.h"

USFBTTask_StopFiring::USFBTTask_StopFiring()
{
	NodeName = TEXT("SF Stop Firing");
}

EBTNodeResult::Type USFBTTask_StopFiring::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ASFCharacterBase* Character = SFBTHelpers::GetControlledCharacter(OwnerComp.GetAIOwner());
	if (!Character)
	{
		return EBTNodeResult::Failed;
	}

	if (USFAbilitySystemComponent* ASC = Cast<USFAbilitySystemComponent>(Character->GetAbilitySystemComponent()))
	{
		ASC->AbilityInputTagReleased(FSignalForgeGameplayTags::Get().Input_PrimaryFire);
	}
	return EBTNodeResult::Succeeded;
}
