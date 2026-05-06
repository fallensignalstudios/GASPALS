#include "AbilitySystem/SFGameplayAbility_Dash.h"

#include "Animation/AnimMontage.h"
#include "Characters/SFCharacterBase.h"
#include "Core/SignalForgeGameplayTags.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

USFGameplayAbility_Dash::USFGameplayAbility_Dash()
{
	const FSignalForgeGameplayTags& GameplayTags = FSignalForgeGameplayTags::Get();

	InputTag = GameplayTags.Input_Ability_3;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(GameplayTags.Ability_Movement_Dash);
	SetAssetTags(AssetTags);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool USFGameplayAbility_Dash::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	ASFCharacterBase* Character = nullptr;
	return TryGetDashCharacter(ActorInfo, Character) && DashStrength > 0.0f;
}

void USFGameplayAbility_Dash::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ASFCharacterBase* Character = nullptr;
	if (!TryGetDashCharacter(ActorInfo, Character))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FVector DashDirection = ResolveDashDirection(Character);
	if (DashDirection.IsNearlyZero())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (DashMontage)
	{
		Character->PlayAnimMontage(DashMontage);
	}

	Character->PerformDash(DashDirection, DashStrength);

	if (DashNiagara)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			DashNiagara,
			Character->GetRootComponent(),
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}

bool USFGameplayAbility_Dash::TryGetDashCharacter(
	const FGameplayAbilityActorInfo* ActorInfo,
	ASFCharacterBase*& OutCharacter) const
{
	OutCharacter = nullptr;

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		return false;
	}

	OutCharacter = Cast<ASFCharacterBase>(ActorInfo->AvatarActor.Get());
	return OutCharacter != nullptr;
}

FVector USFGameplayAbility_Dash::ResolveDashDirection(const ASFCharacterBase* Character) const
{
	if (!Character)
	{
		return FVector::ZeroVector;
	}

	if (bUseLastMovementInputForDash)
	{
		FVector InputDirection = Character->GetLastMovementInputVector();
		InputDirection.Z = 0.0f;

		if (!InputDirection.IsNearlyZero())
		{
			return InputDirection.GetSafeNormal();
		}
	}

	FVector ForwardDirection = Character->GetActorForwardVector();
	ForwardDirection.Z = 0.0f;
	return ForwardDirection.GetSafeNormal();
}