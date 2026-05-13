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
		return EBTNodeResult::Succeeded;
	}

	const EPathFollowingRequestResult::Type MoveResult = AI->MoveToActor(Target, AcceptanceRadius, /*bStopOnOverlap*/ true, bUsePathfinding);
	if (MoveResult == EPathFollowingRequestResult::Failed)
	{
		return EBTNodeResult::Failed;
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
		AI->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
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
		const EPathFollowingRequestResult::Type MoveResult = AI->MoveToActor(Target, AcceptanceRadius, true, bUsePathfinding);
		if (MoveResult == EPathFollowingRequestResult::Failed)
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
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
