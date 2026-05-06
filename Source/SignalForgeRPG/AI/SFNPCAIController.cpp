#include "AI/SFNPCAIController.h"
#include "Characters/SFNPCBase.h"
#include "Characters/SFNPCNarrativeIdentityComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"

ASFNPCAIController::ASFNPCAIController()
{
	BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
	BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));

	Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));

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

	if (UAISenseConfig_Hearing* Hearing = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig")))
	{
		Hearing->HearingRange = 1200.0f;
		Hearing->DetectionByAffiliation.bDetectEnemies = true;
		Hearing->DetectionByAffiliation.bDetectFriendlies = true;
		Hearing->DetectionByAffiliation.bDetectNeutrals = true;
		Perception->ConfigureSense(*Hearing);
	}
}

void ASFNPCAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledNPC = Cast<ASFNPCBase>(InPawn);

	if (Perception && !Perception->OnTargetPerceptionUpdated.IsAlreadyBound(this, &ASFNPCAIController::HandlePerceptionUpdated))
	{
		Perception->OnTargetPerceptionUpdated.AddDynamic(this, &ASFNPCAIController::HandlePerceptionUpdated);
	}

	if (DefaultBehaviorTree && BehaviorTreeComponent && BlackboardComponent)
	{
		if (UBlackboardData* BlackboardAsset = DefaultBehaviorTree->BlackboardAsset)
		{
			UseBlackboard(BlackboardAsset, BlackboardComponent);
		}
		BehaviorTreeComponent->StartTree(*DefaultBehaviorTree);
	}
}

void ASFNPCAIController::OnUnPossess()
{
	if (BehaviorTreeComponent)
	{
		BehaviorTreeComponent->StopTree();
	}

	if (Perception && Perception->OnTargetPerceptionUpdated.IsAlreadyBound(this, &ASFNPCAIController::HandlePerceptionUpdated))
	{
		Perception->OnTargetPerceptionUpdated.RemoveDynamic(this, &ASFNPCAIController::HandlePerceptionUpdated);
	}

	ControlledNPC = nullptr;

	Super::OnUnPossess();
}

void ASFNPCAIController::HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Stimulus.WasSuccessfullySensed() || !ControlledNPC)
	{
		return;
	}

	// Hostile-by-default NPCs flip immediately on sight; civilians could be
	// extended here to flag fear, set blackboard targets, etc. Disposition
	// changes are server-only; clients receive via the identity component's
	// OnRep.
	if (USFNPCNarrativeIdentityComponent* Identity = ControlledNPC->FindComponentByClass<USFNPCNarrativeIdentityComponent>())
	{
		if (Identity->IsHostileByDefault() && Identity->GetDisposition() != ESFNPCDisposition::Hostile)
		{
			Identity->SetDisposition(ESFNPCDisposition::Hostile);
		}
	}

	// Cache the latest perceived hostile actor on the blackboard so
	// designer-authored BTs can react. The "TargetActor" key is a convention;
	// designers can change it in the BB asset if they prefer.
	if (BlackboardComponent && Actor)
	{
		BlackboardComponent->SetValueAsObject(TEXT("TargetActor"), Actor);
	}
}
