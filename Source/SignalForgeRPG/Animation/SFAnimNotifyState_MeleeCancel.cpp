#include "Animation/SFAnimNotifyState_MeleeCancel.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/SignalForgeGameplayTags.h"

namespace SFMeleeCancelDetail
{
	static UAbilitySystemComponent* GetASC(USkeletalMeshComponent* MeshComp)
	{
		return MeshComp ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner()) : nullptr;
	}
}

void USFAnimNotifyState_MeleeCancel::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (UAbilitySystemComponent* ASC = SFMeleeCancelDetail::GetASC(MeshComp))
	{
		const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
		if (Tags.State_Weapon_MeleeCancelWindow.IsValid())
		{
			ASC->AddLooseGameplayTag(Tags.State_Weapon_MeleeCancelWindow, 1);
		}
	}
}

void USFAnimNotifyState_MeleeCancel::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (UAbilitySystemComponent* ASC = SFMeleeCancelDetail::GetASC(MeshComp))
	{
		const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
		if (Tags.State_Weapon_MeleeCancelWindow.IsValid())
		{
			ASC->RemoveLooseGameplayTag(Tags.State_Weapon_MeleeCancelWindow);
		}
	}
}
