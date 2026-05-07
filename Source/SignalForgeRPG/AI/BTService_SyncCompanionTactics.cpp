#include "AI/BTService_SyncCompanionTactics.h"

#include "AI/SFCompanionAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Companions/SFCompanionCharacter.h"
#include "Companions/SFCompanionTacticsComponent.h"
#include "Kismet/GameplayStatics.h"

UBTService_SyncCompanionTactics::UBTService_SyncCompanionTactics()
{
	NodeName = TEXT("Sync Companion Tactics");
	bNotifyBecomeRelevant = true;
	bNotifyTick = true;
	Interval = 0.15f;
	RandomDeviation = 0.02f;
}

void UBTService_SyncCompanionTactics::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	ASFCompanionAIController* AIC = Cast<ASFCompanionAIController>(OwnerComp.GetAIOwner());
	if (!BB || !AIC)
	{
		return;
	}

	ASFCompanionCharacter* Companion = AIC->GetControlledCompanion();
	if (!Companion)
	{
		return;
	}

	USFCompanionTacticsComponent* Tactics = Companion->GetTactics();
	if (!Tactics)
	{
		return;
	}

	// --- Stance / aggression / engagement range ----------------------------
	BB->SetValueAsEnum(StanceKey, static_cast<uint8>(Tactics->GetStance()));
	BB->SetValueAsEnum(AggressionKey, static_cast<uint8>(Tactics->GetAggression()));
	BB->SetValueAsEnum(EngagementRangeKey, static_cast<uint8>(Tactics->GetEngagementRange()));

	const FGameplayTagContainer& StanceTags = Tactics->GetStanceTags();
	BB->SetValueAsName(StanceTagKey, StanceTags.Num() > 0 ? StanceTags.First().GetTagName() : NAME_None);

	// --- Active order ------------------------------------------------------
	const FSFCompanionOrder& Order = Tactics->GetActiveOrder();
	BB->SetValueAsEnum(OrderTypeKey, static_cast<uint8>(Order.Type));
	BB->SetValueAsInt(OrderSequenceKey, Order.Sequence);
	BB->SetValueAsObject(OrderTargetActorKey, Order.TargetActor.Get());
	BB->SetValueAsVector(OrderTargetLocationKey, Order.TargetLocation);
	BB->SetValueAsName(OrderAbilityTagKey, Order.AbilityTag.GetTagName());
	BB->SetValueAsBool(HasValidOrderKey, Order.IsValidOrder());

	// --- Threshold flags ---------------------------------------------------
	BB->SetValueAsBool(SelfLowHealthKey, Tactics->IsSelfLowHealth());
	BB->SetValueAsBool(PlayerLowHealthKey, Tactics->IsPlayerLowHealth());
	BB->SetValueAsBool(CanUseCommandedAbilityKey, Tactics->CanCommandAbility());

	// --- Player resolution -------------------------------------------------
	// OnPossess seeds PlayerActor once, but the player pawn may not exist yet
	// when companions are placed in the level. If the key is null, keep
	// trying every tick until we find a player pawn.
	if (bAutoResolvePlayer)
	{
		UObject* CurrentPlayer = BB->GetValueAsObject(PlayerActorKey);
		if (!CurrentPlayer)
		{
			if (APawn* Player = UGameplayStatics::GetPlayerPawn(Companion, 0))
			{
				BB->SetValueAsObject(PlayerActorKey, Player);
			}
		}
	}
}
