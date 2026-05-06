#include "AI/BTTask_ValidateOrderTarget.h"

#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/SFCharacterBase.h"

UBTTask_ValidateOrderTarget::UBTTask_ValidateOrderTarget()
{
	NodeName = TEXT("Validate Order Target");
}

EBTNodeResult::Type UBTTask_ValidateOrderTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB)
	{
		return EBTNodeResult::Failed;
	}

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(OrderTargetActorKey));
	if (!IsValid(Target))
	{
		return EBTNodeResult::Failed;
	}

	if (bRejectDead)
	{
		if (const ASFCharacterBase* Char = Cast<ASFCharacterBase>(Target))
		{
			if (Char->IsDead())
			{
				return EBTNodeResult::Failed;
			}
		}
	}

	return EBTNodeResult::Succeeded;
}
