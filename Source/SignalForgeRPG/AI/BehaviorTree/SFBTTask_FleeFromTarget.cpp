#include "AI/BehaviorTree/SFBTTask_FleeFromTarget.h"

#include "AI/BehaviorTree/SFBTHelpers.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/SFCharacterBase.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"

USFBTTask_FleeFromTarget::USFBTTask_FleeFromTarget()
{
	NodeName = TEXT("SF Flee From Target");
	bNotifyTick = true;
}

EBTNodeResult::Type USFBTTask_FleeFromTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AI = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	ASFCharacterBase* Self = SFBTHelpers::GetControlledCharacter(AI);
	if (!AI || !BB || !Self)
	{
		return EBTNodeResult::Failed;
	}

	AActor* Target = SFBTHelpers::GetBBActor(BB, BlackboardKey.SelectedKeyName);
	if (!Target)
	{
		return EBTNodeResult::Failed;
	}

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Self->GetWorld());
	if (!NavSys)
	{
		return EBTNodeResult::Failed;
	}

	FMemory* Mem = reinterpret_cast<FMemory*>(NodeMemory);
	*Mem = FMemory();

	const FVector SelfLoc = Self->GetActorLocation();
	const FVector TargetLoc = Target->GetActorLocation();
	FVector AwayDir = SelfLoc - TargetLoc;
	AwayDir.Z = 0.0f;
	if (!AwayDir.Normalize())
	{
		// Standing exactly on the target — pick an arbitrary cardinal so we don't bail.
		AwayDir = FVector::ForwardVector;
	}

	const FVector Ideal = SelfLoc + AwayDir * FleeDistance;

	FNavLocation NavPt;
	bool bFound = false;
	for (int32 i = 0; i < MaxAttempts; ++i)
	{
		if (NavSys->GetRandomReachablePointInRadius(Ideal, NavSearchRadius, NavPt))
		{
			// Verify this point actually puts more distance between us and the target than we have now.
			if (FVector::Dist(NavPt.Location, TargetLoc) > FVector::Dist(SelfLoc, TargetLoc))
			{
				bFound = true;
				break;
			}
		}
	}
	if (!bFound)
	{
		return EBTNodeResult::Failed;
	}

	const EPathFollowingRequestResult::Type MoveResult = AI->MoveToLocation(NavPt.Location, AcceptanceRadius, true, /*bUsePathfinding*/ true);
	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		return EBTNodeResult::Failed;
	}
	if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		return EBTNodeResult::Succeeded;
	}
	Mem->bMoveIssued = true;
	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type USFBTTask_FleeFromTarget::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	if (AAIController* AI = OwnerComp.GetAIOwner())
	{
		AI->StopMovement();
	}
	return EBTNodeResult::Aborted;
}

void USFBTTask_FleeFromTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FMemory* Mem = reinterpret_cast<FMemory*>(NodeMemory);
	AAIController* AI = OwnerComp.GetAIOwner();
	if (!Mem || !AI)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	Mem->Elapsed += DeltaSeconds;

	UPathFollowingComponent* PFC = AI->GetPathFollowingComponent();
	if (PFC && PFC->GetStatus() == EPathFollowingStatus::Idle)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	if (Mem->Elapsed >= TimeoutSeconds)
	{
		AI->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
	}
}

FString USFBTTask_FleeFromTarget::GetStaticDescription() const
{
	return FString::Printf(TEXT("Flee from %s (%.0fcm)"),
		*BlackboardKey.SelectedKeyName.ToString(), FleeDistance);
}
