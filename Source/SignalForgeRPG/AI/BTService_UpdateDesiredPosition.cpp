#include "AI/BTService_UpdateDesiredPosition.h"

#include "AI/SFCompanionAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Companions/SFCompanionCharacter.h"
#include "Companions/SFCompanionTacticsComponent.h"
#include "Companions/SFCompanionTypes.h"
#include "GameFramework/Pawn.h"

UBTService_UpdateDesiredPosition::UBTService_UpdateDesiredPosition()
{
	NodeName = TEXT("Update Desired Position");
	bNotifyBecomeRelevant = true;
	bNotifyTick = true;
	Interval = 0.2f;
	RandomDeviation = 0.03f;
}

void UBTService_UpdateDesiredPosition::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
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

	APawn* Player = Cast<APawn>(BB->GetValueAsObject(PlayerActorKey));
	if (!Player)
	{
		// Without a player anchor we can't compute a slot; leave previous value.
		return;
	}

	AActor* Focus = Cast<AActor>(BB->GetValueAsObject(FocusTargetKey));

	// --- Resolve spacing from EngagementRange -----------------------------
	float Spacing = SkirmishDistance;
	switch (Tactics->GetEngagementRange())
	{
		case ESFCompanionEngagementRange::StayClose: Spacing = StayCloseDistance; break;
		case ESFCompanionEngagementRange::Skirmish:  Spacing = SkirmishDistance;  break;
		case ESFCompanionEngagementRange::LongRange: Spacing = LongRangeDistance; break;
	}

	const FVector PlayerLoc = Player->GetActorLocation();
	const FVector PlayerForward = Player->GetActorForwardVector();
	const FVector PlayerRight = Player->GetActorRightVector();

	FVector Desired = PlayerLoc;
	float DesiredCombatDistance = Spacing;

	switch (Tactics->GetStance())
	{
		case ESFCompanionStance::Tank:
		{
			// Interpose between player and focus, otherwise step in front.
			if (Focus)
			{
				const FVector ToFocus = (Focus->GetActorLocation() - PlayerLoc).GetSafeNormal2D();
				Desired = PlayerLoc + ToFocus * Spacing;
			}
			else
			{
				Desired = PlayerLoc + PlayerForward * (Spacing * 0.5f);
			}
			DesiredCombatDistance = Spacing;
			break;
		}

		case ESFCompanionStance::DPS:
		{
			// Flank: lateral offset off the player->target line, at Spacing.
			if (Focus)
			{
				const FVector PlayerToFocus = (Focus->GetActorLocation() - PlayerLoc).GetSafeNormal2D();
				const FVector Lateral = FVector::CrossProduct(PlayerToFocus, FVector::UpVector).GetSafeNormal2D();
				// Pick the side that's closer to the companion right now to avoid yo-yo.
				const FVector CompanionLoc = Companion->GetActorLocation();
				const float Side = FVector::DotProduct(CompanionLoc - PlayerLoc, Lateral) >= 0.f ? 1.f : -1.f;
				Desired = Focus->GetActorLocation() - PlayerToFocus * Spacing + Lateral * (Side * DPSFlankOffset);
			}
			else
			{
				Desired = PlayerLoc - PlayerForward * (Spacing * 0.4f) + PlayerRight * DPSFlankOffset;
			}
			DesiredCombatDistance = Spacing;
			break;
		}

		case ESFCompanionStance::Healer:
		{
			// Stay near the player's back-side; hug them when StayClose.
			Desired = PlayerLoc - PlayerForward * HealerTrailDistance + PlayerRight * (Spacing * 0.25f);
			DesiredCombatDistance = Spacing;
			break;
		}

		case ESFCompanionStance::Custom:
		default:
		{
			Desired = PlayerLoc - PlayerForward * (Spacing * 0.5f);
			DesiredCombatDistance = Spacing;
			break;
		}
	}

	// Preserve player's Z plane so move-to doesn't path through floors / ceilings.
	Desired.Z = PlayerLoc.Z;

	BB->SetValueAsVector(DesiredPositionKey, Desired);
	BB->SetValueAsFloat(DesiredCombatDistanceKey, DesiredCombatDistance);
}
