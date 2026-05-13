#include "Animation/SFAnimNotify_CastRelease.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/SignalForgeGameplayTags.h"

void USFAnimNotify_CastRelease::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();

	FGameplayEventData Payload;
	Payload.EventTag = Tags.Event_Cast_Release;
	Payload.Instigator = OwnerActor;
	Payload.Target = OwnerActor;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		OwnerActor,
		Tags.Event_Cast_Release,
		Payload);
}

FString USFAnimNotify_CastRelease::GetNotifyName_Implementation() const
{
	return TEXT("Cast Release");
}
