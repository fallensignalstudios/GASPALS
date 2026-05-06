#include "AI/BTTask_MoveToFormationSlot.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"

UBTTask_MoveToFormationSlot::UBTTask_MoveToFormationSlot()
{
	NodeName = TEXT("Move To Formation Slot");
}

EBTNodeResult::Type UBTTask_MoveToFormationSlot::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	if (!BB || !AIC || !Pawn)
	{
		return EBTNodeResult::Failed;
	}

	const FVector Target = BB->GetValueAsVector(DesiredPositionKey);
	if (Target.IsNearlyZero())
	{
		return EBTNodeResult::Failed;
	}

	if (FVector::Dist2D(Pawn->GetActorLocation(), Target) <= AcceptanceRadius)
	{
		return EBTNodeResult::Succeeded;
	}

	const EPathFollowingRequestResult::Type Result = AIC->MoveToLocation(
		Target,
		AcceptanceRadius,
		/*bStopOnOverlap=*/true,
		bUsePathfinding,
		/*bProjectDestinationToNavigation=*/true,
		/*bCanStrafe=*/true,
		/*FilterClass=*/nullptr,
		/*bAllowPartialPath=*/true);

	switch (Result)
	{
		case EPathFollowingRequestResult::AlreadyAtGoal: return EBTNodeResult::Succeeded;
		case EPathFollowingRequestResult::RequestSuccessful: return EBTNodeResult::Succeeded;
		case EPathFollowingRequestResult::Failed:
		default: return EBTNodeResult::Failed;
	}
}
