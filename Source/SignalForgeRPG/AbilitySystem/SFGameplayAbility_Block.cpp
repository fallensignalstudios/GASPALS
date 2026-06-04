#include "AbilitySystem/SFGameplayAbility_Block.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "Core/SignalForgeGameplayTags.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "TimerManager.h"
#include "Core/SignalForgeLogChannels.h"

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

	/*UE_LOG(LogSFCombat, Warning, TEXT("Block CanActivateAbility: Super=%s"),
		bSuperResult ? TEXT("true") : TEXT("false"));*/

	if (!bSuperResult)
	{
		return false;
	}

	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		/*UE_LOG(LogSFCombat, Warning, TEXT("Block CanActivateAbility: invalid ActorInfo/ASC"));*/
		return false;
	}

	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();

	if (ASC->HasMatchingGameplayTag(Tags.State_Broken))
	{
		/*UE_LOG(LogSFCombat, Warning, TEXT("Block CanActivateAbility: blocked by State.Broken"));*/
		return false;
	}

	/*UE_LOG(LogSFCombat, Warning, TEXT("Block CanActivateAbility: success"));*/
	return true;
}

void USFGameplayAbility_Block::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	/*UE_LOG(LogSFCombat, Warning, TEXT("Block ActivateAbility: entered"));*/

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		/*UE_LOG(LogSFCombat, Warning, TEXT("Block ActivateAbility: CommitAbility failed"));*/
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	/*UE_LOG(LogSFCombat, Warning, TEXT("Block ActivateAbility: CommitAbility succeeded"));*/

	PlayAbilityMontage();
	SpawnAbilityVFX();
	PlayAbilitySFX();

	UAbilitySystemComponent* ASC = ActorInfo && ActorInfo->AbilitySystemComponent.IsValid()
		? ActorInfo->AbilitySystemComponent.Get()
		: nullptr;

	// Fallen-Order-style: opening the block press also opens a brief parry
	// window, unless the player just attempted one (State.ParryCooldown).
	// The hold-block GE applies regardless — the parry window just sits on
	// top for the first ParryWindowSeconds of the press.
	OpenParryWindow();

	if (ASC && BlockingEffectClass)
	{
		/*UE_LOG(LogSFCombat, Warning, TEXT("Block ActivateAbility: applying blocking effect"));*/
		const FGameplayEffectSpecHandle SpecHandle =
			MakeOutgoingGameplayEffectSpec(BlockingEffectClass, GetAbilityLevel());

		if (SpecHandle.IsValid())
		{
			BlockingEffectHandle = ApplyGameplayEffectSpecToOwner(
				Handle,
				ActorInfo,
				ActivationInfo,
				SpecHandle);

			/*UE_LOG(LogSFCombat, Warning, TEXT("Block ActivateAbility: effect applied, handle valid=%s"),
				BlockingEffectHandle.IsValid() ? TEXT("true") : TEXT("false"));*/
		}
		else
		{
			/*UE_LOG(LogSFCombat, Warning, TEXT("Block ActivateAbility: effect spec invalid"));*/
		}
	}
	else
	{
		/*UE_LOG(LogSFCombat, Warning, TEXT("Block ActivateAbility: no ASC or no BlockingEffectClass"));*/
	}

	WaitInputReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
	if (WaitInputReleaseTask)
	{
		/*UE_LOG(LogSFCombat, Warning, TEXT("Block ActivateAbility: wait input release task created"));*/
		WaitInputReleaseTask->OnRelease.AddDynamic(this, &USFGameplayAbility_Block::OnInputReleased);
		WaitInputReleaseTask->ReadyForActivation();
	}
	else
	{
		/*UE_LOG(LogSFCombat, Warning, TEXT("Block ActivateAbility: failed to create release task"));*/
	}
}

void USFGameplayAbility_Block::OnInputReleased(float TimeHeld)
{
	UE_LOG(LogSFCombat, Verbose, TEXT("Block OnInputReleased: TimeHeld=%f"), TimeHeld);

	if (!IsActive())
	{
		/*UE_LOG(LogSFCombat, Warning, TEXT("Block OnInputReleased: ability not active"));*/
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void USFGameplayAbility_Block::OpenParryWindow()
{
	if (!CurrentActorInfo || !CurrentActorInfo->AbilitySystemComponent.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* ASC = CurrentActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		return;
	}

	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();

	// Recent parry attempt? Allow the block to proceed, but skip the window.
	// This is the gate that prevents mash-to-parry.
	if (ASC->HasMatchingGameplayTag(Tags.State_ParryCooldown))
	{
		return;
	}

	if (ParryWindowSeconds <= 0.f)
	{
		return;
	}

	ASC->AddLooseGameplayTag(Tags.State_ParryWindow);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ParryWindowTimerHandle,
			FTimerDelegate::CreateUObject(this, &USFGameplayAbility_Block::CloseParryWindow),
			ParryWindowSeconds,
			false);
	}
}

void USFGameplayAbility_Block::CloseParryWindow()
{
	if (!CurrentActorInfo || !CurrentActorInfo->AbilitySystemComponent.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* ASC = CurrentActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		return;
	}

	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();

	// Remove the window if it's still on (resolver may have already cleared it
	// on a successful parry; RemoveLooseGameplayTag is a no-op if absent).
	ASC->RemoveLooseGameplayTag(Tags.State_ParryWindow);

	if (ParryCooldownSeconds <= 0.f)
	{
		return;
	}

	ASC->AddLooseGameplayTag(Tags.State_ParryCooldown);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ParryCooldownTimerHandle,
			[ASCWeak = TWeakObjectPtr<UAbilitySystemComponent>(ASC)]()
			{
				if (UAbilitySystemComponent* PinnedASC = ASCWeak.Get())
				{
					PinnedASC->RemoveLooseGameplayTag(FSignalForgeGameplayTags::Get().State_ParryCooldown);
				}
			},
			ParryCooldownSeconds,
			false);
	}
}

void USFGameplayAbility_Block::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{

	StopAbilityMontage(0.15f);
	/*UE_LOG(LogSFCombat, Warning, TEXT("Block EndAbility: WasCancelled=%s"),
		bWasCancelled ? TEXT("true") : TEXT("false"));*/

	if (WaitInputReleaseTask)
	{
		WaitInputReleaseTask->EndTask();
		WaitInputReleaseTask = nullptr;
	}

	// If the player releases block before the parry window finishes ticking,
	// close it immediately. Otherwise the window timer would happily continue
	// firing on a dead ability instance and try to add cooldown to an ASC that
	// may or may not still want it. The ParryCooldown timer itself is allowed
	// to keep ticking — it operates on the ASC, not the ability, and clears
	// itself via the weak-ptr lambda.
	if (UWorld* World = GetWorld())
	{
		if (ParryWindowTimerHandle.IsValid() && World->GetTimerManager().IsTimerActive(ParryWindowTimerHandle))
		{
			World->GetTimerManager().ClearTimer(ParryWindowTimerHandle);
			CloseParryWindow();
		}
	}

	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
		{
			if (BlockingEffectHandle.IsValid())
			{
				/*UE_LOG(LogSFCombat, Warning, TEXT("Block EndAbility: removing blocking effect"));*/
				ASC->RemoveActiveGameplayEffect(BlockingEffectHandle);
				BlockingEffectHandle.Invalidate();
			}
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}