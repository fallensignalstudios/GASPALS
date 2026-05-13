#include "AI/BehaviorTree/SFBTTask_FindHostileInRadius.h"

#include "AI/BehaviorTree/SFBTHelpers.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/SFCharacterBase.h"
#include "Engine/World.h"
#include "EngineUtils.h"

USFBTTask_FindHostileInRadius::USFBTTask_FindHostileInRadius()
{
	NodeName = TEXT("SF Find Hostile In Radius");
}

EBTNodeResult::Type USFBTTask_FindHostileInRadius::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AI = OwnerComp.GetAIOwner();
	ASFCharacterBase* Self = SFBTHelpers::GetControlledCharacter(AI);
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!Self || !BB)
	{
		return EBTNodeResult::Failed;
	}

	UWorld* World = Self->GetWorld();
	if (!World)
	{
		return EBTNodeResult::Failed;
	}

	const FVector SelfLoc = Self->GetActorLocation();
	const float RadiusSq = Radius * Radius;

	ASFCharacterBase* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	for (TActorIterator<ASFCharacterBase> It(World); It; ++It)
	{
		ASFCharacterBase* Candidate = *It;
		if (!Candidate || Candidate == Self || Candidate->IsDead())
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(SelfLoc, Candidate->GetActorLocation());
		if (DistSq > RadiusSq || DistSq >= BestDistSq)
		{
			continue;
		}
		if (!SFBTHelpers::IsHostile(Self, Candidate))
		{
			continue;
		}
		if (bRequireLineOfSight && !SFBTHelpers::HasLineOfSight(Self, Candidate))
		{
			continue;
		}
		Best = Candidate;
		BestDistSq = DistSq;
	}

	if (Best)
	{
		BB->SetValueAsObject(BlackboardKey.SelectedKeyName, Best);
		return EBTNodeResult::Succeeded;
	}

	if (bClearOnFail)
	{
		BB->ClearValue(BlackboardKey.SelectedKeyName);
	}
	return EBTNodeResult::Failed;
}

FString USFBTTask_FindHostileInRadius::GetStaticDescription() const
{
	return FString::Printf(TEXT("Find Hostile In %.0fcm -> %s%s"),
		Radius,
		*BlackboardKey.SelectedKeyName.ToString(),
		bRequireLineOfSight ? TEXT(" (LOS)") : TEXT(""));
}
