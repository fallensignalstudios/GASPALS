#include "AI/BTTask_UseCompanionAbility.h"

#include "AI/SFCompanionAIController.h"
#include "AbilitySystem/SFGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Companions/SFCompanionCharacter.h"
#include "Companions/SFCompanionTacticsComponent.h"

UBTTask_UseCompanionAbility::UBTTask_UseCompanionAbility()
{
	NodeName = TEXT("Use Companion Ability");
}

EBTNodeResult::Type UBTTask_UseCompanionAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
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

	USFCompanionTacticsComponent* Tactics = Companion->GetTactics();
	if (!Tactics)
	{
		return EBTNodeResult::Failed;
	}

	// Resolve which tag we're activating.
	FGameplayTag TagToFire = AbilityTag;
	if (bUseOrderTag)
	{
		const FName TagName = BB->GetValueAsName(OrderAbilityTagKey);
		if (TagName == NAME_None)
		{
			return EBTNodeResult::Failed;
		}
		TagToFire = FGameplayTag::RequestGameplayTag(TagName, /*ErrorIfNotFound=*/false);
	}

	if (!TagToFire.IsValid())
	{
		return EBTNodeResult::Failed;
	}

	// Commanded activations respect the per-companion command cooldown.
	if (bUseOrderTag && !Tactics->CanCommandAbility())
	{
		return EBTNodeResult::Failed;
	}

	UAbilitySystemComponent* ASC = Companion->GetAbilitySystemComponent();
	if (!ASC)
	{
		return EBTNodeResult::Failed;
	}

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		const USFGameplayAbility* CDO = Cast<USFGameplayAbility>(Spec.Ability);
		if (!CDO)
		{
			continue;
		}

		if (CDO->GetAssetTags().HasTagExact(TagToFire))
		{
			const bool bActivated = ASC->TryActivateAbility(Spec.Handle);
			if (bActivated)
			{
				if (bUseOrderTag && Companion->HasAuthority())
				{
					Tactics->NotifyAbilityCommanded();
				}
				return EBTNodeResult::Succeeded;
			}
			return EBTNodeResult::Failed;
		}
	}

	return EBTNodeResult::Failed;
}
