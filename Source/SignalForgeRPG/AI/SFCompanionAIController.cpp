#include "AI/SFCompanionAIController.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Companions/SFCompanionCharacter.h"
#include "Companions/SFCompanionTacticsComponent.h"
#include "Kismet/GameplayStatics.h"

namespace SFCompanionBlackboardKeys
{
	static const FName PlayerActor      = TEXT("PlayerActor");
	static const FName OrderType        = TEXT("OrderType");
	static const FName OrderTargetActor = TEXT("OrderTargetActor");
	static const FName OrderTargetLoc   = TEXT("OrderTargetLoc");
	static const FName OrderAbilityTag  = TEXT("OrderAbilityTag");
	static const FName StanceTag        = TEXT("StanceTag");
}

ASFCompanionAIController::ASFCompanionAIController()
{
	// Inherits perception + BT/blackboard stack from ASFNPCAIController.
}

void ASFCompanionAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledCompanion = Cast<ASFCompanionCharacter>(InPawn);
	if (!ControlledCompanion)
	{
		return;
	}

	if (USFCompanionTacticsComponent* Tactics = ControlledCompanion->GetTactics())
	{
		Tactics->OnStanceChanged.AddDynamic(this, &ASFCompanionAIController::HandleStanceChanged);
		Tactics->OnOrderIssued.AddDynamic(this, &ASFCompanionAIController::HandleOrderIssued);

		PushStanceToBlackboard();
		PushOrderToBlackboard(Tactics->GetActiveOrder());
	}

	PushPlayerToBlackboard();
}

void ASFCompanionAIController::OnUnPossess()
{
	if (ControlledCompanion)
	{
		if (USFCompanionTacticsComponent* Tactics = ControlledCompanion->GetTactics())
		{
			Tactics->OnStanceChanged.RemoveDynamic(this, &ASFCompanionAIController::HandleStanceChanged);
			Tactics->OnOrderIssued.RemoveDynamic(this, &ASFCompanionAIController::HandleOrderIssued);
		}
	}

	ControlledCompanion = nullptr;

	Super::OnUnPossess();
}

void ASFCompanionAIController::HandleStanceChanged(ESFCompanionStance /*OldStance*/, ESFCompanionStance /*NewStance*/)
{
	PushStanceToBlackboard();
}

void ASFCompanionAIController::HandleOrderIssued(const FSFCompanionOrder& NewOrder)
{
	PushOrderToBlackboard(NewOrder);
}

void ASFCompanionAIController::PushOrderToBlackboard(const FSFCompanionOrder& Order)
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB)
	{
		return;
	}

	BB->SetValueAsEnum(SFCompanionBlackboardKeys::OrderType, static_cast<uint8>(Order.Type));
	BB->SetValueAsObject(SFCompanionBlackboardKeys::OrderTargetActor, Order.TargetActor.Get());
	BB->SetValueAsVector(SFCompanionBlackboardKeys::OrderTargetLoc, Order.TargetLocation);
	BB->SetValueAsName(SFCompanionBlackboardKeys::OrderAbilityTag, Order.AbilityTag.GetTagName());
}

void ASFCompanionAIController::PushStanceToBlackboard()
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB || !ControlledCompanion)
	{
		return;
	}

	if (USFCompanionTacticsComponent* Tactics = ControlledCompanion->GetTactics())
	{
		const FGameplayTagContainer& StanceTags = Tactics->GetStanceTags();
		FName FirstTagName = NAME_None;
		if (StanceTags.Num() > 0)
		{
			FirstTagName = StanceTags.First().GetTagName();
		}
		BB->SetValueAsName(SFCompanionBlackboardKeys::StanceTag, FirstTagName);
	}
}

void ASFCompanionAIController::PushPlayerToBlackboard()
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB)
	{
		return;
	}

	// Single-player assumption (per user scope decision): bind to player 0.
	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		BB->SetValueAsObject(SFCompanionBlackboardKeys::PlayerActor, PlayerPawn);
	}
}
