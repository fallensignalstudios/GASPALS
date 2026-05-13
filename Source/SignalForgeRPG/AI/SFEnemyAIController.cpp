#include "AI/SFEnemyAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/SFEnemyCharacter.h"
#include "Faction/SFFactionStatics.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"

ASFEnemyAIController::ASFEnemyAIController()
{
	BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
	BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));

	Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));

	// Sight: detect everyone at the engine layer. We deliberately do NOT rely
	// on UE's affiliation flags (GenericTeamAgentInterface team IDs), because
	// our faction system is the source of truth -- HandlePerceptionUpdated
	// re-filters every stimulus through USFFactionStatics::AreHostile, and
	// SF Sense Update on the BT side does the same. Filtering here would
	// silently drop the player unless every BP correctly sets a team ID.
	if (UAISenseConfig_Sight* Sight = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig")))
	{
		Sight->SightRadius = 1500.0f;
		Sight->LoseSightRadius = 1800.0f;
		Sight->PeripheralVisionAngleDegrees = 70.0f;
		Sight->DetectionByAffiliation.bDetectEnemies = true;
		Sight->DetectionByAffiliation.bDetectFriendlies = true;
		Sight->DetectionByAffiliation.bDetectNeutrals = true;
		Perception->ConfigureSense(*Sight);
		Perception->SetDominantSense(Sight->GetSenseImplementation());
	}

	// Hearing: same policy -- detect everything, let the faction system decide.
	if (UAISenseConfig_Hearing* Hearing = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig")))
	{
		Hearing->HearingRange = 1200.0f;
		Hearing->DetectionByAffiliation.bDetectEnemies = true;
		Hearing->DetectionByAffiliation.bDetectFriendlies = true;
		Hearing->DetectionByAffiliation.bDetectNeutrals = true;
		Perception->ConfigureSense(*Hearing);
	}
}

void ASFEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledEnemy = Cast<ASFEnemyCharacter>(InPawn);
	if (!ControlledEnemy)
	{
		return;
	}

	// Bind perception updates so we can route hostile sightings into the BT.
	if (Perception && !Perception->OnTargetPerceptionUpdated.IsAlreadyBound(this, &ASFEnemyAIController::HandlePerceptionUpdated))
	{
		Perception->OnTargetPerceptionUpdated.AddDynamic(this, &ASFEnemyAIController::HandlePerceptionUpdated);
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

	// Anchor patrol / return-home behavior at the pawn's spawn point. The
	// SFBTService_PatrolPoint reads this key to pick reachable patrol points.
	if (BlackboardComponent && !HomeLocationKeyName.IsNone() && InPawn)
	{
		BlackboardComponent->SetValueAsVector(HomeLocationKeyName, InPawn->GetActorLocation());
	}
}

void ASFEnemyAIController::OnUnPossess()
{
	if (Perception && Perception->OnTargetPerceptionUpdated.IsAlreadyBound(this, &ASFEnemyAIController::HandlePerceptionUpdated))
	{
		Perception->OnTargetPerceptionUpdated.RemoveDynamic(this, &ASFEnemyAIController::HandlePerceptionUpdated);
	}

	ControlledEnemy = nullptr;

	Super::OnUnPossess();
}

void ASFEnemyAIController::HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Stimulus.WasSuccessfullySensed() || !ControlledEnemy || !Actor)
	{
		return;
	}

	// Double-check faction hostility at the controller layer. Perception
	// affiliation flags already filter at the engine level, but a designer
	// could relax those flags per-BP; this check keeps the contract solid.
	if (!USFFactionStatics::AreHostile(ControlledEnemy, Actor))
	{
		return;
	}

	if (BlackboardComponent && !TargetActorKeyName.IsNone())
	{
		BlackboardComponent->SetValueAsObject(TargetActorKeyName, Actor);
	}
}
