#include "AI/SFNPCAIController.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Characters/SFCharacterBase.h"
#include "Characters/SFNPCBase.h"
#include "Characters/SFNPCNarrativeIdentityComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "Faction/SFFactionStatics.h"
#include "GameplayTagContainer.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Core/SignalForgeLogChannels.h"

ASFNPCAIController::ASFNPCAIController()
{
	BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
	BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));

	Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));

	// Sight: detect everyone at the engine layer. We deliberately do NOT rely on
	// UE's affiliation flags (GenericTeamAgentInterface team IDs) -- our faction
	// system is the source of truth. HandlePerceptionUpdated re-filters every
	// stimulus through USFFactionStatics::AreHostile so faction relationships
	// drive behavior, never per-character flags.
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

void ASFNPCAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Drain the possessed pawn's ASC input queue from the controller's tick.
	// AI BT tasks push presses via AbilityInputTagPressed; without this drain
	// they sit in the queue forever and TryActivateAbility never fires. We
	// run the drain here (instead of relying on the pawn's Tick) because
	// several AI character BP archetypes cooked PrimaryActorTick.bCanEverTick
	// to false, leaving the pawn tick-dormant and silently breaking AI
	// ability activation. AIControllers always tick (the BT depends on it),
	// so this is the reliable place to drive the drain for NPCs.
	if (ASFCharacterBase* SFChar = Cast<ASFCharacterBase>(GetPawn()))
	{
		if (USFAbilitySystemComponent* SFASC = Cast<USFAbilitySystemComponent>(SFChar->GetAbilitySystemComponent()))
		{
			const bool bGamePaused = GetWorld() && GetWorld()->IsPaused();
			SFASC->ProcessAbilityInput(DeltaTime, bGamePaused);
		}
	}
}

void ASFNPCAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!InPawn)
	{
		UE_LOG(LogSFAI, Warning, TEXT("[SFNPCAI] OnPossess: InPawn is null on %s -- BT will not start."),
			*GetNameSafe(this));
		return;
	}

	ControlledNPC = Cast<ASFNPCBase>(InPawn);

	if (Perception && !Perception->OnTargetPerceptionUpdated.IsAlreadyBound(this, &ASFNPCAIController::HandlePerceptionUpdated))
	{
		Perception->OnTargetPerceptionUpdated.AddDynamic(this, &ASFNPCAIController::HandlePerceptionUpdated);
	}

	if (DefaultBehaviorTree && BehaviorTreeComponent && BlackboardComponent)
	{
		if (UBlackboardData* BlackboardAsset = DefaultBehaviorTree->BlackboardAsset)
		{
			// AAIController::UseBlackboard takes UBlackboardComponent*& (non-const ref)
			// so we can't pass the TObjectPtr member directly.
			UBlackboardComponent* BlackboardCompPtr = BlackboardComponent;
			UseBlackboard(BlackboardAsset, BlackboardCompPtr);
		}
		BehaviorTreeComponent->StartTree(*DefaultBehaviorTree);
	}
	else
	{
		// Loud failure path: if any of these are null the BT never runs and the
		// controller silently does nothing. Surface exactly which piece is
		// missing so the user can fix the BP / controller defaults.
		UE_LOG(LogSFAI, Warning,
			TEXT("[SFNPCAI] OnPossess: BT NOT STARTED on pawn='%s'. "
			     "DefaultBehaviorTree=%s, BehaviorTreeComponent=%s, BlackboardComponent=%s. "
			     "Fix: open this controller's BP and set 'Default Behavior Tree' in Class Defaults."),
			*GetNameSafe(InPawn),
			DefaultBehaviorTree ? TEXT("set") : TEXT("NULL"),
			BehaviorTreeComponent ? TEXT("valid") : TEXT("NULL"),
			BlackboardComponent ? TEXT("valid") : TEXT("NULL"));
	}

	// Anchor patrol / return-home behavior at the pawn's spawn point. Without this
	// the patrol service has nothing to query and the alerted branch's MoveTo /
	// LookAt targets stay invalid.
	if (BlackboardComponent && !HomeLocationKeyName.IsNone())
	{
		BlackboardComponent->SetValueAsVector(HomeLocationKeyName, InPawn->GetActorLocation());
	}

	// Display verbosity so this never gets filtered out by category log levels.
	UE_LOG(LogSFAI, Display,
		TEXT("[SFNPCAI] OnPossess: pawn='%s' faction='%s' BT='%s' BB='%s' HomeLoc=%s"),
		*GetNameSafe(InPawn),
		*USFFactionStatics::GetFactionTag(InPawn).ToString(),
		*GetNameSafe(DefaultBehaviorTree),
		DefaultBehaviorTree ? *GetNameSafe(DefaultBehaviorTree->BlackboardAsset) : TEXT("<no BT>"),
		*InPawn->GetActorLocation().ToString());

	// Force-acquire any pawn already inside our sight cone at possess time --
	// UE's perception system only fires OnTargetPerceptionUpdated on STATE
	// CHANGES, so a player who was already standing in front of us when we
	// spawned never triggers a callback. Walk the currently-sensed list once
	// after possess so they get routed through HandlePerceptionUpdated.
	PrimeFromCurrentPerception();
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
	if (!ControlledNPC || !Actor)
	{
		return;
	}

	// Per-actor edge-triggered logging so we get one diagnostic line per state
	// change instead of spamming the log every perception tick.
	const uint32 ActorKey = Actor->GetUniqueID();
	uint8& LastLogState = PerceptionLogStatePerActor.FindOrAdd(ActorKey);

	if (!Stimulus.WasSuccessfullySensed())
	{
		// Lost sight / sound -- reset state so the next acquisition logs again.
		LastLogState = 0;
		return;
	}

	// Hostile-by-default disposition flip (legacy civilian/quest-NPC behavior).
	// This is kept so disposition-aware dialogue still works for NPCs that opt
	// in via bHostileByDefault -- it does NOT decide who the BT attacks.
	if (USFNPCNarrativeIdentityComponent* Identity = ControlledNPC->FindComponentByClass<USFNPCNarrativeIdentityComponent>())
	{
		if (Identity->IsHostileByDefault() && Identity->GetDisposition() != ESFNPCDisposition::Hostile)
		{
			Identity->SetDisposition(ESFNPCDisposition::Hostile);
		}
	}

	// Faction system is the source of truth for combat hostility. Per the design
	// (dual-protagonist, faction-driven enemies), a single NPC controller routes
	// every pawn through AreHostile -- the same actor can be friend or foe across
	// playthroughs without touching the BT or character class.
	const bool bHostile = USFFactionStatics::AreHostile(ControlledNPC, Actor);
	const uint8 NewLogState = bHostile ? 2 : 1;

	if (!bHostile)
	{
		if (LastLogState != NewLogState)
		{
			const FGameplayTag FromTag = USFFactionStatics::GetFactionTag(ControlledNPC);
			const FGameplayTag ToTag = USFFactionStatics::GetFactionTag(Actor);
			UE_LOG(LogSFAI, Warning,
				TEXT("[SFNPCAI Perception] '%s' SAW '%s' but faction system says NOT hostile -- TargetActor not written. "
				     "FromFaction='%s' ToFaction='%s'. "
				     "Check: (1) both actors have a USFFactionComponent with a Faction.* tag set, "
				     "(2) DeveloperSettings -> SignalForge -> DefaultFactionRelationships asset is assigned, "
				     "(3) the relationship asset has a row for FromFaction with a Hostile entry toward ToFaction."),
				*GetNameSafe(ControlledNPC), *GetNameSafe(Actor),
				*FromTag.ToString(), *ToTag.ToString());
			LastLogState = NewLogState;
		}
		return;
	}

	if (BlackboardComponent && !TargetActorKeyName.IsNone())
	{
		BlackboardComponent->SetValueAsObject(TargetActorKeyName, Actor);
		if (LastLogState != NewLogState)
		{
			const bool bBTRunning = BehaviorTreeComponent && BehaviorTreeComponent->IsRunning();
			UE_LOG(LogSFAI, Display,
				TEXT("[SFNPCAI Perception] '%s' acquired hostile target '%s' (key '%s'). BT running=%s, BT asset='%s'"),
				*GetNameSafe(ControlledNPC), *GetNameSafe(Actor), *TargetActorKeyName.ToString(),
				bBTRunning ? TEXT("YES") : TEXT("NO -- pawn will not react"),
				*GetNameSafe(DefaultBehaviorTree));
			LastLogState = NewLogState;
		}
	}
	else if (LastLogState != NewLogState)
	{
		UE_LOG(LogSFAI, Warning,
			TEXT("[SFNPCAI Perception] '%s' saw hostile '%s' but cannot write TargetActor: "
			     "BlackboardComponent=%s, TargetActorKeyName='%s'."),
			*GetNameSafe(ControlledNPC), *GetNameSafe(Actor),
			BlackboardComponent ? TEXT("valid") : TEXT("NULL"),
			*TargetActorKeyName.ToString());
		LastLogState = NewLogState;
	}
}

void ASFNPCAIController::PrimeFromCurrentPerception()
{
	if (!Perception || !ControlledNPC)
	{
		return;
	}

	TArray<AActor*> AlreadyPerceived;
	Perception->GetCurrentlyPerceivedActors(/*SenseToUse*/ nullptr, AlreadyPerceived);

	UE_LOG(LogSFAI, Log,
		TEXT("[SFNPCAI] PrimeFromCurrentPerception: %d actor(s) already in perception on possess."),
		AlreadyPerceived.Num());

	for (AActor* Sensed : AlreadyPerceived)
	{
		if (!Sensed || Sensed == ControlledNPC)
		{
			continue;
		}

		// Skip synthesizing a stimulus -- the perception system's currently-perceived
		// list already implies a successful sense. Run the faction check + BB write
		// path directly, mirroring the hostile-acquired branch of HandlePerceptionUpdated.
		const uint32 ActorKey = Sensed->GetUniqueID();
		uint8& LastLogState = PerceptionLogStatePerActor.FindOrAdd(ActorKey);

		const bool bHostile = USFFactionStatics::AreHostile(ControlledNPC, Sensed);
		const uint8 NewLogState = bHostile ? 2 : 1;

		if (!bHostile)
		{
			if (LastLogState != NewLogState)
			{
				const FGameplayTag FromTag = USFFactionStatics::GetFactionTag(ControlledNPC);
				const FGameplayTag ToTag = USFFactionStatics::GetFactionTag(Sensed);
				UE_LOG(LogSFAI, Warning,
					TEXT("[SFNPCAI Prime] '%s' already saw '%s' on possess but faction system says NOT hostile. "
					     "FromFaction='%s' ToFaction='%s'."),
					*GetNameSafe(ControlledNPC), *GetNameSafe(Sensed),
					*FromTag.ToString(), *ToTag.ToString());
				LastLogState = NewLogState;
			}
			continue;
		}

		if (BlackboardComponent && !TargetActorKeyName.IsNone())
		{
			BlackboardComponent->SetValueAsObject(TargetActorKeyName, Sensed);
			if (LastLogState != NewLogState)
			{
				UE_LOG(LogSFAI, Log,
					TEXT("[SFNPCAI Prime] '%s' acquired hostile target '%s' on possess (key '%s')."),
					*GetNameSafe(ControlledNPC), *GetNameSafe(Sensed), *TargetActorKeyName.ToString());
				LastLogState = NewLogState;
			}
		}
	}
}
