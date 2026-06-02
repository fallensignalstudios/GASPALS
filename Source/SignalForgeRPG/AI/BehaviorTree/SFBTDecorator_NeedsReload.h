#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "SFBTDecorator_NeedsReload.generated.h"

/**
 * Returns true when the AI's currently-equipped weapon has at most
 * AmmoThreshold rounds in its clip, AND the weapon's authored clip size is
 * positive (so unlimited-ammo weapons -- beam emitters, melee, casters --
 * never report needing reload).
 *
 * Use this as a high-priority gate on a Reload subtree:
 *
 *   Selector
 *   ├── [Decorator: NeedsReload]  Reload subtree
 *   ├── Combat subtree (FaceTarget -> FireWeapon)
 *   └── ...
 *
 * Default AmmoThreshold = 0 (only triggers on truly empty mag). Increase to 1
 * or 2 to make the AI top off more aggressively.
 */
UCLASS()
class SIGNALFORGERPG_API USFBTDecorator_NeedsReload : public UBTDecorator
{
	GENERATED_BODY()

public:
	USFBTDecorator_NeedsReload();

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;

protected:
	/** Reload when AmmoInClip is at or below this threshold. 0 = only when empty. */
	UPROPERTY(EditAnywhere, Category = "Reload", meta = (ClampMin = "0", ClampMax = "100"))
	int32 AmmoThreshold = 0;
};
