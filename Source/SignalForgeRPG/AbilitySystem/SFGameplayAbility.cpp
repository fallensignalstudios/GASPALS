#include "AbilitySystem/SFGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/DataValidation.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

USFGameplayAbility::USFGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void USFGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);
	NotifyAbilityUIChanged(ActorInfo);
}

void USFGameplayAbility::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnRemoveAbility(ActorInfo, Spec);
	NotifyAbilityUIChanged(ActorInfo);
}

void USFGameplayAbility::NotifyAbilityUIChanged(const FGameplayAbilityActorInfo* ActorInfo) const
{
	// Ability bar refreshes via USFAbilitySystemComponent::OnAbilitiesChanged delegate.
}

bool USFGameplayAbility::PlayAbilityMontage(float PlayRate, FName StartSectionName)
{
	if (!CurrentActorInfo || !ActivationMontage)
	{
		return false;
	}

	AActor* AvatarActor = CurrentActorInfo->AvatarActor.Get();
	ACharacter* Character = Cast<ACharacter>(AvatarActor);
	if (!Character)
	{
		return false;
	}

	USkeletalMeshComponent* MeshComp = Character->GetMesh();
	if (!MeshComp)
	{
		return false;
	}

	UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
	if (!AnimInstance)
	{
		return false;
	}

	const float Duration = AnimInstance->Montage_Play(ActivationMontage, PlayRate);
	if (Duration <= 0.0f)
	{
		return false;
	}

	if (StartSectionName != NAME_None)
	{
		AnimInstance->Montage_JumpToSection(StartSectionName, ActivationMontage);
	}

	return true;
}

void USFGameplayAbility::StopAbilityMontage(float OverrideBlendOutTime)
{
	if (!CurrentActorInfo || !ActivationMontage)
	{
		return;
	}

	AActor* AvatarActor = CurrentActorInfo->AvatarActor.Get();
	ACharacter* Character = Cast<ACharacter>(AvatarActor);
	if (!Character)
	{
		return;
	}

	USkeletalMeshComponent* MeshComp = Character->GetMesh();
	if (!MeshComp)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = MeshComp->GetAnimInstance())
	{
		AnimInstance->Montage_Stop(OverrideBlendOutTime, ActivationMontage);
	}
}

void USFGameplayAbility::SpawnAbilityVFX() const
{
	if (!CurrentActorInfo || !ActivationVFX)
	{
		return;
	}

	AActor* AvatarActor = CurrentActorInfo->AvatarActor.Get();
	if (!AvatarActor)
	{
		return;
	}

	if (ACharacter* Character = Cast<ACharacter>(AvatarActor))
	{
		if (USkeletalMeshComponent* MeshComp = Character->GetMesh())
		{
			UNiagaraFunctionLibrary::SpawnSystemAttached(
				ActivationVFX,
				MeshComp,
				ActivationVFXAttachSocket,
				ActivationVFXRelativeLocation,
				ActivationVFXRelativeRotation,
				EAttachLocation::KeepRelativeOffset,
				true,
				true,
				ENCPoolMethod::AutoRelease,
				true);
			return;
		}
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		AvatarActor,
		ActivationVFX,
		AvatarActor->GetActorLocation(),
		AvatarActor->GetActorRotation(),
		ActivationVFXRelativeScale,
		true,
		true,
		ENCPoolMethod::AutoRelease,
		true);
}

void USFGameplayAbility::PlayAbilitySFX() const
{
	if (!CurrentActorInfo || !ActivationSFX)
	{
		return;
	}

	AActor* AvatarActor = CurrentActorInfo->AvatarActor.Get();
	if (!AvatarActor)
	{
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(
		AvatarActor,
		ActivationSFX,
		AvatarActor->GetActorLocation());
}

#if WITH_EDITOR
EDataValidationResult USFGameplayAbility::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (bShowInAbilityBar && AbilityIcon == nullptr)
	{
		Context.AddError(FText::FromString(
			FString::Printf(TEXT("%s is configured to appear in the ability bar but has no AbilityIcon."),
				*GetName())));
		Result = EDataValidationResult::Invalid;
	}

	if (ActivationPolicy != ESFAbilityActivationPolicy::OnInputTriggered &&
		ActivationPolicy != ESFAbilityActivationPolicy::WhileInputActive)
	{
		Context.AddError(FText::FromString(
			FString::Printf(TEXT("%s has an invalid ActivationPolicy."), *GetName())));
		Result = EDataValidationResult::Invalid;
	}

	if (ActivationPolicy == ESFAbilityActivationPolicy::OnInputTriggered && !InputTag.IsValid())
	{
		Context.AddWarning(FText::FromString(
			FString::Printf(TEXT("%s uses OnInputTriggered but has no InputTag."), *GetName())));
		if (Result == EDataValidationResult::Valid)
		{
			Result = EDataValidationResult::Invalid;
		}
	}

	if (ActivationPolicy == ESFAbilityActivationPolicy::WhileInputActive && !InputTag.IsValid())
	{
		Context.AddWarning(FText::FromString(
			FString::Printf(TEXT("%s uses WhileInputActive but has no InputTag."), *GetName())));
		if (Result == EDataValidationResult::Valid)
		{
			Result = EDataValidationResult::Invalid;
		}
	}

	return Result;
}
#endif