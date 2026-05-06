#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "BTTask_UseCompanionAbility.generated.h"

/**
 * UBTTask_UseCompanionAbility
 *
 * Drives ability activation from two contexts:
 *
 *   1) Commanded (UseAbility order) — read OrderAbilityTag from BB. Gated
 *      by USFCompanionTacticsComponent::CanCommandAbility() and stamps
 *      NotifyAbilityCommanded() on success so the cooldown begins.
 *
 *   2) Suggested by stance — designer-supplied AbilityTag. NOT gated by
 *      the commanded-ability cooldown (that gate is for *player commands*,
 *      not autonomous behavior).
 *
 * Mode is selected by bUseOrderTag.
 */
UCLASS()
class SIGNALFORGERPG_API UBTTask_UseCompanionAbility : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_UseCompanionAbility();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName OrderAbilityTagKey = TEXT("OrderAbilityTag");

	/** When true, read the tag from OrderAbilityTagKey + apply the command cooldown. */
	UPROPERTY(EditAnywhere, Category = "Ability")
	bool bUseOrderTag = true;

	/** Used when bUseOrderTag == false (stance-suggested or autonomous activation). */
	UPROPERTY(EditAnywhere, Category = "Ability")
	FGameplayTag AbilityTag;
};
