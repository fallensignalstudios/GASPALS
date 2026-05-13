#include "AI/BehaviorTree/SFBTService_PatrolPoint.h"

#include "AI/BehaviorTree/SFBTHelpers.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Characters/SFCharacterBase.h"
#include "NavigationSystem.h"

USFBTService_PatrolPoint::USFBTService_PatrolPoint()
{
	NodeName = TEXT("SF Patrol Point");
	Interval = 0.5f;
	RandomDeviation = 0.1f;
	bNotifyTick = true;

	HomeLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(USFBTService_PatrolPoint, HomeLocationKey));
	PatrolPointKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(USFBTService_PatrolPoint, PatrolPointKey));
}

void USFBTService_PatrolPoint::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		HomeLocationKey.ResolveSelectedKey(*BBAsset);
		PatrolPointKey.ResolveSelectedKey(*BBAsset);
	}
}

void USFBTService_PatrolPoint::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	FMemory* Mem = reinterpret_cast<FMemory*>(NodeMemory);
	AAIController* AI = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	ASFCharacterBase* Self = SFBTHelpers::GetControlledCharacter(AI);
	if (!Mem || !AI || !BB || !Self || PatrolPointKey.SelectedKeyName == NAME_None)
	{
		return;
	}

	Mem->SecondsSinceLastRoll += DeltaSeconds;

	// Resolve the anchor (HomeLocation, with current location as fallback).
	FVector Anchor = FVector::ZeroVector;
	if (HomeLocationKey.SelectedKeyName != NAME_None)
	{
		Anchor = BB->GetValueAsVector(HomeLocationKey.SelectedKeyName);
	}
	if (Anchor.IsNearlyZero())
	{
		if (!bFallbackToCurrentLocation)
		{
			return;
		}
		Anchor = Self->GetActorLocation();
	}

	const FVector CurrentPoint = BB->GetValueAsVector(PatrolPointKey.SelectedKeyName);
	const FVector SelfLoc = Self->GetActorLocation();
	const float DistToCurrent = FVector::Dist(SelfLoc, CurrentPoint);

	const bool bNeedsReroll =
		CurrentPoint.IsNearlyZero()
		|| DistToCurrent <= ArrivalDistance
		|| Mem->SecondsSinceLastRoll >= StuckTimeout;

	if (!bNeedsReroll)
	{
		return;
	}

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Self->GetWorld());
	if (!NavSys)
	{
		return;
	}

	FNavLocation NavPt;
	for (int32 i = 0; i < MaxAttempts; ++i)
	{
		if (NavSys->GetRandomReachablePointInRadius(Anchor, PatrolRadius, NavPt))
		{
			BB->SetValueAsVector(PatrolPointKey.SelectedKeyName, NavPt.Location);
			Mem->SecondsSinceLastRoll = 0.0f;
			return;
		}
	}
}

FString USFBTService_PatrolPoint::GetStaticDescription() const
{
	return FString::Printf(TEXT("Patrol point within %.0fcm of %s -> %s"),
		PatrolRadius,
		*HomeLocationKey.SelectedKeyName.ToString(),
		*PatrolPointKey.SelectedKeyName.ToString());
}
