#include "AI/BehaviorTree/SFBTTask_FireWeapon.h"

#include "AI/BehaviorTree/SFBTHelpers.h"
#include "AIController.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Characters/SFCharacterBase.h"
#include "Core/SignalForgeGameplayTags.h"

USFBTTask_FireWeapon::USFBTTask_FireWeapon()
{
	NodeName = TEXT("SF Fire Weapon");
	bNotifyTick = true;
}

EBTNodeResult::Type USFBTTask_FireWeapon::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AI = OwnerComp.GetAIOwner();
	ASFCharacterBase* Character = SFBTHelpers::GetControlledCharacter(AI);
	if (!Character || !Character->GetAbilitySystemComponent())
	{
		return EBTNodeResult::Failed;
	}

	FMemory* Mem = reinterpret_cast<FMemory*>(NodeMemory);
	new (Mem) FMemory();

	SendPressed(OwnerComp);
	Mem->bPressed = true;

	switch (FireMode)
	{
	case EFireMode::Tap:
		SendReleased(OwnerComp);
		Mem->bPressed = false;
		return EBTNodeResult::Succeeded;

	case EFireMode::PressOnly:
		// Caller is responsible for releasing via SFBTTask_StopFiring.
		return EBTNodeResult::Succeeded;

	case EFireMode::Hold:
	default:
		// Tick until HoldDuration elapses, then release.
		return EBTNodeResult::InProgress;
	}
}

void USFBTTask_FireWeapon::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	if (FireMode != EFireMode::Hold)
	{
		return;
	}

	FMemory* Mem = reinterpret_cast<FMemory*>(NodeMemory);
	Mem->ElapsedSeconds += DeltaSeconds;
	if (Mem->ElapsedSeconds >= HoldDuration)
	{
		SendReleased(OwnerComp);
		Mem->bPressed = false;
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type USFBTTask_FireWeapon::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FMemory* Mem = reinterpret_cast<FMemory*>(NodeMemory);
	// Always release input on abort so we don't leave the weapon stuck firing.
	if (Mem && Mem->bPressed)
	{
		SendReleased(OwnerComp);
		Mem->bPressed = false;
	}
	return EBTNodeResult::Aborted;
}

FString USFBTTask_FireWeapon::GetStaticDescription() const
{
	const TCHAR* ModeStr = TEXT("Tap");
	switch (FireMode)
	{
	case EFireMode::Hold:      ModeStr = TEXT("Hold"); break;
	case EFireMode::PressOnly: ModeStr = TEXT("PressOnly"); break;
	default: break;
	}
	if (FireMode == EFireMode::Hold)
	{
		return FString::Printf(TEXT("Fire (%s, %.2fs)"), ModeStr, HoldDuration);
	}
	return FString::Printf(TEXT("Fire (%s)"), ModeStr);
}

void USFBTTask_FireWeapon::SendPressed(UBehaviorTreeComponent& OwnerComp)
{
	ASFCharacterBase* Character = SFBTHelpers::GetControlledCharacter(OwnerComp.GetAIOwner());
	if (!Character) { return; }

	USFAbilitySystemComponent* ASC = Cast<USFAbilitySystemComponent>(Character->GetAbilitySystemComponent());
	if (!ASC) { return; }

	ASC->AbilityInputTagPressed(FSignalForgeGameplayTags::Get().Input_PrimaryFire);
}

void USFBTTask_FireWeapon::SendReleased(UBehaviorTreeComponent& OwnerComp)
{
	ASFCharacterBase* Character = SFBTHelpers::GetControlledCharacter(OwnerComp.GetAIOwner());
	if (!Character) { return; }

	USFAbilitySystemComponent* ASC = Cast<USFAbilitySystemComponent>(Character->GetAbilitySystemComponent());
	if (!ASC) { return; }

	ASC->AbilityInputTagReleased(FSignalForgeGameplayTags::Get().Input_PrimaryFire);
}
