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
	bNotifyBecomeRelevant = true;

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

void USFBTService_PatrolPoint::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	FMemory* Mem = reinterpret_cast<FMemory*>(NodeMemory);
	if (!Mem)
	{
		return;
	}
	*Mem = FMemory();

	// Immediately roll a point so Move To has a valid target on its first tick.
	TryRollPatrolPoint(OwnerComp, Mem, /*bForceReroll*/ true);
}

void USFBTService_PatrolPoint::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	FMemory* Mem = reinterpret_cast<FMemory*>(NodeMemory);
	if (!Mem)
	{
		return;
	}
	Mem->SecondsSinceLastRoll += DeltaSeconds;
	TryRollPatrolPoint(OwnerComp, Mem, /*bForceReroll*/ false);
}

bool USFBTService_PatrolPoint::TryRollPatrolPoint(UBehaviorTreeComponent& OwnerComp, FMemory* Mem, bool bForceReroll) const
{
	AAIController* AI = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	ASFCharacterBase* Self = SFBTHelpers::GetControlledCharacter(AI);
	if (!Mem || !AI || !BB || !Self || PatrolPointKey.SelectedKeyName == NAME_None)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[SFAI Patrol] Missing AI/BB/Self/key; skipping roll."));
		return false;
	}

	FVector Anchor = FVector::ZeroVector;
	if (HomeLocationKey.SelectedKeyName != NAME_None)
	{
		Anchor = BB->GetValueAsVector(HomeLocationKey.SelectedKeyName);
	}

	// UE leaves unset Vector blackboard keys at FVector(FLT_MAX) (~3.4e38), not
	// zero, so a NearlyZero check alone silently lets that sentinel through and
	// NavSystem then tries to find points at the edge of float-space (we saw
	// ~120 'no reachable point' warnings around X=3.4e38 in the field). Treat
	// any anchor with NaN or any component beyond a sane world bound as invalid
	// and fall back to the pawn's current location instead.
	const float kSaneWorldBound = 1.0e6f; // 10 km in UE units; far beyond any real level.
	const bool bAnchorInvalid =
		Anchor.ContainsNaN() ||
		Anchor.IsNearlyZero() ||
		FMath::Abs(Anchor.X) > kSaneWorldBound ||
		FMath::Abs(Anchor.Y) > kSaneWorldBound ||
		FMath::Abs(Anchor.Z) > kSaneWorldBound;
	if (bAnchorInvalid)
	{
		if (!bFallbackToCurrentLocation)
		{
			UE_LOG(LogTemp, Verbose,
				TEXT("[SFAI Patrol] '%s' HomeLocation key invalid (%s) and fallback disabled; skipping roll."),
				*GetNameSafe(Self), *Anchor.ToString());
			return false;
		}
		UE_LOG(LogTemp, Verbose,
			TEXT("[SFAI Patrol] '%s' HomeLocation key invalid (%s); falling back to current location."),
			*GetNameSafe(Self), *Anchor.ToString());
		Anchor = Self->GetActorLocation();
	}

	const FVector CurrentPoint = BB->GetValueAsVector(PatrolPointKey.SelectedKeyName);
	const FVector SelfLoc = Self->GetActorLocation();

	// PatrolPoint may also be FLT_MAX on first tick (UE default for unset Vector
	// keys). Guard the same way so we always reroll into a real value.
	const bool bCurrentPointInvalid =
		CurrentPoint.ContainsNaN() ||
		CurrentPoint.IsNearlyZero() ||
		FMath::Abs(CurrentPoint.X) > kSaneWorldBound ||
		FMath::Abs(CurrentPoint.Y) > kSaneWorldBound ||
		FMath::Abs(CurrentPoint.Z) > kSaneWorldBound;
	const float DistToCurrent = bCurrentPointInvalid ? FLT_MAX : FVector::Dist(SelfLoc, CurrentPoint);

	const bool bNeedsReroll =
		bForceReroll
		|| bCurrentPointInvalid
		|| DistToCurrent <= ArrivalDistance
		|| Mem->SecondsSinceLastRoll >= StuckTimeout;

	if (!bNeedsReroll)
	{
		return false;
	}

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Self->GetWorld());
	if (!NavSys)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SFAI Patrol] No NavigationSystemV1 in world for '%s' -- patrol cannot pick points."), *GetNameSafe(Self));
		return false;
	}

	FNavLocation NavPt;
	for (int32 i = 0; i < MaxAttempts; ++i)
	{
		if (NavSys->GetRandomReachablePointInRadius(Anchor, PatrolRadius, NavPt))
		{
			BB->SetValueAsVector(PatrolPointKey.SelectedKeyName, NavPt.Location);
			Mem->SecondsSinceLastRoll = 0.0f;
			UE_LOG(LogTemp, Verbose, TEXT("[SFAI Patrol] '%s' rolled patrol point %s (anchor %s)."),
				*GetNameSafe(Self), *NavPt.Location.ToString(), *Anchor.ToString());
			return true;
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[SFAI Patrol] '%s' could not find a reachable point within %.0fcm of %s after %d attempts. "
		     "Check that a NavMeshBoundsVolume covers this area and that Anchor sits on or near the navmesh."),
		*GetNameSafe(Self), PatrolRadius, *Anchor.ToString(), MaxAttempts);
	return false;
}

FString USFBTService_PatrolPoint::GetStaticDescription() const
{
	return FString::Printf(TEXT("Patrol point within %.0fcm of %s -> %s"),
		PatrolRadius,
		*HomeLocationKey.SelectedKeyName.ToString(),
		*PatrolPointKey.SelectedKeyName.ToString());
}
