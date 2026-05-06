#include "AI/BTService_UpdateTargetAndRange.h"
#include "AI/SFEnemyAIController.h"
#include "Characters/SFEnemyCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

UBTService_UpdateTargetAndRange::UBTService_UpdateTargetAndRange()
{
	NodeName = TEXT("Update Target And Range");
	bNotifyBecomeRelevant = true;
	bNotifyTick = true;
	Interval = 0.2f;
	RandomDeviation = 0.0f;
}

void UBTService_UpdateTargetAndRange::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	ASFEnemyAIController* AIController = Cast<ASFEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIController)
	{
		return;
	}

	ASFEnemyCharacter* Enemy = AIController->GetControlledEnemy();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

	if (!Enemy || !Blackboard || Enemy->IsDead())
	{
		if (Blackboard)
		{
			Blackboard->SetValueAsObject(TargetActorKey, nullptr);
			Blackboard->SetValueAsBool(IsInAttackRangeKey, false);
		}
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(Enemy, 0);
	if (!PlayerPawn)
	{
		Blackboard->SetValueAsObject(TargetActorKey, nullptr);
		Blackboard->SetValueAsBool(IsInAttackRangeKey, false);
		return;
	}

	const float Distance = FVector::Dist(Enemy->GetActorLocation(), PlayerPawn->GetActorLocation());
	const bool bInDetectionRange = Distance <= Enemy->GetDetectionRange();
	const bool bInAttackRange = Distance <= Enemy->GetAttackRange();

	Blackboard->SetValueAsObject(TargetActorKey, bInDetectionRange ? PlayerPawn : nullptr);
	Blackboard->SetValueAsBool(IsInAttackRangeKey, bInDetectionRange && bInAttackRange);
}