#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "SFAnimNotifyState_MeleeWindow.generated.h"

/**
 * Marks the active hit-detection window of a melee swing. Designers drop this notify state on a
 * melee montage section spanning the frames where the swing should actually deal damage.
 *
 * NotifyBegin -> CombatComponent::BeginAttackWindow() (clears HitActorsThisAttack, opens window)
 * NotifyTick  -> CombatComponent::HandleAttackHitInternal() for the configured attack type each
 *                tick the window is open (multi-hit per swing; HitActorsThisAttack dedupes).
 * NotifyEnd   -> CombatComponent::EndAttackWindow() (closes window).
 *
 * AttackType chooses whether the trace uses the light or heavy tuning on the combat component
 * (radius / start offset / trace length).
 */
UENUM(BlueprintType)
enum class ESFMeleeWindowAttackType : uint8
{
	Light	UMETA(DisplayName = "Light"),
	Heavy	UMETA(DisplayName = "Heavy")
};

UCLASS(meta = (DisplayName = "Melee Hit Window"))
class SIGNALFORGERPG_API USFAnimNotifyState_MeleeWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	/** Whether this window deals light or heavy damage (selects trace tuning on CombatComponent). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee Window")
	ESFMeleeWindowAttackType AttackType = ESFMeleeWindowAttackType::Light;

	/**
	 * If true, hit-detection runs every NotifyTick. If false, fires only once at NotifyBegin (best
	 * for fast jabs where window length doesn't matter). Default true for multi-frame swing arcs.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee Window")
	bool bTickHitDetection = true;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime,
		const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
