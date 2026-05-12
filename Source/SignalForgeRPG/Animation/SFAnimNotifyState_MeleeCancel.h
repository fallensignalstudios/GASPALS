#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "SFAnimNotifyState_MeleeCancel.generated.h"

/**
 * Marks the late-montage window during which the next combo input can chain smoothly instead of
 * waiting for the entire montage to finish.
 *
 * Implementation: while this notify is active, sets the State.Weapon.MeleeCancelWindow loose tag
 * on the character's ASC. USFGameplayAbility_WeaponMelee checks for that tag when its input is
 * pressed to decide whether to interrupt the current montage and play the next combo step.
 */
UCLASS(meta = (DisplayName = "Melee Cancel Window"))
class SIGNALFORGERPG_API USFAnimNotifyState_MeleeCancel : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override { return TEXT("Melee Cancel Window"); }
};
