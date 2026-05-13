#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "SFAnimNotify_CastRelease.generated.h"

/**
 * One-shot AnimNotify placed in a caster montage at the frame the projectile should leave the
 * character. Sends FSignalForgeGameplayTags::Event_Cast_Release to the owning actor's ASC; the
 * active USFGameplayAbility_WeaponCast listens for it and spawns the projectile in response.
 *
 * Authoring: drop this notify on the Release section of a caster montage, ~1-2 frames after the
 * pose hits its release apex. Do NOT spawn the projectile from the ability's ActivateAbility --
 * it should always be driven by this notify so the visual and the gameplay stay in sync.
 */
UCLASS(meta = (DisplayName = "SF Cast Release"))
class SIGNALFORGERPG_API USFAnimNotify_CastRelease : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
