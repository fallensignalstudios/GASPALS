#include "AI/BTTask_ClearActiveOrder.h"

#include "AI/SFCompanionAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Companions/SFCompanionCharacter.h"
#include "Companions/SFCompanionTacticsComponent.h"

UBTTask_ClearActiveOrder::UBTTask_ClearActiveOrder()
{
	NodeName = TEXT("Clear Active Order");
}

EBTNodeResult::Type UBTTask_ClearActiveOrder::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	ASFCompanionAIController* AIC = Cast<ASFCompanionAIController>(OwnerComp.GetAIOwner());
	if (!AIC)
	{
		return EBTNodeResult::Failed;
	}

	ASFCompanionCharacter* Companion = AIC->GetControlledCompanion();
	if (!Companion)
	{
		return EBTNodeResult::Failed;
	}

	USFCompanionTacticsComponent* Tactics = Companion->GetTactics();
	if (!Tactics)
	{
		return EBTNodeResult::Failed;
	}

	// ClearOrder is BlueprintAuthorityOnly; on clients this is a no-op but the
	// next replication tick will flow the cleared order back through the BB.
	if (Companion->HasAuthority())
	{
		Tactics->ClearOrder();
	}

	return EBTNodeResult::Succeeded;
}
