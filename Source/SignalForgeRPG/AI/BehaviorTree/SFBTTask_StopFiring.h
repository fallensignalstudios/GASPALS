#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "SFBTTask_StopFiring.generated.h"

/**
 * Releases the Input.PrimaryFire input tag on the AI's ASC. Pairs with
 * SFBTTask_FireWeapon in PressOnly mode for full-auto / sustained-beam
 * weapons that need an explicit release later in the tree (e.g. after
 * the target leaves LOS or the magazine empties).
 *
 * Always succeeds (it's a no-op if input was already released).
 */
UCLASS()
class SIGNALFORGERPG_API USFBTTask_StopFiring : public UBTTaskNode
{
	GENERATED_BODY()

public:
	USFBTTask_StopFiring();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override { return TEXT("Stop Firing (release primary fire)"); }
};
