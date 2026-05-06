#include "AI/SFEnemyAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/SFEnemyCharacter.h"

ASFEnemyAIController::ASFEnemyAIController()
{
	BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
	BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
}

void ASFEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledEnemy = Cast<ASFEnemyCharacter>(InPawn);
	if (!ControlledEnemy)
	{
		return;
	}

	UBehaviorTree* BehaviorTree = ControlledEnemy->GetBehaviorTreeAsset();
	if (!BehaviorTree || !BehaviorTree->BlackboardAsset)
	{
		return;
	}

	UBlackboardComponent* BlackboardComp = nullptr;
	if (!UseBlackboard(BehaviorTree->BlackboardAsset, BlackboardComp))
	{
		return;
	}

	BlackboardComponent = BlackboardComp;

	if (!RunBehaviorTree(BehaviorTree))
	{
		return;
	}
}

void ASFEnemyAIController::OnUnPossess()
{
	ControlledEnemy = nullptr;

	Super::OnUnPossess();
}