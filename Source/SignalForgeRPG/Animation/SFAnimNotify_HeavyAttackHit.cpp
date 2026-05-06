#include "Animation/SFAnimNotify_HeavyAttackHit.h"
#include "Characters/SFCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"

void USFAnimNotify_HeavyAttackHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
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

	Character->HandleHeavyAttackHitEvent();
}