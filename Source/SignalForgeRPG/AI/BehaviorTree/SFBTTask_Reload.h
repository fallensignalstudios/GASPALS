#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "SFBTTask_Reload.generated.h"

/**
 * Presses the Input.Reload input tag on the AI's ASC, then waits for the
 * State.Weapon.Reloading loose gameplay tag to clear (USFGameplayAbility_Reload
 * adds the tag while reloading and removes it on completion).
 *
 * Pair this with SFBTDecorator_NeedsReload in the BT so the AI only enters
 * this branch when its magazine is actually empty (or low). The task ends
 * Succeeded when the reload completes, Failed if no ASC / no reload ability
 * is granted, or if TimeoutSeconds elapses (handles a stuck reload).
 */
UCLASS()
class SIGNALFORGERPG_API USFBTTask_Reload : public UBTTaskNode
{
	GENERATED_BODY()

public:
	USFBTTask_Reload();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FMemory); }

protected:
	/** Hard cap so a broken reload ability doesn't pin the BT forever. */
	UPROPERTY(EditAnywhere, Category = "Reload", meta = (ClampMin = "0.5", ClampMax = "20.0"))
	float TimeoutSeconds = 6.0f;

private:
	struct FMemory
	{
		float ElapsedSeconds = 0.0f;
		bool bSawReloadingTag = false;
	};
};
