#include "AI/BehaviorTree/SFBTTask_MoveToCombatRange.h"

#include "AI/BehaviorTree/SFBTHelpers.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/SFCharacterBase.h"
#include "Navigation/PathFollowingComponent.h"

USFBTTask_MoveToCombatRange::USFBTTask_MoveToCombatRange()
{
	NodeName = TEXT("SF Move To Combat Range");
	bNotifyTick = true;
}

namespace
{
	void ResolveBand(const ASFCharacterBase* Self,
		bool bUseWeaponRange, float ExplicitMin, float ExplicitMax, float MinMul, float MaxMul,
		float& OutMin, float& OutMax)
	{
		if (bUseWeaponRange && Self)
		{
			const float WeaponRange = SFBTHelpers::GetEquippedWeaponRange(Self);
			if (WeaponRange > KINDA_SMALL_NUMBER)
			{
				OutMax = WeaponRange * MaxMul;
				OutMin = WeaponRange * MinMul;
				return;
			}
		}
		OutMin = ExplicitMin;
		OutMax = ExplicitMax;
	}
}

EBTNodeResult::Type USFBTTask_MoveToCombatRange::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

	FMemory* Mem = reinterpret_cast<FMemory*>(NodeMemory);
	*Mem = FMemory();
	Mem->LastTargetLoc = Target->GetActorLocation();

	float MinD = 0.0f;
	float MaxD = 0.0f;
	ResolveBand(Self, bUseWeaponRange, MinDistance, MaxDistance, MinRangeMultiplier, MaxRangeMultiplier, MinD, MaxD);

	const float DistNow = FVector::Dist(Self->GetActorLocation(), Mem->LastTargetLoc);
	if (DistNow >= MinD && DistNow <= MaxD)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[SFBTTask_MoveToCombatRange] '%s' Execute: already in band. Dist=%.0fcm, Band=[%.0f..%.0f] -> Succeeded."),
			*GetNameSafe(Self), DistNow, MinD, MaxD);
		return EBTNodeResult::Succeeded;
	}

	// CRITICAL: path follower's AcceptanceRadius determines how close MoveToActor
	// gets the pawn to the target. If the user-authored AcceptanceRadius lands
	// the pawn OUTSIDE our [MinD, MaxD] band, the task never succeeds and we
	// deadlock. Clamp the effective radius into the band so the pawn always
	// stops somewhere we count as success. Use MaxD - 10cm as the stopping
	// distance: it's well inside the band and leaves slack for the path
	// follower's own tolerance.
	const float EffectiveAcceptanceRadius = FMath::Max(AcceptanceRadius, FMath::Max(MinD + 10.0f, MaxD - 10.0f));

	const EPathFollowingRequestResult::Type MoveResult = AI->MoveToActor(Target, EffectiveAcceptanceRadius, /*bStopOnOverlap*/ true, bUsePathfinding);

	UE_LOG(LogTemp, Display,
		TEXT("[SFBTTask_MoveToCombatRange] '%s' Execute: Dist=%.0fcm, Band=[%.0f..%.0f], AcceptanceRadius=%.0fcm (authored=%.0f), MoveResult=%d."),
		*GetNameSafe(Self), DistNow, MinD, MaxD, EffectiveAcceptanceRadius, AcceptanceRadius, (int32)MoveResult);

	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[SFBTTask_MoveToCombatRange] '%s' MoveToActor FAILED -- check NavMesh coverage around target '%s' at %s."),
			*GetNameSafe(Self), *GetNameSafe(Target), *Target->GetActorLocation().ToString());
		return EBTNodeResult::Failed;
	}
	if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		// Path follower says we're at goal but our band check failed -- usually means
		// the pawn is too close (DistNow < MinD). Succeed anyway: the BT will
		// re-evaluate next tick and InWeaponRange (or whatever upstream gate) will
		// pick the next branch. Better than deadlocking.
		UE_LOG(LogTemp, Display,
			TEXT("[SFBTTask_MoveToCombatRange] '%s' Execute: AlreadyAtGoal (Dist=%.0f outside band [%.0f..%.0f]) -> Succeeded to let BT re-evaluate."),
			*GetNameSafe(Self), DistNow, MinD, MaxD);
		return EBTNodeResult::Succeeded;
	}
	Mem->bMoveIssued = (MoveResult == EPathFollowingRequestResult::RequestSuccessful);
	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type USFBTTask_MoveToCombatRange::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* AI = OwnerComp.GetAIOwner())
	{
		AI->StopMovement();
	}
	FMemory* Mem = reinterpret_cast<FMemory*>(NodeMemory);
	if (Mem)
	{
		Mem->bMoveIssued = false;
	}
	return EBTNodeResult::Aborted;
}

void USFBTTask_MoveToCombatRange::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FMemory* Mem = reinterpret_cast<FMemory*>(NodeMemory);
	AAIController* AI = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	ASFCharacterBase* Self = SFBTHelpers::GetControlledCharacter(AI);
	if (!Mem || !AI || !BB || !Self)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	AActor* Target = SFBTHelpers::GetBBActor(BB, BlackboardKey.SelectedKeyName);
	if (!Target)
	{
		AI->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	float MinD = 0.0f;
	float MaxD = 0.0f;
	ResolveBand(Self, bUseWeaponRange, MinDistance, MaxDistance, MinRangeMultiplier, MaxRangeMultiplier, MinD, MaxD);

	const FVector TargetLoc = Target->GetActorLocation();
	const float DistNow = FVector::Dist(Self->GetActorLocation(), TargetLoc);

	if (DistNow >= MinD && DistNow <= MaxD)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[SFBTTask_MoveToCombatRange] '%s' Tick: in band. Dist=%.0fcm, Band=[%.0f..%.0f] -> Succeeded."),
			*GetNameSafe(Self), DistNow, MinD, MaxD);
		AI->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// Watchdog: if the path follower has gone Idle but we're not in the band,
	// either we overshot (too close) or path failed. Succeed-fallback so the
	// BT can re-evaluate next tick instead of deadlocking here forever.
	Mem->TimeSinceMoveCheck += DeltaSeconds;
	if (Mem->TimeSinceMoveCheck >= 0.25f)
	{
		Mem->TimeSinceMoveCheck = 0.0f;
		const bool bPathFollowingIdle = !AI->GetPathFollowingComponent()
			|| AI->GetPathFollowingComponent()->GetStatus() == EPathFollowingStatus::Idle;
		if (Mem->bMoveIssued && bPathFollowingIdle)
		{
			UE_LOG(LogTemp, Display,
				TEXT("[SFBTTask_MoveToCombatRange] '%s' Tick: path follower Idle outside band (Dist=%.0f, Band=[%.0f..%.0f]) -> Succeeded to let BT re-evaluate."),
				*GetNameSafe(Self), DistNow, MinD, MaxD);
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}
	}

	Mem->TimeSinceRepath += DeltaSeconds;
	const float TargetDrift = FVector::Dist(TargetLoc, Mem->LastTargetLoc);
	const bool bShouldRepath = !Mem->bMoveIssued
		|| Mem->TimeSinceRepath >= RepathInterval
		|| TargetDrift > AcceptanceRadius;

	if (bShouldRepath)
	{
		Mem->TimeSinceRepath = 0.0f;
		Mem->LastTargetLoc = TargetLoc;
		const float EffectiveAcceptanceRadius = FMath::Max(AcceptanceRadius, FMath::Max(MinD + 10.0f, MaxD - 10.0f));
		const EPathFollowingRequestResult::Type MoveResult = AI->MoveToActor(Target, EffectiveAcceptanceRadius, true, bUsePathfinding);
		if (MoveResult == EPathFollowingRequestResult::Failed)
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			return;
		}
		if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			return;
		}
		Mem->bMoveIssued = (MoveResult == EPathFollowingRequestResult::RequestSuccessful);
	}
}

FString USFBTTask_MoveToCombatRange::GetStaticDescription() const
{
	if (bUseWeaponRange)
	{
		return FString::Printf(TEXT("Move To %s combat band (weapon * [%.2f .. %.2f])"),
			*BlackboardKey.SelectedKeyName.ToString(), MinRangeMultiplier, MaxRangeMultiplier);
	}
	return FString::Printf(TEXT("Move To %s combat band [%.0f .. %.0f]"),
		*BlackboardKey.SelectedKeyName.ToString(), MinDistance, MaxDistance);
}
