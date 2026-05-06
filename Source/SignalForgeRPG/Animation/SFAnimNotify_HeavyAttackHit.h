#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "SFAnimNotify_HeavyAttackHit.generated.h"

UCLASS()
class SIGNALFORGERPG_API USFAnimNotify_HeavyAttackHit : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};