#include "AI/BTTask_ConsumeOrder.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Companions/SFCompanionTypes.h"

UBTTask_ConsumeOrder::UBTTask_ConsumeOrder()
{
	NodeName = TEXT("Consume Order");
}

EBTNodeResult::Type UBTTask_ConsumeOrder::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB)
	{
		return EBTNodeResult::Failed;
	}

	const ESFCompanionOrderType Type = static_cast<ESFCompanionOrderType>(BB->GetValueAsEnum(OrderTypeKey));
	if (Type == ESFCompanionOrderType::None)
	{
		return EBTNodeResult::Failed;
	}

	// SyncCompanionTactics already wrote OrderSequence; we re-stamp it here so
	// downstream decorators that watch this key see "handled" semantics.
	const int32 Sequence = BB->GetValueAsInt(OrderSequenceKey);
	BB->SetValueAsInt(OrderSequenceKey, Sequence);

	return EBTNodeResult::Succeeded;
}
