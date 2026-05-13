#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "SFBTService_SenseUpdate.generated.h"

/**
 * Polls the AI controller's AIPerceptionComponent for currently-perceived
 * actors and writes the closest hostile (filtered through the faction system)
 * into the configured TargetActor BB key.
 *
 * Pairs naturally with FindHostileInRadius (which scans the world) — this
 * service uses perception as the primary acquire path so AI react to what
 * they've actually seen / heard, while the radius scan is a fallback.
 *
 * Will only overwrite an existing target if bPreferCloserTargets = true and
 * the newly-perceived hostile is closer than the current one. Otherwise the
 * existing TargetActor is preserved until cleared by UpdateTargetState's
 * leash / dead / non-hostile logic.
 */
UCLASS()
class SIGNALFORGERPG_API USFBTService_SenseUpdate : public UBTService
{
	GENERATED_BODY()

public:
	USFBTService_SenseUpdate();

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Keys")
	struct FBlackboardKeySelector TargetActorKey;

	/** Require line-of-sight to count a perceived actor (filters out hearing-only stimuli). */
	UPROPERTY(EditAnywhere, Category = "Behavior")
	bool bRequireLineOfSight = false;

	/** If true, switch to a closer hostile mid-fight; otherwise keep the existing target. */
	UPROPERTY(EditAnywhere, Category = "Behavior")
	bool bPreferCloserTargets = false;

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
};
