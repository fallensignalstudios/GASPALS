#include "Animation/SFAnimNotify_LightAttackHit.h"
#include "Characters/SFCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"

void USFAnimNotify_LightAttackHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	if (!MeshComp)
	{
		return;
	}

	ASFCharacterBase* Character = Cast<ASFCharacterBase>(MeshComp->GetOwner());
	if (!Character)
	{
		return;
	}

	Character->HandleLightAttackHitEvent();
}