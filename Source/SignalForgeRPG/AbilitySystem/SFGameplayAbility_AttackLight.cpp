#include "AbilitySystem/SFGameplayAbility_AttackLight.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Characters/SFCharacterBase.h"
#include "Combat/SFCombatComponent.h"
#include "Components/SFEquipmentComponent.h"
#include "Core/SignalForgeGameplayTags.h"

USFGameplayAbility_AttackLight::USFGameplayAbility_AttackLight()
{
	const FSignalForgeGameplayTags& GameplayTags = FSignalForgeGameplayTags::Get();

	InputTag = GameplayTags.Input_Ability_1;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(GameplayTags.Ability_Attack_Light);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(GameplayTags.State_Attacking);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool USFGameplayAbility_AttackLight::CanActivateAbility(
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

void USFGameplayAbility_AttackLight::ActivateAbility(
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

	MontageTask->OnCompleted.AddDynamic(this, &USFGameplayAbility_AttackLight::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &USFGameplayAbility_AttackLight::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &USFGameplayAbility_AttackLight::OnMontageCancelled);
	MontageTask->OnBlendOut.AddDynamic(this, &USFGameplayAbility_AttackLight::OnMontageBlendOut);
	MontageTask->ReadyForActivation();
}

void USFGameplayAbility_AttackLight::EndAbility(
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

bool USFGameplayAbility_AttackLight::TryGetActivationData(
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

	OutAttackMontage = OutEquipmentComponent->GetCurrentLightAttackMontage();
	return OutAttackMontage != nullptr;
}

void USFGameplayAbility_AttackLight::HandleMontageFinished(bool bWasCancelled, bool bReplicateEndAbility)
{
	if (bHasHandledMontageExit || !IsActive())
	{
		return;
	}

	bHasHandledMontageExit = true;
	EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USFGameplayAbility_AttackLight::ClearCachedActivationState()
{
	CachedHandle = FGameplayAbilitySpecHandle();
	CachedActorInfo = nullptr;
	CachedActivationInfo = FGameplayAbilityActivationInfo();
	bHasHandledMontageExit = false;
}

void USFGameplayAbility_AttackLight::OnMontageCompleted()
{
	HandleMontageFinished(false, false);
}

void USFGameplayAbility_AttackLight::OnMontageInterrupted()
{
	HandleMontageFinished(true, true);
}

void USFGameplayAbility_AttackLight::OnMontageCancelled()
{
	HandleMontageFinished(true, true);
}

void USFGameplayAbility_AttackLight::OnMontageBlendOut()
{
	HandleMontageFinished(false, false);
}