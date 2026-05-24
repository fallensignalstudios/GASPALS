#include "Combat/SFCombatComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Camera/CameraShakeBase.h"
#include "GameFramework/Character.h"
#include "Characters/SFCharacterBase.h"
#include "Characters/SFEnemyCharacter.h"
#include "Combat/SFHitReactionComponent.h"
#include "Combat/SFWeaponActor.h"
#include "Combat/SFWeaponData.h"
#include "Components/SFEquipmentComponent.h"
#include "Faction/SFFactionStatics.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/SFAttributeSetBase.h"
#include "Core/SignalForgeGameplayTags.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameplayCueManager.h"
#include "GameplayEffect.h"
#include "Input/SFPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "SFHitResolverInterface.h"
#include "SFHitTypes.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogSFMeleeDamage, Log, All);

USFCombatComponent::USFCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Sensible defaults for the two feedback profiles. Heavy attacks freeze
	// harder, shake more, and run a touch longer of a slow-mo on critical.
	LightFeedback.HitStopTimeDilation = 0.10f;
	LightFeedback.HitStopDuration = 0.06f;
	LightFeedback.CameraShakeScale = 0.85f;

	HeavyFeedback.HitStopTimeDilation = 0.04f;
	HeavyFeedback.HitStopDuration = 0.10f;
	HeavyFeedback.CameraShakeScale = 1.35f;
	HeavyFeedback.SlowMoTimeDilation = 0.25f;
	HeavyFeedback.SlowMoDuration = 0.30f;

	// Common humanoid weakpoints. Designers can override per-character.
	WeakpointBoneNames = { TEXT("head"), TEXT("neck") };
}

void USFCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ASFCharacterBase>(GetOwner());
}

void USFCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Make sure we never leave the world stuck at 0.05 time dilation if the
	// component dies mid hit-stop. This is the kind of bug that kills demos.
	if (UWorld* World = GetWorld())
	{
		if (HitStopTimerHandle.IsValid())
		{
			World->GetTimerManager().ClearTimer(HitStopTimerHandle);
			ClearHitStop();
		}
		if (SlowMoTimerHandle.IsValid())
		{
			World->GetTimerManager().ClearTimer(SlowMoTimerHandle);
			ClearSlowMo();
		}
	}

	Super::EndPlay(EndPlayReason);
}

void USFCombatComponent::BeginAttackWindow()
{
	UE_LOG(LogSFMeleeDamage, Verbose, TEXT("BeginAttackWindow: owner=%s comboIdx=%d"),
		OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("NULL"),
		CurrentComboIndex);
	HitActorsThisAttack.Reset();

	// Combo chain bookkeeping: if the player threw another attack within
	// ComboWindowSeconds of the previous one, advance the chain. Otherwise
	// reset to 0. This lets anim notifies / abilities read GetCurrentComboIndex
	// to pick the right montage slot or transition.
	if (const UWorld* World = GetWorld())
	{
		const float NowReal = World->GetRealTimeSeconds();
		if (NowReal - LastAttackRealTime <= ComboWindowSeconds)
		{
			CurrentComboIndex = (CurrentComboIndex + 1) % FMath::Max(1, MaxComboLength);
		}
		else
		{
			CurrentComboIndex = 0;
		}
		LastAttackRealTime = NowReal;
	}
}

void USFCombatComponent::EndAttackWindow()
{
	HitActorsThisAttack.Reset();
}

void USFCombatComponent::ResetCombo()
{
	CurrentComboIndex = 0;
	LastAttackRealTime = -100.f;
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
		UE_LOG(LogSFMeleeDamage, Verbose, TEXT("HandleAttackHitInternal: owner=%s dead=%d \u2014 skipping"),
			OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("NULL"),
			OwnerCharacter ? (int32)OwnerCharacter->IsDead() : -1);
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
	const bool bSwept = PerformAttackSweep(StartOffset, TraceLength, TraceRadius, HitResults);
	UE_LOG(LogSFMeleeDamage, Log, TEXT("HandleAttackHitInternal: attack=%s sweepHit=%d numHits=%d"),
		(AttackType == ESFAttackType::Heavy) ? TEXT("Heavy") : TEXT("Light"),
		bSwept ? 1 : 0,
		HitResults.Num());
	if (!bSwept)
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

		// Skip dead characters and non-hostile targets (friend-foe gate).
		if (ASFCharacterBase* HitCharacter = Cast<ASFCharacterBase>(HitActor))
		{
			if (HitCharacter->IsDead())
			{
				continue;
			}

			// Faction friend-foe check: only damage hostile characters. Self-hits
			// are also rejected here (faction statics returns Friendly for self).
			if (OwnerCharacter && !USFFactionStatics::AreHostile(OwnerCharacter, HitCharacter))
			{
				continue;
			}

			// Attribute the eventual kill to OwnerCharacter so HandleDeath grants XP.
			HitCharacter->SetLastDamagingCharacter(OwnerCharacter);
		}

		FSFHitData HitData = BuildHitDataFromResult(HitActor, HitResult, AttackType);

		FSFResolvedHit ResolvedHit;
		if (HitActor->GetClass()->ImplementsInterface(USFHitResolverInterface::StaticClass()))
		{
			ResolvedHit = ISFHitResolverInterface::Execute_ResolveIncomingHit(HitActor, HitData);
		}
		else
		{
			// Attribute-driven defaults instead of a magic 20.f. Resolvers can
			// still override anything they care about; this is the floor.
			ResolvedHit.Outcome = ESFHitOutcome::Hit;
			ResolvedHit.HealthDamage = ResolveBaseMeleeDamage(AttackType);
		}

		// Layer in crit / weakpoint scaling regardless of resolver path.
		if (HitData.bIsWeakpointHit)
		{
			ResolvedHit.DamageMultiplier *= WeakpointDamageMultiplier;
			ResolvedHit.ResultTagsOnTarget.AddTag(FSignalForgeGameplayTags::Get().Cue_Hit_Weakpoint);
		}
		ResolvedHit.HealthDamage *= ResolvedHit.DamageMultiplier;

		if (ResolvedHit.Outcome == ESFHitOutcome::Invalid || ResolvedHit.Outcome == ESFHitOutcome::Immune)
		{
			continue;
		}

		ApplyResolvedHitWithGAS(HitData, ResolvedHit);
		HitActorsThisAttack.Add(HitActor);

		TriggerHitFeedback(HitData, ResolvedHit);

		OnHitDelivered.Broadcast(HitData, ResolvedHit);

		// Drive the procedural hit reaction on the TARGET (not us, the attacker).
		// We do this *after* GAS application + feedback so any state tags applied
		// by the resolver (HitReact.Direction.*, State.Stagger, etc.) are already
		// live on the target's ASC when the component reads them.
		if (HitActor)
		{
			if (USFHitReactionComponent* HRC = HitActor->FindComponentByClass<USFHitReactionComponent>())
			{
				HRC->HandleIncomingHit(HitData, ResolvedHit);
			}
		}
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
	QueryParams.bReturnPhysicalMaterial = true;
	QueryParams.bTraceComplex = true; // need complex to get bone names on skel meshes
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
	HitData.ComboIndex = CurrentComboIndex;
	HitData.bIsWeakpointHit = IsWeakpointBone(HitResult.BoneName);

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
	HitData.PoiseDamageScale = (AttackType == ESFAttackType::Heavy) ? 1.5f : 1.0f;
	HitData.BreakGuardBonus = 0.0f;

	return HitData;
}

float USFCombatComponent::ResolveBaseMeleeDamage(ESFAttackType AttackType) const
{
	// Resolution order, most specific to least specific:
	//   1. Equipped weapon's MeleeConfig.LightComboDamages / HeavyComboDamages indexed by current
	//      combo step. Lets designers ramp damage across the chain (jab \u2192 jab \u2192 finisher).
	//      If the array is shorter than the combo length, the last entry repeats.
	//   2. Equipped weapon's RangedConfig.BaseDamage \u2014 the only flat "base damage" slot the
	//      USFWeaponData asset exposes. Designers tend to drop the desired number there for both
	//      ranged and melee. Multiplied by Light/HeavyBaseDamageMultiplier so a heavy still hits
	//      harder than a light when both pull from the same weapon stat.
	//   3. Component-level Light/HeavyDefaultDamage as the final floor for unarmed swings or NPCs
	//      without an equipped weapon.
	if (OwnerCharacter)
	{
		if (USFEquipmentComponent* Equipment = OwnerCharacter->GetEquipmentComponent())
		{
			if (USFWeaponData* WeaponData = Equipment->GetCurrentWeaponData())
			{
				const TArray<float>& ComboDamages = (AttackType == ESFAttackType::Heavy)
					? WeaponData->MeleeConfig.HeavyComboDamages
					: WeaponData->MeleeConfig.LightComboDamages;
				if (ComboDamages.Num() > 0)
				{
					const int32 Idx = FMath::Clamp(CurrentComboIndex, 0, ComboDamages.Num() - 1);
					return ComboDamages[Idx];
				}

				if (WeaponData->RangedConfig.BaseDamage > 0.f)
				{
					const float Multiplier = (AttackType == ESFAttackType::Heavy)
						? HeavyBaseDamageMultiplier
						: LightBaseDamageMultiplier;
					return WeaponData->RangedConfig.BaseDamage * Multiplier;
				}
			}
		}
	}

	return (AttackType == ESFAttackType::Heavy) ? HeavyDefaultDamage : LightDefaultDamage;
}

TSubclassOf<UGameplayEffect> USFCombatComponent::GetDamageEffectForAttackType(ESFAttackType AttackType) const
{
	if (!OwnerCharacter)
	{
		return nullptr;
	}

	// Prefer the equipped weapon's damage GE when set. USFWeaponData only exposes one
	// TSubclassOf<UGameplayEffect> for damage (RangedConfig.DamageEffectClass), so we read from
	// there for both ranged and melee \u2014 designers configuring a sword tend to drop their kinetic
	// GE in that same slot. Falls back to the character's Light/HeavyAttackDamageEffect for
	// unarmed swings or NPCs that don't equip a weapon.
	if (USFEquipmentComponent* Equipment = OwnerCharacter->GetEquipmentComponent())
	{
		if (USFWeaponData* WeaponData = Equipment->GetCurrentWeaponData())
		{
			if (WeaponData->RangedConfig.DamageEffectClass)
			{
				return WeaponData->RangedConfig.DamageEffectClass;
			}
		}
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
		UE_LOG(LogSFMeleeDamage, Warning,
			TEXT("ApplyResolvedHitWithGAS: no DamageEffect resolved for %s attack on %s. Set USFWeaponData::DamageEffectClass on the equipped weapon asset (preferred) or LightAttackDamageEffect/HeavyAttackDamageEffect on the character BP."),
			(HitData.AttackType == ESFAttackType::Heavy) ? TEXT("Heavy") : TEXT("Light"),
			HitData.TargetActor ? *HitData.TargetActor->GetName() : TEXT("NULL"));
		return;
	}

	UE_LOG(LogSFMeleeDamage, Verbose,
		TEXT("ApplyResolvedHitWithGAS: target=%s damage=%.1f effect=%s outcome=%d weakpoint=%d"),
		HitData.TargetActor ? *HitData.TargetActor->GetName() : TEXT("NULL"),
		ResolvedHit.HealthDamage,
		*DamageEffect->GetName(),
		(int32)ResolvedHit.Outcome,
		HitData.bIsWeakpointHit ? 1 : 0);

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

	// Weakpoint flag so the damage execution can apply additional bonuses on its side.
	if (HitData.bIsWeakpointHit)
	{
		Spec->SetSetByCallerMagnitude(Tags.Data_IsWeakpointHit, 1.f);
	}

	// Apply any result tags (e.g. State.Broken, HitReact.Direction.*) as dynamic granted tags
	for (const FGameplayTag& Tag : ResolvedHit.ResultTagsOnTarget)
	{
		Spec->DynamicGrantedTags.AddTag(Tag);
	}

	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec, TargetASC);

	// Track enemy for lock-on / camera logic. Only lock onto hostile targets so
	// allies / civilians don't get framed as enemies in the player's camera.
	if (ASFCharacterBase* HitCharacter = Cast<ASFCharacterBase>(HitData.TargetActor))
	{
		if (OwnerCharacter && USFFactionStatics::AreHostile(OwnerCharacter, HitCharacter))
		{
			if (ASFPlayerController* PlayerController = Cast<ASFPlayerController>(OwnerCharacter->GetController()))
			{
				PlayerController->SetTrackedEnemy(HitCharacter);
			}
		}
	}

	// Self-poise damage (attacker takes posture cost from heavy weapons / blocked hits).
	if (ResolvedHit.PoiseDamageToSource != 0.f && SourceASC)
	{
		FGameplayEffectContextHandle SelfContext = SourceASC->MakeEffectContext();
		SelfContext.AddSourceObject(OwnerCharacter);
		FGameplayEffectSpecHandle SelfSpec = SourceASC->MakeOutgoingSpec(DamageEffect, 1.0f, SelfContext);
		if (SelfSpec.IsValid())
		{
			SelfSpec.Data->SetSetByCallerMagnitude(Tags.Data_BaseDamage, 0.f);
			SelfSpec.Data->SetSetByCallerMagnitude(Tags.Data_PoiseDamageScale, ResolvedHit.PoiseDamageToSource);
			SourceASC->ApplyGameplayEffectSpecToSelf(*SelfSpec.Data.Get());
		}
	}
}

void USFCombatComponent::TriggerHitFeedback(const FSFHitData& HitData, const FSFResolvedHit& ResolvedHit)
{
	if (!HitData.TargetActor || !OwnerCharacter)
	{
		return;
	}

	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
	const FSFCinematicFeedback& Feedback = GetFeedbackForAttackType(HitData.AttackType);

	// Compute hit reaction tags so anim graphs can pick the right montage.
	FGameplayTag DirectionTag = ComputeDirectionalHitTag(HitData);
	FGameplayTag SeverityTag = ComputeSeverityHitTag(HitData, ResolvedHit);

	// Intensity scales feedback up for crits/weakpoints/guard-breaks. Anything
	// that should *feel* bigger gets a multiplier; finishers double on top.
	float IntensityScale = 1.f;
	const bool bIsCrit = HitData.bIsWeakpointHit || ResolvedHit.DamageMultiplier > 1.25f;
	if (bIsCrit)
	{
		IntensityScale *= CriticalFeedbackMultiplier;
	}
	if (ResolvedHit.bCauseGuardBreak)
	{
		IntensityScale *= 1.5f;
	}
	if (ResolvedHit.bIsFinisher)
	{
		IntensityScale *= 2.0f;
	}

	// -------- GameplayCue dispatch (target ASC handles VFX + SFX + decals). --------
	const FVector HitLocation = HitData.HitResult.ImpactPoint.IsNearlyZero()
		? HitData.TargetActor->GetActorLocation()
		: FVector(HitData.HitResult.ImpactPoint);

	switch (ResolvedHit.Outcome)
	{
	case ESFHitOutcome::Blocked:
		ExecuteHitCue(HitData.TargetActor, Tags.Cue_Hit_Block, HitData, IntensityScale);
		break;

	case ESFHitOutcome::PerfectParry:
		ExecuteHitCue(HitData.TargetActor, Tags.Cue_Hit_Parry, HitData, IntensityScale);
		break;

	case ESFHitOutcome::Graze:
		ExecuteHitCue(HitData.TargetActor, Tags.Cue_Hit_Impact, HitData, IntensityScale * 0.5f);
		break;

	case ESFHitOutcome::Hit:
	default:
		if (ResolvedHit.bIsFinisher)
		{
			ExecuteHitCue(HitData.TargetActor, Tags.Cue_Hit_Finisher, HitData, IntensityScale);
		}
		else if (ResolvedHit.bCauseGuardBreak)
		{
			ExecuteHitCue(HitData.TargetActor, Tags.Cue_Hit_GuardBreak, HitData, IntensityScale);
		}
		else if (HitData.bIsWeakpointHit)
		{
			ExecuteHitCue(HitData.TargetActor, Tags.Cue_Hit_Weakpoint, HitData, IntensityScale);
		}
		else if (bIsCrit)
		{
			ExecuteHitCue(HitData.TargetActor, Tags.Cue_Hit_Critical, HitData, IntensityScale);
		}
		else
		{
			ExecuteHitCue(HitData.TargetActor, Tags.Cue_Hit_Impact, HitData, IntensityScale);
		}
		break;
	}

	// Hit reaction tags as transient cue payload: anim graphs can also read the
	// applied DynamicGrantedTags on their ASC for direction/severity.
	if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitData.TargetActor))
	{
		FGameplayTagContainer ReactionTags;
		if (DirectionTag.IsValid())
		{
			ReactionTags.AddTag(DirectionTag);
		}
		if (SeverityTag.IsValid())
		{
			ReactionTags.AddTag(SeverityTag);
		}
		if (ReactionTags.Num() > 0)
		{
			// Use a 0-duration loose tag burst so anim BPs can poll for one frame.
			TargetASC->AddLooseGameplayTags(ReactionTags, 1);
			// Schedule removal next tick so the tag acts as a one-shot trigger.
			if (UWorld* World = GetWorld())
			{
				TWeakObjectPtr<UAbilitySystemComponent> WeakASC = TargetASC;
				World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([WeakASC, ReactionTags]()
				{
					if (UAbilitySystemComponent* ASC = WeakASC.Get())
					{
						ASC->RemoveLooseGameplayTags(ReactionTags, 1);
					}
				}));
			}
		}
	}

	// -------- Hit stop on melee impact (never on graze/block — those have their own feel). --------
	if (ResolvedHit.Outcome == ESFHitOutcome::Hit ||
		ResolvedHit.Outcome == ESFHitOutcome::PerfectParry ||
		ResolvedHit.bCauseGuardBreak)
	{
		ApplyHitStop(HitData.TargetActor, Feedback, IntensityScale);
	}

	// -------- Slow-mo: parries, finishers, or anything the resolver flagged. --------
	if (ResolvedHit.bTriggerSlowMo || ResolvedHit.bIsFinisher || ResolvedHit.Outcome == ESFHitOutcome::PerfectParry)
	{
		ApplySlowMo(Feedback, IntensityScale);
	}

	// -------- Camera shake. --------
	TSubclassOf<UCameraShakeBase> ShakeClass = NormalHitCameraShake;
	if (ResolvedHit.bIsFinisher || ResolvedHit.Outcome == ESFHitOutcome::PerfectParry)
	{
		ShakeClass = CinematicCameraShake ? CinematicCameraShake : HeavyHitCameraShake;
	}
	else if (bIsCrit || ResolvedHit.bCauseGuardBreak || HitData.AttackType == ESFAttackType::Heavy)
	{
		ShakeClass = HeavyHitCameraShake ? HeavyHitCameraShake : NormalHitCameraShake;
	}
	PlayCameraShakeForHit(HitLocation, Feedback, IntensityScale, ShakeClass);
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

const FSFCinematicFeedback& USFCombatComponent::GetFeedbackForAttackType(ESFAttackType AttackType) const
{
	switch (AttackType)
	{
	case ESFAttackType::Light:
		return LightFeedback;
	case ESFAttackType::Heavy:
	default:
		return HeavyFeedback;
	}
}

FGameplayTag USFCombatComponent::ComputeDirectionalHitTag(const FSFHitData& HitData) const
{
	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
	if (!HitData.TargetActor || !HitData.SourceActor)
	{
		return Tags.HitReact_Direction_Front;
	}

	// Vector from target *to* attacker, projected on the horizontal plane.
	FVector ToAttacker = HitData.SourceActor->GetActorLocation() - HitData.TargetActor->GetActorLocation();
	ToAttacker.Z = 0.f;
	ToAttacker.Normalize();

	const FVector TargetForward = HitData.TargetActor->GetActorForwardVector().GetSafeNormal2D();
	const FVector TargetRight = HitData.TargetActor->GetActorRightVector().GetSafeNormal2D();

	const float Forward = FVector::DotProduct(ToAttacker, TargetForward);
	const float Right = FVector::DotProduct(ToAttacker, TargetRight);

	// Forward / Back take priority when the angle is close to ±forward axis.
	if (FMath::Abs(Forward) >= FMath::Abs(Right))
	{
		return Forward >= 0.f ? Tags.HitReact_Direction_Front : Tags.HitReact_Direction_Back;
	}
	return Right >= 0.f ? Tags.HitReact_Direction_Right : Tags.HitReact_Direction_Left;
}

FGameplayTag USFCombatComponent::ComputeSeverityHitTag(const FSFHitData& HitData, const FSFResolvedHit& ResolvedHit) const
{
	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
	if (ResolvedHit.bIsFinisher)
	{
		return Tags.HitReact_Severity_Launch;
	}
	if (ResolvedHit.bCauseGuardBreak || ResolvedHit.bCauseStagger)
	{
		return Tags.HitReact_Severity_Stagger;
	}
	if (HitData.AttackType == ESFAttackType::Heavy || HitData.bIsWeakpointHit)
	{
		return Tags.HitReact_Severity_Heavy;
	}
	return Tags.HitReact_Severity_Light;
}

bool USFCombatComponent::IsWeakpointBone(FName BoneName) const
{
	if (BoneName.IsNone())
	{
		return false;
	}
	const FString BoneStr = BoneName.ToString();
	for (const FName& Candidate : WeakpointBoneNames)
	{
		if (Candidate.IsNone())
		{
			continue;
		}
		if (BoneStr.Equals(Candidate.ToString(), ESearchCase::IgnoreCase))
		{
			return true;
		}
	}
	return false;
}

void USFCombatComponent::ApplyHitStop(AActor* Target, const FSFCinematicFeedback& Feedback, float IntensityScale)
{
	if (!Target || Feedback.HitStopDuration <= 0.f)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Prefer dilating just the target's mesh, so the source character keeps
	// momentum (this is what makes a hit "punchy" without freezing the player).
	if (ACharacter* Char = Cast<ACharacter>(Target))
	{
		if (USkeletalMeshComponent* Mesh = Char->GetMesh())
		{
			PreHitStopMeshTimeDilation = 1.f; // mesh component dilation isn't easily readable; assume 1
			Mesh->GlobalAnimRateScale = FMath::Clamp(Feedback.HitStopTimeDilation, 0.01f, 1.f);
			PendingHitStopTarget = Target;
		}
	}

	// Also briefly dilate the actor itself for VFX timing.
	Target->CustomTimeDilation = FMath::Clamp(Feedback.HitStopTimeDilation, 0.01f, 1.f);

	const float Duration = FMath::Clamp(Feedback.HitStopDuration, 0.f, 0.5f) / FMath::Max(IntensityScale, 0.1f);
	World->GetTimerManager().SetTimer(HitStopTimerHandle, this, &USFCombatComponent::ClearHitStop, Duration, false);
}

void USFCombatComponent::ClearHitStop()
{
	if (AActor* Target = PendingHitStopTarget.Get())
	{
		Target->CustomTimeDilation = 1.f;
		if (ACharacter* Char = Cast<ACharacter>(Target))
		{
			if (USkeletalMeshComponent* Mesh = Char->GetMesh())
			{
				Mesh->GlobalAnimRateScale = 1.f;
			}
		}
	}
	PendingHitStopTarget.Reset();
}

void USFCombatComponent::ApplySlowMo(const FSFCinematicFeedback& Feedback, float /*IntensityScale*/)
{
	UWorld* World = GetWorld();
	if (!World || Feedback.SlowMoDuration <= 0.f)
	{
		return;
	}

	PreSlowMoGlobalTimeDilation = UGameplayStatics::GetGlobalTimeDilation(World);
	UGameplayStatics::SetGlobalTimeDilation(World, FMath::Clamp(Feedback.SlowMoTimeDilation, 0.05f, 1.f));

	// Slow-mo duration is in real seconds, so we run the timer ignoring time dilation.
	FTimerManager& TM = World->GetTimerManager();
	TM.SetTimer(SlowMoTimerHandle, this, &USFCombatComponent::ClearSlowMo, FMath::Max(0.01f, Feedback.SlowMoDuration), false);
}

void USFCombatComponent::ClearSlowMo()
{
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, FMath::IsNearlyZero(PreSlowMoGlobalTimeDilation) ? 1.f : PreSlowMoGlobalTimeDilation);
	}
}

void USFCombatComponent::ExecuteHitCue(AActor* Target, FGameplayTag CueTag, const FSFHitData& HitData, float Magnitude) const
{
	if (!CueTag.IsValid() || !Target)
	{
		return;
	}

	FGameplayCueParameters CueParams;
	CueParams.Instigator = HitData.SourceActor;
	CueParams.EffectCauser = HitData.SourceActor;
	CueParams.Location = HitData.HitResult.ImpactPoint;
	CueParams.Normal = HitData.HitResult.ImpactNormal;
	CueParams.PhysicalMaterial = HitData.HitResult.PhysMaterial;
	CueParams.RawMagnitude = Magnitude;
	CueParams.AggregatedSourceTags = HitData.AttackTags;

	if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target))
	{
		TargetASC->ExecuteGameplayCue(CueTag, CueParams);
		return;
	}

	// Fallback: ask the cue manager to fire a one-shot burst at the impact point.
	if (UWorld* World = GetWorld())
	{
		if (UGameplayCueManager* CueMgr = UAbilitySystemGlobals::Get().GetGameplayCueManager())
		{
			CueMgr->HandleGameplayCue(Target, CueTag, EGameplayCueEvent::Executed, CueParams);
		}
	}
}

void USFCombatComponent::PlayCameraShakeForHit(const FVector& WorldLocation, const FSFCinematicFeedback& Feedback, float IntensityScale, TSubclassOf<UCameraShakeBase> ShakeOverride) const
{
	if (!ShakeOverride)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float Scale = Feedback.CameraShakeScale * FMath::Max(IntensityScale, 0.1f);

	// Radial shake reaches every local player camera in range — handles split-screen
	// and lets bystander cameras feel the hit too.
	UGameplayStatics::PlayWorldCameraShake(
		World,
		ShakeOverride,
		WorldLocation,
		CameraShakeInnerRadius,
		CameraShakeOuterRadius,
		Scale,
		false
	);
}

void USFCombatComponent::PlayCinematicFeedback(FGameplayTag FeedbackTag, AActor* AtActor, FVector AtLocation)
{
	if (!FeedbackTag.IsValid())
	{
		return;
	}

	FSFHitData StubData;
	StubData.SourceActor = OwnerCharacter;
	StubData.TargetActor = AtActor ? AtActor : OwnerCharacter.Get();
	StubData.HitResult.ImpactPoint = AtLocation;

	ExecuteHitCue(StubData.TargetActor, FeedbackTag, StubData, 1.f);
}
