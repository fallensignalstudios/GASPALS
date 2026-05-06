#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_SyncCompanionTactics.generated.h"

/**
 * UBTService_SyncCompanionTactics
 *
 * Reads USFCompanionTacticsComponent on the controlled pawn each tick and
 * mirrors its authoritative state onto the blackboard:
 *   - Stance / Aggression / EngagementRange (enums)
 *   - Active order: Type, Sequence, TargetActor, TargetLocation, AbilityTag
 *   - bHasValidOrder, bSelfLowHealth, bPlayerLowHealth, bCanUseCommandedAbility
 *   - First stance gameplay tag (Name) for quick subtree branching
 *
 * Tactics is the authoritative intent layer; the BT only translates these
 * values into action. Place this service at or near the BT root.
 */
UCLASS()
class SIGNALFORGERPG_API UBTService_SyncCompanionTactics : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_SyncCompanionTactics();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// --- Order keys --------------------------------------------------------

	UPROPERTY(EditAnywhere, Category = "Blackboard|Order")
	FName OrderTypeKey = TEXT("OrderType");

	UPROPERTY(EditAnywhere, Category = "Blackboard|Order")
	FName OrderSequenceKey = TEXT("OrderSequence");

	UPROPERTY(EditAnywhere, Category = "Blackboard|Order")
	FName OrderTargetActorKey = TEXT("OrderTargetActor");

	UPROPERTY(EditAnywhere, Category = "Blackboard|Order")
	FName OrderTargetLocationKey = TEXT("OrderTargetLocation");

	UPROPERTY(EditAnywhere, Category = "Blackboard|Order")
	FName OrderAbilityTagKey = TEXT("OrderAbilityTag");

	UPROPERTY(EditAnywhere, Category = "Blackboard|Order")
	FName HasValidOrderKey = TEXT("bHasValidOrder");

	// --- Stance / aggression / range --------------------------------------

	UPROPERTY(EditAnywhere, Category = "Blackboard|Stance")
	FName StanceKey = TEXT("Stance");

	UPROPERTY(EditAnywhere, Category = "Blackboard|Stance")
	FName AggressionKey = TEXT("Aggression");

	UPROPERTY(EditAnywhere, Category = "Blackboard|Stance")
	FName EngagementRangeKey = TEXT("EngagementRange");

	UPROPERTY(EditAnywhere, Category = "Blackboard|Stance")
	FName StanceTagKey = TEXT("StanceTag");

	// --- Threshold flags ---------------------------------------------------

	UPROPERTY(EditAnywhere, Category = "Blackboard|Threshold")
	FName SelfLowHealthKey = TEXT("bSelfLowHealth");

	UPROPERTY(EditAnywhere, Category = "Blackboard|Threshold")
	FName PlayerLowHealthKey = TEXT("bPlayerLowHealth");

	UPROPERTY(EditAnywhere, Category = "Blackboard|Threshold")
	FName CanUseCommandedAbilityKey = TEXT("bCanUseCommandedAbility");
};
