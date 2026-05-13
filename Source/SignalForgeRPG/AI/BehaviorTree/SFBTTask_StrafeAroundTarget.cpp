#include "AI/BehaviorTree/SFBTTask_StrafeAroundTarget.h"

#include "AI/BehaviorTree/SFBTHelpers.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/SFCharacterBase.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"

USFBTTask_StrafeAroundTarget::USFBTTask_StrafeAroundTarget()
{
	NodeName = TEXT("SF Strafe Around Target");
	bNotifyTick = true;
}

EBTNodeResult::Type USFBTTask_StrafeAroundTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

	// Resolve direction.
	bool bGoRight = false;
	switch (Direction)
	{
		case EStrafeDirection::Right: bGoRight = true; break;
		case EStrafeDirection::Left:  bGoRight = false; break;
		case EStrafeDirection::Auto:
		default:
			bGoRight = !bLastWentRight;
			bLastWentRight = bGoRight;
			break;
	}

	// Lateral basis: take "right" of the Self->Target vector projected on the ground.
	const FVector SelfLoc = Self->GetActorLocation();
	const FVector TargetLoc = Target->GetActorLocation();
	FVector Forward = TargetLoc - SelfLoc;
	Forward.Z = 0.0f;
	if (!Forward.Normalize())
	{
		return EBTNodeResult::Failed;
	}
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();
	const FVector Lateral = bGoRight ? Right : -Right;

	// Anchor point: project Self's current radial offset so we keep our distance,
	// then offset laterally by StrafeDistance.
	const float CurrentRadius = FVector::Dist2D(SelfLoc, TargetLoc);
	const FVector Anchor = TargetLoc - Forward * CurrentRadius + Lateral * StrafeDistance;

	FNavLocation NavPt;
	bool bFound = false;
	for (int32 i = 0; i < MaxAttempts; ++i)
	{
		if (NavSys->GetRandomReachablePointInRadius(Anchor, NavSearchRadius, NavPt))
		{
			bFound = true;
			break;
		}
	}
	if (!bFound)
	{
		return EBTNodeResult::Failed;
	}

	const EPathFollowingRequestResult::Type MoveResult = AI->MoveToLocation(NavPt.Location, AcceptanceRadius, /*bStopOnOverlap*/ true, /*bUsePathfinding*/ true);
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

EBTNodeResult::Type USFBTTask_StrafeAroundTarget::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	if (AAIController* AI = OwnerComp.GetAIOwner())
	{
		AI->StopMovement();
	}
	return EBTNodeResult::Aborted;
}

void USFBTTask_StrafeAroundTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
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
		// MoveTo finished (succeeded or failed at the engine layer).
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	if (Mem->Elapsed >= TimeoutSeconds)
	{
		AI->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
	}
}

FString USFBTTask_StrafeAroundTarget::GetStaticDescription() const
{
	const TCHAR* DirText = TEXT("Auto");
	switch (Direction)
	{
		case EStrafeDirection::Left:  DirText = TEXT("Left"); break;
		case EStrafeDirection::Right: DirText = TEXT("Right"); break;
		default: break;
	}
	return FString::Printf(TEXT("Strafe %s around %s (%.0fcm)"),
		DirText, *BlackboardKey.SelectedKeyName.ToString(), StrafeDistance);
}
