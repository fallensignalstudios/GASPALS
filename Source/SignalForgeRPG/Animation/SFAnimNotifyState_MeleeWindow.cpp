#include "Animation/SFAnimNotifyState_MeleeWindow.h"

#include "Characters/SFCharacterBase.h"
#include "Combat/SFCombatComponent.h"
#include "Components/SkeletalMeshComponent.h"

namespace SFMeleeWindowDetail
{
	static USFCombatComponent* GetCombatComponent(USkeletalMeshComponent* MeshComp)
	{
		if (!MeshComp)
		{
			return nullptr;
		}
		ASFCharacterBase* Character = Cast<ASFCharacterBase>(MeshComp->GetOwner());
		return Character ? Character->FindComponentByClass<USFCombatComponent>() : nullptr;
	}

	static void RunHitDetection(USFCombatComponent* Combat, ESFMeleeWindowAttackType AttackType)
	{
		if (!Combat) return;
		switch (AttackType)
		{
		case ESFMeleeWindowAttackType::Heavy:
			Combat->HandleHeavyAttackHit();
			break;
		case ESFMeleeWindowAttackType::Light:
		default:
			Combat->HandleLightAttackHit();
			break;
		}
	}
}

void USFAnimNotifyState_MeleeWindow::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	USFCombatComponent* Combat = SFMeleeWindowDetail::GetCombatComponent(MeshComp);
	if (!Combat) return;

	// Open the window (resets HitActorsThisAttack so this swing can hit new targets).
	Combat->BeginAttackWindow();

	// Fire one detection on the opening frame so single-frame "instant" swings still register
	// even if bTickHitDetection is false. Per-frame tick will continue if enabled.
	SFMeleeWindowDetail::RunHitDetection(Combat, AttackType);
}

void USFAnimNotifyState_MeleeWindow::NotifyTick(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!bTickHitDetection)
	{
		return;
	}

	USFCombatComponent* Combat = SFMeleeWindowDetail::GetCombatComponent(MeshComp);
	if (!Combat) return;

	// HitActorsThisAttack dedupes per-window so multi-frame sweeps don't double-tap targets.
	SFMeleeWindowDetail::RunHitDetection(Combat, AttackType);
}

void USFAnimNotifyState_MeleeWindow::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	USFCombatComponent* Combat = SFMeleeWindowDetail::GetCombatComponent(MeshComp);
	if (Combat)
	{
		Combat->EndAttackWindow();
	}
}

FString USFAnimNotifyState_MeleeWindow::GetNotifyName_Implementation() const
{
	return (AttackType == ESFMeleeWindowAttackType::Heavy)
		? TEXT("Melee Window (Heavy)")
		: TEXT("Melee Window (Light)");
}
