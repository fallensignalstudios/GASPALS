#include "AbilitySystem/SFGameplayAbility_Block.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "Core/SignalForgeGameplayTags.h"
#include "GameplayEffect.h"

USFGameplayAbility_Block::USFGameplayAbility_Block()
{
	ActivationPolicy = ESFAbilityActivationPolicy::WhileInputActive;

	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();

	ActivationBlockedTags.AddTag(Tags.State_Broken);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

bool USFGameplayAbility_Block::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	const bool bSuperResult = Super::CanActivateAbility(
		Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);

	/*UE_LOG(LogTemp, Warning, TEXT("Block CanActivateAbility: Super=%s"),
		bSuperResult ? TEXT("true") : TEXT("false"));*/

	if (!bSuperResult)
	{
		return false;
	}

	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		/*UE_LOG(LogTemp, Warning, TEXT("Block CanActivateAbility: invalid ActorInfo/ASC"));*/
		return false;
	}

	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();

	if (ASC->HasMatchingGameplayTag(Tags.State_Broken))
	{
		/*UE_LOG(LogTemp, Warning, TEXT("Block CanActivateAbility: blocked by State.Broken"));*/
		return false;
	}

	/*UE_LOG(LogTemp, Warning, TEXT("Block CanActivateAbility: success"));*/
	return true;
}

void USFGameplayAbility_Block::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	/*UE_LOG(LogTemp, Warning, TEXT("Block ActivateAbility: entered"));*/

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		/*UE_LOG(LogTemp, Warning, TEXT("Block ActivateAbility: CommitAbility failed"));*/
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	/*UE_LOG(LogTemp, Warning, TEXT("Block ActivateAbility: CommitAbility succeeded"));*/

	PlayAbilityMontage();
	SpawnAbilityVFX();
	PlayAbilitySFX();

	UAbilitySystemComponent* ASC = ActorInfo && ActorInfo->AbilitySystemComponent.IsValid()
		? ActorInfo->AbilitySystemComponent.Get()
		: nullptr;

	if (ASC && BlockingEffectClass)
	{
		/*UE_LOG(LogTemp, Warning, TEXT("Block ActivateAbility: applying blocking effect"));*/
		const FGameplayEffectSpecHandle SpecHandle =
			MakeOutgoingGameplayEffectSpec(BlockingEffectClass, GetAbilityLevel());

		if (SpecHandle.IsValid())
		{
			BlockingEffectHandle = ApplyGameplayEffectSpecToOwner(
				Handle,
				ActorInfo,
				ActivationInfo,
				SpecHandle);

			/*UE_LOG(LogTemp, Warning, TEXT("Block ActivateAbility: effect applied, handle valid=%s"),
				BlockingEffectHandle.IsValid() ? TEXT("true") : TEXT("false"));*/
		}
		else
		{
			/*UE_LOG(LogTemp, Warning, TEXT("Block ActivateAbility: effect spec invalid"));*/
		}
	}
	else
	{
		/*UE_LOG(LogTemp, Warning, TEXT("Block ActivateAbility: no ASC or no BlockingEffectClass"));*/
	}

	WaitInputReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
	if (WaitInputReleaseTask)
	{
		/*UE_LOG(LogTemp, Warning, TEXT("Block ActivateAbility: wait input release task created"));*/
		WaitInputReleaseTask->OnRelease.AddDynamic(this, &USFGameplayAbility_Block::OnInputReleased);
		WaitInputReleaseTask->ReadyForActivation();
	}
	else
	{
		/*UE_LOG(LogTemp, Warning, TEXT("Block ActivateAbility: failed to create release task"));*/
	}
}

void USFGameplayAbility_Block::OnInputReleased(float TimeHeld)
{
	UE_LOG(LogTemp, Warning, TEXT("Block OnInputReleased: TimeHeld=%f"), TimeHeld);

	if (!IsActive())
	{
		/*UE_LOG(LogTemp, Warning, TEXT("Block OnInputReleased: ability not active"));*/
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGameplayAbility_Block::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{

	StopAbilityMontage(0.15f);
	/*UE_LOG(LogTemp, Warning, TEXT("Block EndAbility: WasCancelled=%s"),
		bWasCancelled ? TEXT("true") : TEXT("false"));*/

	if (WaitInputReleaseTask)
	{
		WaitInputReleaseTask->EndTask();
		WaitInputReleaseTask = nullptr;
	}

	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
		{
			if (BlockingEffectHandle.IsValid())
			{
				/*UE_LOG(LogTemp, Warning, TEXT("Block EndAbility: removing blocking effect"));*/
				ASC->RemoveActiveGameplayEffect(BlockingEffectHandle);
				BlockingEffectHandle.Invalidate();
			}
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}