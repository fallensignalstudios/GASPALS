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

	if (!InPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SFAI] OnPossess: InPawn is null on %s -- BT will not start."), *GetNameSafe(this));
		return;
	}

	ControlledEnemy = Cast<ASFEnemyCharacter>(InPawn);
	if (!ControlledEnemy)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[SFAI] OnPossess: pawn '%s' is class '%s' which is NOT an ASFEnemyCharacter. "
			     "Reparent the BP to BP_EnemyCharacter (or ASFEnemyCharacter) so the controller can fetch its BehaviorTreeAsset."),
			*GetNameSafe(InPawn), *GetNameSafe(InPawn->GetClass()));
		return;
	}

	// Bind perception updates so we can route hostile sightings into the BT.
	if (Perception && !Perception->OnTargetPerceptionUpdated.IsAlreadyBound(this, &ASFEnemyAIController::HandlePerceptionUpdated))
	{
		Perception->OnTargetPerceptionUpdated.AddDynamic(this, &ASFEnemyAIController::HandlePerceptionUpdated);
	}

	UBehaviorTree* BehaviorTree = ControlledEnemy->GetBehaviorTreeAsset();
	if (!BehaviorTree)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[SFAI] OnPossess: pawn '%s' has no BehaviorTreeAsset assigned. "
			     "Open the BP, find Class Defaults -> AI -> Behavior Tree Asset and assign BT_NPC (or your tree)."),
			*GetNameSafe(InPawn));
		return;
	}
	if (!BehaviorTree->BlackboardAsset)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[SFAI] OnPossess: BehaviorTree '%s' has no BlackboardAsset set. "
			     "Open BT_NPC and assign BB_NPC in the asset details."),
			*GetNameSafe(BehaviorTree));
		return;
	}

	UBlackboardComponent* BlackboardComp = nullptr;
	if (!UseBlackboard(BehaviorTree->BlackboardAsset, BlackboardComp))
	{
		UE_LOG(LogTemp, Warning, TEXT("[SFAI] OnPossess: UseBlackboard failed for '%s'."), *GetNameSafe(BehaviorTree->BlackboardAsset));
		return;
	}

	BlackboardComponent = BlackboardComp;

	if (!RunBehaviorTree(BehaviorTree))
	{
		UE_LOG(LogTemp, Warning, TEXT("[SFAI] OnPossess: RunBehaviorTree failed for '%s'."), *GetNameSafe(BehaviorTree));
		return;
	}

	// Anchor patrol / return-home behavior at the pawn's spawn point. The
	// SFBTService_PatrolPoint reads this key to pick reachable patrol points.
	if (BlackboardComponent && !HomeLocationKeyName.IsNone())
	{
		BlackboardComponent->SetValueAsVector(HomeLocationKeyName, InPawn->GetActorLocation());
	}

	UE_LOG(LogTemp, Log,
		TEXT("[SFAI] OnPossess SUCCESS: pawn '%s' running BT '%s' with BB '%s'. HomeLocation = %s."),
		*GetNameSafe(InPawn), *GetNameSafe(BehaviorTree), *GetNameSafe(BehaviorTree->BlackboardAsset),
		*InPawn->GetActorLocation().ToString());
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
