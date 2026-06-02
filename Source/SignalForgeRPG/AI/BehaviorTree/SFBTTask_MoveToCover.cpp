#include "AI/BehaviorTree/SFBTTask_MoveToCover.h"

#include "AI/BehaviorTree/SFBTHelpers.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/SFCharacterBase.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"

USFBTTask_MoveToCover::USFBTTask_MoveToCover()
{
	NodeName = TEXT("SF Move To Cover");
	bNotifyTick = true;
}

EBTNodeResult::Type USFBTTask_MoveToCover::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

	UWorld* World = Self->GetWorld();
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);
	if (!World || !NavSys)
	{
		return EBTNodeResult::Failed;
	}

	FMemory* Mem = reinterpret_cast<FMemory*>(NodeMemory);
	*Mem = FMemory();

	const FVector SelfLoc = Self->GetActorLocation();
	const FVector TargetLoc = Target->GetActorLocation();
	const FVector TargetEye = TargetLoc + FVector(0.0f, 0.0f, EyeHeightOffset);

	// Sanitize radii in case a designer flips them.
	const float InnerR = FMath::Min(MinSearchRadius, MaxSearchRadius);
	const float OuterR = FMath::Max(MinSearchRadius, MaxSearchRadius);
	const float MinDistSq = MinDistanceFromTarget * MinDistanceFromTarget;

	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(SFBTTask_MoveToCover_LOS), /*bTraceComplex*/ false);
	TraceParams.AddIgnoredActor(Self);
	TraceParams.AddIgnoredActor(Target);

	FNavLocation BestPt;
	float BestDistToSelfSq = TNumericLimits<float>::Max();
	bool bFound = false;

	for (int32 i = 0; i < SampleCount; ++i)
	{
		// Random point inside the annulus around the AI.
		const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
		const float Radius = FMath::FRandRange(InnerR, OuterR);
		const FVector Offset(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f);
		const FVector CandidateIdeal = SelfLoc + Offset;

		// Project to navmesh. Small search radius so we don't snap onto a
		// totally different surface.
		FNavLocation NavPt;
		if (!NavSys->ProjectPointToNavigation(CandidateIdeal, NavPt, FVector(200.0f, 200.0f, 400.0f)))
		{
			continue;
		}

		// Must put real distance between us and the threat.
		if (FVector::DistSquared(NavPt.Location, TargetLoc) < MinDistSq)
		{
			continue;
		}

		// LOS check: trace from candidate eye to target eye. If the trace
		// HITS something, the candidate is in cover.
		const FVector CandidateEye = NavPt.Location + FVector(0.0f, 0.0f, EyeHeightOffset);
		FHitResult Hit;
		const bool bBlocked = World->LineTraceSingleByChannel(Hit, CandidateEye, TargetEye, ECC_Visibility, TraceParams);
		if (!bBlocked)
		{
			continue;
		}

		// Prefer the closest valid cover so the AI doesn't sprint across the
		// map when something is right next to it.
		const float DistToSelfSq = FVector::DistSquared(NavPt.Location, SelfLoc);
		if (DistToSelfSq < BestDistToSelfSq)
		{
			BestDistToSelfSq = DistToSelfSq;
			BestPt = NavPt;
			bFound = true;
		}
	}

	if (!bFound)
	{
		return EBTNodeResult::Failed;
	}

	const EPathFollowingRequestResult::Type MoveResult = AI->MoveToLocation(BestPt.Location, AcceptanceRadius, true, /*bUsePathfinding*/ true);
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

EBTNodeResult::Type USFBTTask_MoveToCover::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	if (AAIController* AI = OwnerComp.GetAIOwner())
	{
		AI->StopMovement();
	}
	return EBTNodeResult::Aborted;
}

void USFBTTask_MoveToCover::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
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

FString USFBTTask_MoveToCover::GetStaticDescription() const
{
	return FString::Printf(TEXT("Move To Cover (LOS-break from %s, %.0f-%.0fcm)"),
		*BlackboardKey.SelectedKeyName.ToString(), MinSearchRadius, MaxSearchRadius);
}
