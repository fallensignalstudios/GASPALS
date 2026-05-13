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

	// Sight: only fire perception events for actors the faction system
	// considers hostile to us. Friendlies / neutrals are ignored at the
	// engine layer so the BT never has to filter them.
	if (UAISenseConfig_Sight* Sight = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig")))
	{
		Sight->SightRadius = 1500.0f;
		Sight->LoseSightRadius = 1800.0f;
		Sight->PeripheralVisionAngleDegrees = 70.0f;
		Sight->DetectionByAffiliation.bDetectEnemies = true;
		Sight->DetectionByAffiliation.bDetectFriendlies = false;
		Sight->DetectionByAffiliation.bDetectNeutrals = false;
		Perception->ConfigureSense(*Sight);
		Perception->SetDominantSense(Sight->GetSenseImplementation());
	}

	// Hearing: still pick up neutrals (someone breaking nearby) but never
	// chase allies on hearing alone. Designers can flip this in BPs.
	if (UAISenseConfig_Hearing* Hearing = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig")))
	{
		Hearing->HearingRange = 1200.0f;
		Hearing->DetectionByAffiliation.bDetectEnemies = true;
		Hearing->DetectionByAffiliation.bDetectFriendlies = false;
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
