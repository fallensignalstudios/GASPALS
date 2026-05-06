#include "Combat/SFCombatComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/SFCharacterBase.h"
#include "Characters/SFEnemyCharacter.h"
#include "Combat/SFWeaponActor.h"
#include "Components/SFEquipmentComponent.h"
#include "Core/SFAttributeSetBase.h"
#include "Core/SignalForgeGameplayTags.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "Input/SFPlayerController.h"
#include "SFHitResolverInterface.h"
#include "SFHitTypes.h"

USFCombatComponent::USFCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USFCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ASFCharacterBase>(GetOwner());
}

void USFCombatComponent::BeginAttackWindow()
{
	HitActorsThisAttack.Reset();
}

void USFCombatComponent::EndAttackWindow()
{
	HitActorsThisAttack.Reset();
}

void USFCombatComponent::HandleLightAttackHit()
{
	HandleAttackHitInternal(ESFAttackType::Light);
}

void USFCombatComponent::HandleHeavyAttackHit()
{
	HandleAttackHitInternal(ESFAttackType::Heavy);
}

void USFCombatComponent::HandleAttackHitInternal(ESFAttackType AttackType)
{
	if (!OwnerCharacter || OwnerCharacter->IsDead())
	{
		return;
	}

	// Choose trace parameters based on attack type
	float StartOffset = 0.f;
	float TraceLength = 0.f;
	float TraceRadius = 0.f;

	switch (AttackType)
	{
	case ESFAttackType::Light:
		StartOffset = LightAttackTraceStartOffset;
		TraceLength = LightAttackTraceLength;
		TraceRadius = LightAttackTraceRadius;
		break;

	case ESFAttackType::Heavy:
	default:
		StartOffset = HeavyAttackTraceStartOffset;
		TraceLength = HeavyAttackTraceLength;
		TraceRadius = HeavyAttackTraceRadius;
		break;
	}

	TArray<FHitResult> HitResults;
	if (!PerformAttackSweep(StartOffset, TraceLength, TraceRadius, HitResults))
	{
		return;
	}

	for (const FHitResult& HitResult : HitResults)
	{
		AActor* HitActor = HitResult.GetActor();
		if (!HitActor || HitActorsThisAttack.Contains(HitActor))
		{
			continue;
		}

		// Skip dead characters
		if (ASFCharacterBase* HitCharacter = Cast<ASFCharacterBase>(HitActor))
		{
			if (HitCharacter->IsDead())
			{
				continue;
			}
		}

		FSFHitData HitData = BuildHitDataFromResult(HitActor, HitResult, AttackType);

		FSFResolvedHit ResolvedHit;
		if (HitActor->GetClass()->ImplementsInterface(USFHitResolverInterface::StaticClass()))
		{
			ResolvedHit = ISFHitResolverInterface::Execute_ResolveIncomingHit(HitActor, HitData);
		}
		else
		{
			// Simple default: straight hit if no resolver implemented
			ResolvedHit.Outcome = ESFHitOutcome::Hit;
			ResolvedHit.HealthDamage = 20.f;
		}

		if (ResolvedHit.Outcome == ESFHitOutcome::Invalid || ResolvedHit.Outcome == ESFHitOutcome::Immune)
		{
			continue;
		}

		ApplyResolvedHitWithGAS(HitData, ResolvedHit);
		HitActorsThisAttack.Add(HitActor);

		TriggerHitFeedback(HitData, ResolvedHit);
	}
}

bool USFCombatComponent::PerformAttackSweep(float StartOffset, float TraceLength, float TraceRadius, TArray<FHitResult>& OutHitResults) const
{
	OutHitResults.Reset();

	if (!OwnerCharacter || OwnerCharacter->IsDead())
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;

	const bool bUsingWeaponTrace = GetWeaponTracePoints(Start, End);
	if (!bUsingWeaponTrace)
	{
		Start = OwnerCharacter->GetActorLocation() + (OwnerCharacter->GetActorForwardVector() * StartOffset);
		End = Start + (OwnerCharacter->GetActorForwardVector() * TraceLength);
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CombatSweep), false);
	QueryParams.AddIgnoredActor(OwnerCharacter);

	if (USFEquipmentComponent* EquipmentComponent = OwnerCharacter->GetEquipmentComponent())
	{
		if (ASFWeaponActor* WeaponActor = EquipmentComponent->GetEquippedWeaponActor())
		{
			QueryParams.AddIgnoredActor(WeaponActor);
		}
	}

	const bool bHitAnything = World->SweepMultiByChannel(
		OutHitResults,
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(TraceRadius),
		QueryParams
	);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	// Debug visualization
	const FColor LineColor = bHitAnything ? FColor::Red : FColor::Green;
	DrawDebugLine(World, Start, End, LineColor, false, 0.5f, 0, 2.0f);
	DrawDebugSphere(World, Start, TraceRadius, 12, FColor::Blue, false, 0.5f);
	DrawDebugSphere(World, End, TraceRadius, 12, LineColor, false, 0.5f);
#endif

	return bHitAnything && OutHitResults.Num() > 0;
}

FSFHitData USFCombatComponent::BuildHitDataFromResult(AActor* HitActor, const FHitResult& HitResult, ESFAttackType AttackType) const
{
	FSFHitData HitData;
	if (!OwnerCharacter || !HitActor)
	{
		return HitData;
	}

	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();

	HitData.SourceActor = OwnerCharacter;
	HitData.TargetActor = HitActor;
	HitData.AttackType = AttackType;
	HitData.HitResult = HitResult;
	HitData.HitDirection = (HitActor->GetActorLocation() - OwnerCharacter->GetActorLocation()).GetSafeNormal();
	HitData.AttackTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	// Encode identity: light vs heavy
	if (AttackType == ESFAttackType::Light)
	{
		HitData.AttackTags.AddTag(Tags.Ability_Attack_Light);
	}
	else if (AttackType == ESFAttackType::Heavy)
	{
		HitData.AttackTags.AddTag(Tags.Ability_Attack_Heavy);
	}

	// Default blockability / poise data; can be overridden by abilities
	HitData.bIsBlockable = true;
	HitData.bIgnoreGuard = false;
	HitData.PoiseDamageScale = 1.0f;
	HitData.BreakGuardBonus = 0.0f;

	return HitData;
}

TSubclassOf<UGameplayEffect> USFCombatComponent::GetDamageEffectForAttackType(ESFAttackType AttackType) const
{
	if (!OwnerCharacter)
	{
		return nullptr;
	}

	switch (AttackType)
	{
	case ESFAttackType::Light:
		return OwnerCharacter->GetLightAttackDamageEffect();

	case ESFAttackType::Heavy:
	default:
		return OwnerCharacter->GetHeavyAttackDamageEffect();
	}
}

void USFCombatComponent::ApplyResolvedHitWithGAS(const FSFHitData& HitData, const FSFResolvedHit& ResolvedHit)
{
	if (!OwnerCharacter || !HitData.TargetActor)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = OwnerCharacter->GetAbilitySystemComponent();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitData.TargetActor);
	if (!SourceASC || !TargetASC)
	{
		return;
	}

	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();

	TSubclassOf<UGameplayEffect> DamageEffect = GetDamageEffectForAttackType(HitData.AttackType);
	if (!DamageEffect)
	{
		return;
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(OwnerCharacter);
	Context.AddHitResult(HitData.HitResult);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffect, 1.0f, Context);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	FGameplayEffectSpec* Spec = SpecHandle.Data.Get();

	// SetByCaller magnitudes using your Data.* tags
	Spec->SetSetByCallerMagnitude(Tags.Data_BaseDamage, ResolvedHit.HealthDamage);
	Spec->SetSetByCallerMagnitude(Tags.Data_PoiseDamageScale, HitData.PoiseDamageScale);

	if (ResolvedHit.bCauseGuardBreak)
	{
		Spec->SetSetByCallerMagnitude(Tags.Data_BreakGuardBonus, HitData.BreakGuardBonus);
	}

	// Apply any result tags (e.g. State.Broken) as dynamic granted tags
	for (const FGameplayTag& Tag : ResolvedHit.ResultTagsOnTarget)
	{
		Spec->DynamicGrantedTags.AddTag(Tag);
	}

	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec, TargetASC);

	// Track enemy for lock‑on / camera logic
	if (ASFCharacterBase* HitCharacter = Cast<ASFCharacterBase>(HitData.TargetActor))
	{
		if (ASFPlayerController* PlayerController = Cast<ASFPlayerController>(OwnerCharacter->GetController()))
		{
			PlayerController->SetTrackedEnemy(HitCharacter);
		}
	}

	// Optional: posture cost for attacker via self‑effect or attribute ops
	if (ResolvedHit.PoiseDamageToSource != 0.f)
	{
		// You can either:
		// 1) Apply a dedicated self effect that reads Data_PoiseDamageScale, or
		// 2) Directly modify a Poise attribute on OwnerCharacter’s attribute set.
	}
}

void USFCombatComponent::TriggerHitFeedback(const FSFHitData& HitData, const FSFResolvedHit& ResolvedHit)
{
	// This function is intentionally left as a hook.
	// You can:
	// - Execute GameplayCues for hits, blocks, parries
	// - Trigger camera shakes
	// - Apply hit stop (time dilation)
	// It’s better to call into a dedicated feedback system than to hard‑code it here.
}

bool USFCombatComponent::GetWeaponTracePoints(FVector& OutStart, FVector& OutEnd) const
{
	if (!OwnerCharacter)
	{
		return false;
	}

	if (USFEquipmentComponent* EquipmentComponent = OwnerCharacter->GetEquipmentComponent())
	{
		if (ASFWeaponActor* WeaponActor = EquipmentComponent->GetEquippedWeaponActor())
		{
			if (WeaponActor->HasValidTraceSockets())
			{
				OutStart = WeaponActor->GetTraceStartLocation();
				OutEnd = WeaponActor->GetTraceEndLocation();
				return true;
			}
		}
	}

	return false;
}