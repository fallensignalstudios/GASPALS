#include "AI/BTTask_AttackCurrentTarget.h"

#include "AI/SFCompanionAIController.h"
#include "AbilitySystem/SFGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/SFCharacterBase.h"
#include "Companions/SFCompanionCharacter.h"
#include "Core/SignalForgeGameplayTags.h"

UBTTask_AttackCurrentTarget::UBTTask_AttackCurrentTarget()
{
	NodeName = TEXT("Attack Current Target");

	const FSignalForgeGameplayTags& GameplayTags = FSignalForgeGameplayTags::Get();
	AbilityTag = GameplayTags.Ability_Attack_Light;
}

EBTNodeResult::Type UBTTask_AttackCurrentTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	ASFCompanionAIController* AIC = Cast<ASFCompanionAIController>(OwnerComp.GetAIOwner());
	if (!BB || !AIC)
	{
		return EBTNodeResult::Failed;
	}

	ASFCompanionCharacter* Companion = AIC->GetControlledCompanion();
	if (!Companion)
	{
		return EBTNodeResult::Failed;
	}

	if (const ASFCharacterBase* AsChar = Cast<ASFCharacterBase>(Companion))
	{
		if (AsChar->IsDead())
		{
			return EBTNodeResult::Failed;
		}
	}

	AActor* Target = Cast<AActor>(BB->GetValueAsObject(FocusTargetKey));
	if (!IsValid(Target))
	{
		return EBTNodeResult::Failed;
	}

	if (const ASFCharacterBase* TargetChar = Cast<ASFCharacterBase>(Target))
	{
		if (TargetChar->IsDead())
		{
			return EBTNodeResult::Failed;
		}
	}

	UAbilitySystemComponent* ASC = Companion->GetAbilitySystemComponent();
	if (!ASC || !AbilityTag.IsValid())
	{
		return EBTNodeResult::Failed;
	}

	// Aim at the target so the activation traces / projectile spawns face it.
	AIC->SetFocus(Target);

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		const USFGameplayAbility* CDO = Cast<USFGameplayAbility>(Spec.Ability);
		if (!CDO)
		{
			continue;
		}

		if (CDO->GetAssetTags().HasTagExact(AbilityTag))
		{
			const bool bActivated = ASC->TryActivateAbility(Spec.Handle);
			return bActivated ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
		}
	}

	return EBTNodeResult::Failed;
}
