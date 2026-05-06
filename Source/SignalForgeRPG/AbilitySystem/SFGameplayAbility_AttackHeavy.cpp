#include "AbilitySystem/SFGameplayAbility_AttackHeavy.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Characters/SFCharacterBase.h"
#include "Combat/SFCombatComponent.h"
#include "Components/SFEquipmentComponent.h"
#include "Core/SignalForgeGameplayTags.h"
#include "AbilitySystemComponent.h"

USFGameplayAbility_AttackHeavy::USFGameplayAbility_AttackHeavy()
{
	const FSignalForgeGameplayTags& GameplayTags = FSignalForgeGameplayTags::Get();

	InputTag = GameplayTags.Input_Ability_2;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(GameplayTags.Ability_Attack_Heavy);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(GameplayTags.State_Attacking);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool USFGameplayAbility_AttackHeavy::CanActivateAbility(
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
	USFEquipmentComponent* EquipmentComponent = nullptr;
	UAnimMontage* AttackMontage = nullptr;

	return TryGetActivationData(ActorInfo, Character, EquipmentComponent, AttackMontage);
}

void USFGameplayAbility_AttackHeavy::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ASFCharacterBase* Character = nullptr;
	USFEquipmentComponent* EquipmentComponent = nullptr;
	UAnimMontage* AttackMontage = nullptr;

	if (!TryGetActivationData(ActorInfo, Character, EquipmentComponent, AttackMontage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (USFCombatComponent* CombatComponent = Character->GetCombatComponent())
	{
		CombatComponent->BeginAttackWindow();
	}

	CachedHandle = Handle;
	CachedActorInfo = ActorInfo;
	CachedActivationInfo = ActivationInfo;
	CachedAbilitySystemComponent = Cast<UAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	bHasHandledMontageExit = false;

	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			AttackMontage);

	if (!MontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &USFGameplayAbility_AttackHeavy::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &USFGameplayAbility_AttackHeavy::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &USFGameplayAbility_AttackHeavy::OnMontageCancelled);
	MontageTask->OnBlendOut.AddDynamic(this, &USFGameplayAbility_AttackHeavy::OnMontageBlendOut);
	MontageTask->ReadyForActivation();
}

void USFGameplayAbility_AttackHeavy::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (ActorInfo && ActorInfo->AvatarActor.IsValid())
	{
		if (ASFCharacterBase* Character = Cast<ASFCharacterBase>(ActorInfo->AvatarActor.Get()))
		{
			if (USFCombatComponent* CombatComponent = Character->GetCombatComponent())
			{
				CombatComponent->EndAttackWindow();
			}
		}
	}

	ClearCachedActivationState();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool USFGameplayAbility_AttackHeavy::TryGetActivationData(
	const FGameplayAbilityActorInfo* ActorInfo,
	ASFCharacterBase*& OutCharacter,
	USFEquipmentComponent*& OutEquipmentComponent,
	UAnimMontage*& OutAttackMontage) const
{
	OutCharacter = nullptr;
	OutEquipmentComponent = nullptr;
	OutAttackMontage = nullptr;

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		return false;
	}

	OutCharacter = Cast<ASFCharacterBase>(ActorInfo->AvatarActor.Get());
	if (!OutCharacter)
	{
		return false;
	}

	OutEquipmentComponent = OutCharacter->GetEquipmentComponent();
	if (!OutEquipmentComponent)
	{
		return false;
	}

	OutAttackMontage = OutEquipmentComponent->GetCurrentHeavyAttackMontage();
	return OutAttackMontage != nullptr;
}

void USFGameplayAbility_AttackHeavy::HandleMontageFinished(bool bWasCancelled, bool bReplicateEndAbility)
{
	if (bHasHandledMontageExit || !IsActive())
	{
		return;
	}

	bHasHandledMontageExit = true;
	EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USFGameplayAbility_AttackHeavy::ClearCachedActivationState()
{
	CachedHandle = FGameplayAbilitySpecHandle();
	CachedActorInfo = nullptr;
	CachedActivationInfo = FGameplayAbilityActivationInfo();
	CachedAbilitySystemComponent.Reset();
	bHasHandledMontageExit = false;
}

void USFGameplayAbility_AttackHeavy::OnMontageCompleted()
{
	HandleMontageFinished(false, false);
}

void USFGameplayAbility_AttackHeavy::OnMontageInterrupted()
{
	HandleMontageFinished(true, true);
}

void USFGameplayAbility_AttackHeavy::OnMontageCancelled()
{
	HandleMontageFinished(true, true);
}

void USFGameplayAbility_AttackHeavy::OnMontageBlendOut()
{
	HandleMontageFinished(false, false);
}