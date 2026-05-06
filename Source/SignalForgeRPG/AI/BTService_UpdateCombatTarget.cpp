#include "AI/BTService_UpdateCombatTarget.h"

#include "AIController.h"
#include "AI/SFCompanionAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Companions/SFCompanionCharacter.h"
#include "Companions/SFCompanionTacticsComponent.h"
#include "Companions/SFCompanionTypes.h"
#include "GameFramework/Pawn.h"
#include "GenericTeamAgentInterface.h"
#include "Perception/AIPerceptionComponent.h"

UBTService_UpdateCombatTarget::UBTService_UpdateCombatTarget()
{
	NodeName = TEXT("Update Combat Target");
	bNotifyBecomeRelevant = true;
	bNotifyTick = true;
	Interval = 0.25f;
	RandomDeviation = 0.05f;
}

namespace
{
	bool IsHostileTo(const AAIController* SelfController, const AActor* Self, AActor* Candidate)
	{
		if (!Candidate || Self == Candidate)
		{
			return false;
		}

		// Prefer the controller's team interface (AAIController implements it),
		// fall back to the pawn if the controller is missing/unset.
		const IGenericTeamAgentInterface* SelfTeam =
			SelfController ? Cast<const IGenericTeamAgentInterface>(SelfController) : nullptr;
		if (!SelfTeam && Self)
		{
			SelfTeam = Cast<const IGenericTeamAgentInterface>(Self);
		}
		if (!SelfTeam)
		{
			// No team interface -> can't classify; treat as not hostile to avoid
			// false positives. Designers can swap this out if needed.
			return false;
		}

		return SelfTeam->GetTeamAttitudeTowards(*Candidate) == ETeamAttitude::Hostile;
	}
}

void UBTService_UpdateCombatTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
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

	// FocusFire holds the focus regardless of perception until the order ends.
	const ESFCompanionOrderType OrderType = static_cast<ESFCompanionOrderType>(BB->GetValueAsEnum(OrderTypeKey));
	if (OrderType == ESFCompanionOrderType::FocusFire || OrderType == ESFCompanionOrderType::AttackTarget)
	{
		// Order branch already wrote OrderTargetActor -> FocusTarget; don't fight it.
		return;
	}

	USFCompanionTacticsComponent* Tactics = Companion->GetTactics();
	if (!Tactics)
	{
		return;
	}

	const ESFCompanionAggression Aggression = Tactics->GetAggression();

	// Drop dead / invalid focus immediately so Combat branch can re-pick.
	AActor* Current = Cast<AActor>(BB->GetValueAsObject(FocusTargetKey));
	if (Current && (!IsValid(Current) || !IsHostileTo(AIC, Companion, Current)))
	{
		Current = nullptr;
		BB->SetValueAsObject(FocusTargetKey, nullptr);
	}

	if (Aggression == ESFCompanionAggression::Passive)
	{
		// Passive never proactively picks. Reactive targeting (return-fire) is
		// expected to be set by a damage-taken hook elsewhere.
		return;
	}

	UAIPerceptionComponent* Perception = AIC->GetPerceptionComponent();
	if (!Perception)
	{
		return;
	}

	TArray<AActor*> Perceived;
	Perception->GetCurrentlyPerceivedActors(/*SenseToUse=*/nullptr, Perceived);
	if (Perceived.Num() == 0)
	{
		return;
	}

	APawn* Player = Cast<APawn>(BB->GetValueAsObject(PlayerActorKey));
	const FVector CompanionLoc = Companion->GetActorLocation();
	const FVector PlayerLoc = Player ? Player->GetActorLocation() : CompanionLoc;
	const float DefRadiusSq = DefensivePlayerThreatRadius * DefensivePlayerThreatRadius;
	const float AggRangeSq = AggressiveEngageRange * AggressiveEngageRange;

	AActor* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	for (AActor* Candidate : Perceived)
	{
		if (!IsHostileTo(AIC, Companion, Candidate))
		{
			continue;
		}

		const float DistToSelfSq = FVector::DistSquared(CompanionLoc, Candidate->GetActorLocation());

		bool bConsider = false;
		if (Aggression == ESFCompanionAggression::Aggressive)
		{
			bConsider = DistToSelfSq <= AggRangeSq;
		}
		else // Defensive
		{
			const float DistToPlayerSq = FVector::DistSquared(PlayerLoc, Candidate->GetActorLocation());
			bConsider = (DistToPlayerSq <= DefRadiusSq) || (DistToSelfSq <= DefRadiusSq);
		}

		if (bConsider && DistToSelfSq < BestDistSq)
		{
			BestDistSq = DistToSelfSq;
			Best = Candidate;
		}
	}

	if (Best)
	{
		BB->SetValueAsObject(FocusTargetKey, Best);
	}
	// If we found nothing this tick, leave the previous (validated) focus alone.
}
