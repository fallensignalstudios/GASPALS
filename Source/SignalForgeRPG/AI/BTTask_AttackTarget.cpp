#include "AI/BTTask_AttackTarget.h"

#include "AI/SFEnemyAIController.h"
#include "AbilitySystem/SFGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Characters/SFEnemyCharacter.h"
#include "Core/SignalForgeGameplayTags.h"

UBTTask_AttackTarget::UBTTask_AttackTarget()
{
	NodeName = TEXT("Attack Target");

	const FSignalForgeGameplayTags& GameplayTags = FSignalForgeGameplayTags::Get();
	AbilityTag = GameplayTags.Ability_Attack_Light;
}

EBTNodeResult::Type UBTTask_AttackTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ASFEnemyAIController* AIController = Cast<ASFEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	ASFEnemyCharacter* Enemy = AIController->GetControlledEnemy();
	if (!Enemy || Enemy->IsDead())
	{
		return EBTNodeResult::Failed;
	}

	UAbilitySystemComponent* AbilitySystemComponent = Enemy->GetAbilitySystemComponent();
	if (!AbilitySystemComponent || !AbilityTag.IsValid())
	{
		return EBTNodeResult::Failed;
	}

	for (const FGameplayAbilitySpec& AbilitySpec : AbilitySystemComponent->GetActivatableAbilities())
	{
		const USFGameplayAbility* AbilityCDO = Cast<USFGameplayAbility>(AbilitySpec.Ability);
		if (!AbilityCDO)
		{
			continue;
		}

		if (AbilityCDO->GetAssetTags().HasTagExact(AbilityTag))
		{
			const bool bActivated = AbilitySystemComponent->TryActivateAbility(AbilitySpec.Handle);
			return bActivated ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
		}
	}

	return EBTNodeResult::Failed;
}