#include "AI/BTService_UpdateFacing.h"

#include "AI/SFCompanionAIController.h"
#include "AI/SFCompanionBlackboardKeys.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Companions/SFCompanionCharacter.h"
#include "Companions/SFCompanionTypes.h"
#include "GameFramework/CharacterMovementComponent.h"

UBTService_UpdateFacing::UBTService_UpdateFacing()
{
	NodeName = TEXT("Update Facing");
	bNotifyBecomeRelevant = true;
	bNotifyTick = true;
	Interval = 0.2f;
	RandomDeviation = 0.02f;
}

void UBTService_UpdateFacing::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	ASFCompanionAIController* AIC = Cast<ASFCompanionAIController>(OwnerComp.GetAIOwner());
	if (!BB || !AIC)
	{
		return;
	}

	ASFCompanionCharacter* Companion = AIC->GetControlledCompanion();
	if (!Companion)
	{
		return;
	}

	UCharacterMovementComponent* Move = Companion->GetCharacterMovement();
	if (!Move)
	{
		return;
	}

	AActor* Focus = Cast<AActor>(BB->GetValueAsObject(FocusTargetKey));
	const ESFCompanionOrderType OrderType =
		static_cast<ESFCompanionOrderType>(SFCompanionBlackboardKeys::GetEnumOrInt(BB, OrderTypeKey));

	const bool bInCombat = Focus != nullptr
		|| OrderType == ESFCompanionOrderType::AttackTarget
		|| OrderType == ESFCompanionOrderType::FocusFire;

	if (bInCombat)
	{
		Move->bOrientRotationToMovement = false;
		Move->bUseControllerDesiredRotation = true;
		Move->RotationRate = FRotator(0.f, CombatYawRate, 0.f);
		AIC->SetFocus(Focus);
	}
	else
	{
		Move->bOrientRotationToMovement = true;
		Move->bUseControllerDesiredRotation = false;
		Move->RotationRate = FRotator(0.f, MovementYawRate, 0.f);
		AIC->ClearFocus(EAIFocusPriority::Gameplay);
	}
}
