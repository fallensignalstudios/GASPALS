#include "AI/BehaviorTree/SFBTService_SenseUpdate.h"

#include "AI/BehaviorTree/SFBTHelpers.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Characters/SFCharacterBase.h"
#include "Perception/AIPerceptionComponent.h"

USFBTService_SenseUpdate::USFBTService_SenseUpdate()
{
	NodeName = TEXT("SF Sense Update");
	Interval = 0.25f;
	RandomDeviation = 0.05f;
	bNotifyTick = true;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(USFBTService_SenseUpdate, TargetActorKey), AActor::StaticClass());
}

void USFBTService_SenseUpdate::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBAsset);
	}
}

void USFBTService_SenseUpdate::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AI = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	ASFCharacterBase* Self = SFBTHelpers::GetControlledCharacter(AI);
	if (!AI || !BB || !Self || TargetActorKey.SelectedKeyName == NAME_None)
	{
		return;
	}

	UAIPerceptionComponent* Perception = AI->GetPerceptionComponent();
	if (!Perception)
	{
		return;
	}

	TArray<AActor*> Perceived;
	Perception->GetCurrentlyPerceivedActors(/*SenseToUse*/ nullptr, Perceived);

	const FVector SelfLoc = Self->GetActorLocation();
	AActor* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	for (AActor* Candidate : Perceived)
	{
		if (!Candidate || Candidate == Self)
		{
			continue;
		}
		const ASFCharacterBase* CandidateChar = Cast<ASFCharacterBase>(Candidate);
		if (CandidateChar && CandidateChar->IsDead())
		{
			continue;
		}
		if (!SFBTHelpers::IsHostile(Self, Candidate))
		{
			continue;
		}
		if (bRequireLineOfSight && CandidateChar && !SFBTHelpers::HasLineOfSight(Self, Candidate))
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(SelfLoc, Candidate->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Candidate;
		}
	}

	if (!Best)
	{
		return;
	}

	AActor* Current = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!Current)
	{
		BB->SetValueAsObject(TargetActorKey.SelectedKeyName, Best);
		return;
	}

	if (bPreferCloserTargets && Current != Best)
	{
		const float CurrentDistSq = FVector::DistSquared(SelfLoc, Current->GetActorLocation());
		if (BestDistSq < CurrentDistSq)
		{
			BB->SetValueAsObject(TargetActorKey.SelectedKeyName, Best);
		}
	}
}

FString USFBTService_SenseUpdate::GetStaticDescription() const
{
	return FString::Printf(TEXT("Sense Update every %.2fs%s%s"),
		Interval,
		bRequireLineOfSight ? TEXT(" (LOS)") : TEXT(""),
		bPreferCloserTargets ? TEXT(" prefer closer") : TEXT(""));
}
