#include "AI/BehaviorTree/SFBTTask_FireWeapon.h"

#include "AI/BehaviorTree/SFBTHelpers.h"
#include "AIController.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Characters/SFCharacterBase.h"
#include "Core/SignalForgeGameplayTags.h"
#include "Core/SignalForgeLogChannels.h"

USFBTTask_FireWeapon::USFBTTask_FireWeapon()
{
	NodeName = TEXT("SF Fire Weapon");
	bNotifyTick = true;
}

void USFBTTask_FireWeapon::ResolveRuntimeTuning(FMemory& Mem) const
{
	// Custom: copy the manually-authored UPROPERTYs straight across.
	if (CombatStyle == ESFCombatStyle::Custom)
	{
		Mem.RuntimeFireMode = FireMode;
		Mem.RuntimeHoldDuration = HoldDuration;
		Mem.RuntimeBurstShotCount = BurstShotCount;
		Mem.RuntimeBurstInterShotDelay = BurstInterShotDelay;
		Mem.bRuntimeRequiresAds = bRequiresAds;
		Mem.RuntimeAdsLeadInSeconds = AdsLeadInSeconds;
		return;
	}

	// Named presets. Tuned to match each preset's reference game feel; all values
	// are designer-overridable per encounter by switching CombatStyle to Custom.
	switch (CombatStyle)
	{
	case ESFCombatStyle::BurstShooter:
		// Halo-grunt: ADS, then 3-shot burst at 100ms cadence.
		Mem.RuntimeFireMode = ESFBTFireMode::Burst;
		Mem.RuntimeHoldDuration = 0.0f;
		Mem.RuntimeBurstShotCount = 3;
		Mem.RuntimeBurstInterShotDelay = 0.10f;
		Mem.bRuntimeRequiresAds = true;
		Mem.RuntimeAdsLeadInSeconds = 0.35f;
		break;

	case ESFCombatStyle::HipFireSpammer:
		// Stormtrooper: no ADS, rapid single taps. Each task activation is one shot;
		// the BT cooldown decorator sets the inter-burst pause.
		Mem.RuntimeFireMode = ESFBTFireMode::Tap;
		Mem.RuntimeHoldDuration = 0.0f;
		Mem.RuntimeBurstShotCount = 1;
		Mem.RuntimeBurstInterShotDelay = 0.0f;
		Mem.bRuntimeRequiresAds = false;
		Mem.RuntimeAdsLeadInSeconds = 0.0f;
		break;

	case ESFCombatStyle::PrecisionMarksman:
		// Destiny-vandal: long ADS settle, one heavy shot, long pull-out.
		Mem.RuntimeFireMode = ESFBTFireMode::Tap;
		Mem.RuntimeHoldDuration = 0.0f;
		Mem.RuntimeBurstShotCount = 1;
		Mem.RuntimeBurstInterShotDelay = 0.0f;
		Mem.bRuntimeRequiresAds = true;
		Mem.RuntimeAdsLeadInSeconds = 0.85f;
		break;

	case ESFCombatStyle::MeleeBrute:
		// Sword elite: no ADS, one solid swing. Inter-swing cadence handled by BT cooldown.
		Mem.RuntimeFireMode = ESFBTFireMode::Tap;
		Mem.RuntimeHoldDuration = 0.0f;
		Mem.RuntimeBurstShotCount = 1;
		Mem.RuntimeBurstInterShotDelay = 0.0f;
		Mem.bRuntimeRequiresAds = false;
		Mem.RuntimeAdsLeadInSeconds = 0.0f;
		break;

	case ESFCombatStyle::MeleeAssassin:
		// Knife / dreg: no ADS, two-swing flurry.
		Mem.RuntimeFireMode = ESFBTFireMode::Burst;
		Mem.RuntimeHoldDuration = 0.0f;
		Mem.RuntimeBurstShotCount = 2;
		Mem.RuntimeBurstInterShotDelay = 0.25f;
		Mem.bRuntimeRequiresAds = false;
		Mem.RuntimeAdsLeadInSeconds = 0.0f;
		break;

	default:
		// Unknown preset (shouldn't happen) -- fall through to authored fields.
		Mem.RuntimeFireMode = FireMode;
		Mem.RuntimeHoldDuration = HoldDuration;
		Mem.RuntimeBurstShotCount = BurstShotCount;
		Mem.RuntimeBurstInterShotDelay = BurstInterShotDelay;
		Mem.bRuntimeRequiresAds = bRequiresAds;
		Mem.RuntimeAdsLeadInSeconds = AdsLeadInSeconds;
		break;
	}
}

EBTNodeResult::Type USFBTTask_FireWeapon::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AI = OwnerComp.GetAIOwner();
	ASFCharacterBase* Character = SFBTHelpers::GetControlledCharacter(AI);
	if (!Character || !Character->GetAbilitySystemComponent())
	{
		UE_LOG(LogSFAI, Warning, TEXT("[SFBTTask_FireWeapon] Execute FAILED: no controlled character or no ASC."));
		return EBTNodeResult::Failed;
	}

	UE_LOG(LogSFAI, Display, TEXT("[SFBTTask_FireWeapon] '%s' Execute (style=%d)."),
		*GetNameSafe(Character), (int32)CombatStyle);

	FMemory* Mem = reinterpret_cast<FMemory*>(NodeMemory);
	new (Mem) FMemory();
	ResolveRuntimeTuning(*Mem);

	// ADS lead-in: only when the resolved style asks for it.
	if (Mem->bRuntimeRequiresAds && Mem->RuntimeAdsLeadInSeconds > 0.0f)
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
		if (Mem->ElapsedSeconds >= Mem->RuntimeAdsLeadInSeconds)
		{
			// ADS has settled -- start the actual shot. Reset elapsed for downstream phase timers.
			Mem->ElapsedSeconds = 0.0f;
			const EBTNodeResult::Type FireResult = BeginFiring(OwnerComp, *Mem);
			if (FireResult != EBTNodeResult::InProgress)
			{
				// Single-shot resolution (Tap / PressOnly): release ADS and finish.
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
		if (Mem->ElapsedSeconds >= Mem->RuntimeHoldDuration)
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

	case EFirePhase::BurstInterShotWait:
	{
		if (Mem->ElapsedSeconds >= Mem->RuntimeBurstInterShotDelay)
		{
			Mem->ElapsedSeconds = 0.0f;
			FireBurstShot(OwnerComp, *Mem);

			if (Mem->BurstShotsFired >= Mem->RuntimeBurstShotCount)
			{
				if (Mem->bAdsPressed)
				{
					SendAdsReleased(OwnerComp);
					Mem->bAdsPressed = false;
				}
				Mem->Phase = EFirePhase::Done;
				FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
			}
			else
			{
				Mem->Phase = EFirePhase::BurstInterShotWait;
			}
		}
		break;
	}

	case EFirePhase::BurstShooting:
		// Transient -- BeginFiring / FireBurstShot moves us to BurstInterShotWait the same frame.
		break;

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
	const TCHAR* StyleStr = TEXT("Custom");
	switch (CombatStyle)
	{
	case ESFCombatStyle::BurstShooter:      StyleStr = TEXT("BurstShooter"); break;
	case ESFCombatStyle::HipFireSpammer:    StyleStr = TEXT("HipFireSpammer"); break;
	case ESFCombatStyle::PrecisionMarksman: StyleStr = TEXT("PrecisionMarksman"); break;
	case ESFCombatStyle::MeleeBrute:        StyleStr = TEXT("MeleeBrute"); break;
	case ESFCombatStyle::MeleeAssassin:     StyleStr = TEXT("MeleeAssassin"); break;
	default: break;
	}

	if (CombatStyle != ESFCombatStyle::Custom)
	{
		return FString::Printf(TEXT("Fire (%s preset)"), StyleStr);
	}

	const TCHAR* ModeStr = TEXT("Tap");
	switch (FireMode)
	{
	case ESFBTFireMode::Hold:      ModeStr = TEXT("Hold"); break;
	case ESFBTFireMode::PressOnly: ModeStr = TEXT("PressOnly"); break;
	case ESFBTFireMode::Burst:     ModeStr = TEXT("Burst"); break;
	default: break;
	}

	const FString AdsStr = (bRequiresAds && AdsLeadInSeconds > 0.0f)
		? FString::Printf(TEXT(", ADS %.2fs"), AdsLeadInSeconds)
		: FString();

	if (FireMode == ESFBTFireMode::Hold)
	{
		return FString::Printf(TEXT("Fire (Custom, %s, %.2fs%s)"), ModeStr, HoldDuration, *AdsStr);
	}
	if (FireMode == ESFBTFireMode::Burst)
	{
		return FString::Printf(TEXT("Fire (Custom, %s x%d @ %.2fs%s)"), ModeStr, BurstShotCount, BurstInterShotDelay, *AdsStr);
	}
	return FString::Printf(TEXT("Fire (Custom, %s%s)"), ModeStr, *AdsStr);
}

EBTNodeResult::Type USFBTTask_FireWeapon::BeginFiring(UBehaviorTreeComponent& OwnerComp, FMemory& Mem)
{
	switch (Mem.RuntimeFireMode)
	{
	case ESFBTFireMode::Tap:
		SendFirePressed(OwnerComp);
		Mem.bFirePressed = true;
		SendFireReleased(OwnerComp);
		Mem.bFirePressed = false;
		Mem.Phase = EFirePhase::Done;
		return EBTNodeResult::Succeeded;

	case ESFBTFireMode::PressOnly:
		SendFirePressed(OwnerComp);
		Mem.bFirePressed = true;
		// External StopFiring task is responsible for the release.
		Mem.Phase = EFirePhase::Done;
		return EBTNodeResult::Succeeded;

	case ESFBTFireMode::Hold:
		SendFirePressed(OwnerComp);
		Mem.bFirePressed = true;
		Mem.Phase = EFirePhase::Holding;
		return EBTNodeResult::InProgress;

	case ESFBTFireMode::Burst:
	{
		Mem.BurstShotsFired = 0;
		FireBurstShot(OwnerComp, Mem);

		if (Mem.BurstShotsFired >= Mem.RuntimeBurstShotCount)
		{
			// 1-shot "burst" -- single-tap behavior.
			Mem.Phase = EFirePhase::Done;
			return EBTNodeResult::Succeeded;
		}
		Mem.ElapsedSeconds = 0.0f;
		Mem.Phase = EFirePhase::BurstInterShotWait;
		return EBTNodeResult::InProgress;
	}

	default:
		Mem.Phase = EFirePhase::Done;
		return EBTNodeResult::Failed;
	}
}

void USFBTTask_FireWeapon::FireBurstShot(UBehaviorTreeComponent& OwnerComp, FMemory& Mem)
{
	// One press+release pair = one shot, regardless of the underlying weapon's
	// auto-fire policy. Semi-auto weapons treat this as one trigger pull; full-auto
	// weapons treat the brief tap as one shot too because of their min-press-time
	// budget. Beam / charge weapons should use Hold mode instead.
	SendFirePressed(OwnerComp);
	Mem.bFirePressed = true;
	SendFireReleased(OwnerComp);
	Mem.bFirePressed = false;
	++Mem.BurstShotsFired;
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
