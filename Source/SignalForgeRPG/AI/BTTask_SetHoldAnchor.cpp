#include "AI/BTTask_SetHoldAnchor.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"

UBTTask_SetHoldAnchor::UBTTask_SetHoldAnchor()
{
	NodeName = TEXT("Set Hold Anchor");
}

EBTNodeResult::Type UBTTask_SetHoldAnchor::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!BB || !AIC)
	{
		return EBTNodeResult::Failed;
	}

	FVector Anchor = BB->GetValueAsVector(OrderTargetLocationKey);
	if (Anchor.IsNearlyZero())
	{
		if (APawn* Pawn = AIC->GetPawn())
		{
			Anchor = Pawn->GetActorLocation();
		}
		else
		{
			return EBTNodeResult::Failed;
		}
	}

	BB->SetValueAsVector(HomeAnchorLocationKey, Anchor);
	return EBTNodeResult::Succeeded;
}
