#include "AI/BehaviorTree/SFBTTask_Reload.h"

#include "AI/BehaviorTree/SFBTHelpers.h"
#include "AIController.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Characters/SFCharacterBase.h"
#include "Core/SignalForgeGameplayTags.h"

USFBTTask_Reload::USFBTTask_Reload()
{
	NodeName = TEXT("SF Reload");
	bNotifyTick = true;
}

EBTNodeResult::Type USFBTTask_Reload::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ASFCharacterBase* Character = SFBTHelpers::GetControlledCharacter(OwnerComp.GetAIOwner());
	if (!Character)
	{
		return EBTNodeResult::Failed;
	}

	USFAbilitySystemComponent* ASC = Cast<USFAbilitySystemComponent>(Character->GetAbilitySystemComponent());
	if (!ASC)
	{
		return EBTNodeResult::Failed;
	}

	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();

	// If a reload is already running (e.g. because the player-grade abilities
	// pre-triggered it on level start), just wait for it to finish rather than
	// firing a redundant press that the ability would refuse anyway.
	const bool bAlreadyReloading = ASC->HasMatchingGameplayTag(Tags.State_Weapon_Reloading);

	FMemory* Mem = reinterpret_cast<FMemory*>(NodeMemory);
	new (Mem) FMemory();
	Mem->bSawReloadingTag = bAlreadyReloading;

	if (!bAlreadyReloading)
	{
		ASC->AbilityInputTagPressed(Tags.Input_Reload);
		// Release immediately -- USFGameplayAbility_Reload activates on press
		// (OnInputTriggered) and runs its own montage / timer to completion.
		// Holding the input adds nothing and risks queuing a re-press if the
		// ability is briefly inactive between commit and montage start.
		ASC->AbilityInputTagReleased(Tags.Input_Reload);
	}

	return EBTNodeResult::InProgress;
}

void USFBTTask_Reload::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FMemory* Mem = reinterpret_cast<FMemory*>(NodeMemory);
	if (!Mem)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	ASFCharacterBase* Character = SFBTHelpers::GetControlledCharacter(OwnerComp.GetAIOwner());
	USFAbilitySystemComponent* ASC = Character
		? Cast<USFAbilitySystemComponent>(Character->GetAbilitySystemComponent())
		: nullptr;
	if (!ASC)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
	const bool bReloadingNow = ASC->HasMatchingGameplayTag(Tags.State_Weapon_Reloading);

	if (bReloadingNow)
	{
		Mem->bSawReloadingTag = true;
	}

	// Reload completes when the State.Weapon.Reloading tag is removed AFTER we
	// observed it set at least once. If we never saw the tag turn on (because
	// the ability refused to activate -- no ammo reserve, blocked tags, etc.)
	// fall through to the timeout so the BT doesn't pin here forever.
	if (Mem->bSawReloadingTag && !bReloadingNow)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	Mem->ElapsedSeconds += DeltaSeconds;
	if (Mem->ElapsedSeconds >= TimeoutSeconds)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[SFBTTask_Reload] '%s' timed out after %.2fs (SawReloadingTag=%d, ReloadingNow=%d). Reload ability may be missing or blocked."),
			*GetNameSafe(Character), Mem->ElapsedSeconds, Mem->bSawReloadingTag ? 1 : 0, bReloadingNow ? 1 : 0);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
	}
}

EBTNodeResult::Type USFBTTask_Reload::AbortTask(UBehaviorTreeComponent& /*OwnerComp*/, uint8* /*NodeMemory*/)
{
	// We don't cancel the reload ability on abort: the reload should still
	// finish even if the BT moved on (e.g. flee branch took priority). The
	// ASC will remove the Reloading tag itself on completion.
	return EBTNodeResult::Aborted;
}

FString USFBTTask_Reload::GetStaticDescription() const
{
	return FString::Printf(TEXT("Reload (timeout %.1fs)"), TimeoutSeconds);
}
