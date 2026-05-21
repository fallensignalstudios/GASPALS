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

	// If the designer wants an ADS lead-in, press ADS first and wait for it to settle before
	// pulling the trigger. Otherwise jump straight to firing.
	if (AdsLeadInSeconds > 0.0f)
	{
		SendAdsPressed(OwnerComp);
		Mem->bAdsPressed = true;
		Mem->Phase = EFirePhase::AdsRamp;
		Mem->ElapsedSeconds = 0.0f;
		return EBTNodeResult::InProgress;
	}

	return BeginFiring(OwnerComp, *Mem);
}

void USFBTTask_FireWeapon::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FMemory* Mem = reinterpret_cast<FMemory*>(NodeMemory);
	if (!Mem)
	{
		return;
	}

	Mem->ElapsedSeconds += DeltaSeconds;

	switch (Mem->Phase)
	{
	case EFirePhase::AdsRamp:
	{
		if (Mem->ElapsedSeconds >= AdsLeadInSeconds)
		{
			// ADS has settled -- start the actual shot. Reset elapsed for the Hold-mode timer.
			Mem->ElapsedSeconds = 0.0f;
			const EBTNodeResult::Type FireResult = BeginFiring(OwnerComp, *Mem);
			if (FireResult != EBTNodeResult::InProgress)
			{
				// Tap / PressOnly are immediate -- release ADS and finish.
				if (Mem->bAdsPressed)
				{
					SendAdsReleased(OwnerComp);
					Mem->bAdsPressed = false;
				}
				Mem->Phase = EFirePhase::Done;
				FinishLatentTask(OwnerComp, FireResult);
			}
		}
		break;
	}

	case EFirePhase::Holding:
	{
		if (Mem->ElapsedSeconds >= HoldDuration)
		{
			if (Mem->bFirePressed)
			{
				SendFireReleased(OwnerComp);
				Mem->bFirePressed = false;
			}
			if (Mem->bAdsPressed)
			{
				SendAdsReleased(OwnerComp);
				Mem->bAdsPressed = false;
			}
			Mem->Phase = EFirePhase::Done;
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
		break;
	}

	case EFirePhase::Done:
	default:
		break;
	}
}

EBTNodeResult::Type USFBTTask_FireWeapon::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FMemory* Mem = reinterpret_cast<FMemory*>(NodeMemory);
	// Always release any held inputs on abort so we don't leave the weapon stuck firing or
	// the AI permanently scoped in.
	if (Mem)
	{
		if (Mem->bFirePressed)
		{
			SendFireReleased(OwnerComp);
			Mem->bFirePressed = false;
		}
		if (Mem->bAdsPressed)
		{
			SendAdsReleased(OwnerComp);
			Mem->bAdsPressed = false;
		}
	}
	return EBTNodeResult::Aborted;
}

FString USFBTTask_FireWeapon::GetStaticDescription() const
{
	const TCHAR* ModeStr = TEXT("Tap");
	switch (FireMode)
	{
	case ESFBTFireMode::Hold:      ModeStr = TEXT("Hold"); break;
	case ESFBTFireMode::PressOnly: ModeStr = TEXT("PressOnly"); break;
	default: break;
	}

	const FString AdsStr = AdsLeadInSeconds > 0.0f
		? FString::Printf(TEXT(", ADS %.2fs"), AdsLeadInSeconds)
		: FString();

	if (FireMode == ESFBTFireMode::Hold)
	{
		return FString::Printf(TEXT("Fire (%s, %.2fs%s)"), ModeStr, HoldDuration, *AdsStr);
	}
	return FString::Printf(TEXT("Fire (%s%s)"), ModeStr, *AdsStr);
}

EBTNodeResult::Type USFBTTask_FireWeapon::BeginFiring(UBehaviorTreeComponent& OwnerComp, FMemory& Mem)
{
	SendFirePressed(OwnerComp);
	Mem.bFirePressed = true;

	switch (FireMode)
	{
	case ESFBTFireMode::Tap:
		// Single shot: release immediately, succeed. Caller will release ADS if it was pressed.
		SendFireReleased(OwnerComp);
		Mem.bFirePressed = false;
		Mem.Phase = EFirePhase::Done;
		return EBTNodeResult::Succeeded;

	case ESFBTFireMode::PressOnly:
		// External StopFiring task is responsible for the release.
		Mem.Phase = EFirePhase::Done;
		return EBTNodeResult::Succeeded;

	case ESFBTFireMode::Hold:
	default:
		Mem.Phase = EFirePhase::Holding;
		return EBTNodeResult::InProgress;
	}
}

void USFBTTask_FireWeapon::SendFirePressed(UBehaviorTreeComponent& OwnerComp)
{
	ASFCharacterBase* Character = SFBTHelpers::GetControlledCharacter(OwnerComp.GetAIOwner());
	if (!Character) { return; }

	USFAbilitySystemComponent* ASC = Cast<USFAbilitySystemComponent>(Character->GetAbilitySystemComponent());
	if (!ASC) { return; }

	ASC->AbilityInputTagPressed(FSignalForgeGameplayTags::Get().Input_PrimaryFire);
}

void USFBTTask_FireWeapon::SendFireReleased(UBehaviorTreeComponent& OwnerComp)
{
	ASFCharacterBase* Character = SFBTHelpers::GetControlledCharacter(OwnerComp.GetAIOwner());
	if (!Character) { return; }

	USFAbilitySystemComponent* ASC = Cast<USFAbilitySystemComponent>(Character->GetAbilitySystemComponent());
	if (!ASC) { return; }

	ASC->AbilityInputTagReleased(FSignalForgeGameplayTags::Get().Input_PrimaryFire);
}

void USFBTTask_FireWeapon::SendAdsPressed(UBehaviorTreeComponent& OwnerComp)
{
	ASFCharacterBase* Character = SFBTHelpers::GetControlledCharacter(OwnerComp.GetAIOwner());
	if (!Character) { return; }

	USFAbilitySystemComponent* ASC = Cast<USFAbilitySystemComponent>(Character->GetAbilitySystemComponent());
	if (!ASC) { return; }

	ASC->AbilityInputTagPressed(FSignalForgeGameplayTags::Get().Input_ADS);
}

void USFBTTask_FireWeapon::SendAdsReleased(UBehaviorTreeComponent& OwnerComp)
{
	ASFCharacterBase* Character = SFBTHelpers::GetControlledCharacter(OwnerComp.GetAIOwner());
	if (!Character) { return; }

	USFAbilitySystemComponent* ASC = Cast<USFAbilitySystemComponent>(Character->GetAbilitySystemComponent());
	if (!ASC) { return; }

	ASC->AbilityInputTagReleased(FSignalForgeGameplayTags::Get().Input_ADS);
}
