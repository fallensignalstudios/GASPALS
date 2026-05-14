#include "Combat/SFHitReactionComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Characters/SFCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/SignalForgeGameplayTags.h"
#include "Engine/World.h"
#include "TimerManager.h"

USFHitReactionComponent::USFHitReactionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);

	// Sensible humanoid defaults so the component does something useful even with no setup.
	BonesToNotSimulate = { TEXT("root"), TEXT("pelvis") };
}

void USFHitReactionComponent::BeginPlay()
{
	Super::BeginPlay();

	// Pre-populate severity multiplier maps with native tags so designers see
	// them as defaults in the details panel without authoring every key.
	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();

	if (SeverityImpulseMultipliers.Num() == 0)
	{
		SeverityImpulseMultipliers.Add(Tags.HitReact_Severity_Light,   1.0f);
		SeverityImpulseMultipliers.Add(Tags.HitReact_Severity_Heavy,   1.6f);
		SeverityImpulseMultipliers.Add(Tags.HitReact_Severity_Stagger, 2.4f);
		SeverityImpulseMultipliers.Add(Tags.HitReact_Severity_Launch,  4.0f);
	}

	if (SeverityPlayRateMultipliers.Num() == 0)
	{
		SeverityPlayRateMultipliers.Add(Tags.HitReact_Severity_Light,   1.10f);
		SeverityPlayRateMultipliers.Add(Tags.HitReact_Severity_Heavy,   0.95f);
		SeverityPlayRateMultipliers.Add(Tags.HitReact_Severity_Stagger, 0.85f);
		SeverityPlayRateMultipliers.Add(Tags.HitReact_Severity_Launch,  0.75f);
	}
}

void USFHitReactionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Make sure we don't leave a half-simulated mesh behind on map transition.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BlendOutStartTimer);
		World->GetTimerManager().ClearTimer(BlendOutFinalizeTimer);
	}

	if (!ActiveSimulationBone.IsNone())
	{
		if (USkeletalMeshComponent* Mesh = GetTargetMesh())
		{
			Mesh->SetAllBodiesBelowSimulatePhysics(ActiveSimulationBone, false);
			Mesh->SetAllBodiesBelowPhysicsBlendWeight(ActiveSimulationBone, 0.f);
		}
		ActiveSimulationBone = NAME_None;
	}

	Super::EndPlay(EndPlayReason);
}

void USFHitReactionComponent::HandleIncomingHit(const FSFHitData& HitData, const FSFResolvedHit& ResolvedHit)
{
	if (IsOwnerDead())
	{
		// Skip live reactions on a corpse -- death montage / ragdoll should win.
		return;
	}

	if (ResolvedHit.Outcome == ESFHitOutcome::Invalid ||
		ResolvedHit.Outcome == ESFHitOutcome::Immune)
	{
		// Hit didn't actually land on us in a way that should produce a reaction.
		return;
	}

	// ---- Direction + severity classification ----------------------------------
	// Mirror the math in USFCombatComponent::ComputeDirectionalHitTag so a target
	// can react identically regardless of whether the hit came from a melee
	// trace, a projectile, or a designer-driven TriggerReaction call. We can't
	// just call the attacker's combat component because some hits (projectiles)
	// resolve through different paths and may not have populated its state.
	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
	FGameplayTag DirectionTag = Tags.HitReact_Direction_Front;
	FGameplayTag SeverityTag  = Tags.HitReact_Severity_Light;

	const AActor* Owner = GetOwner();
	if (Owner && HitData.SourceActor)
	{
		FVector ToAttacker = HitData.SourceActor->GetActorLocation() - Owner->GetActorLocation();
		ToAttacker.Z = 0.f;
		ToAttacker.Normalize();

		const FVector Forward = Owner->GetActorForwardVector().GetSafeNormal2D();
		const FVector Right   = Owner->GetActorRightVector().GetSafeNormal2D();

		const float FwdDot   = FVector::DotProduct(ToAttacker, Forward);
		const float RightDot = FVector::DotProduct(ToAttacker, Right);

		if (FMath::Abs(FwdDot) >= FMath::Abs(RightDot))
		{
			DirectionTag = FwdDot >= 0.f ? Tags.HitReact_Direction_Front : Tags.HitReact_Direction_Back;
		}
		else
		{
			DirectionTag = RightDot >= 0.f ? Tags.HitReact_Direction_Right : Tags.HitReact_Direction_Left;
		}
	}

	// Severity: prefer ResolvedHit signals, fall back to AttackType.
	if (ResolvedHit.bIsFinisher)
	{
		SeverityTag = Tags.HitReact_Severity_Launch;
	}
	else if (ResolvedHit.bCauseGuardBreak || ResolvedHit.bCauseStagger)
	{
		SeverityTag = Tags.HitReact_Severity_Stagger;
	}
	else if (HitData.AttackType == ESFAttackType::Heavy || HitData.bIsWeakpointHit)
	{
		SeverityTag = Tags.HitReact_Severity_Heavy;
	}

	// ---- Designer/BP extension hook ------------------------------------------
	ReceiveHitReactionStarted(HitData, ResolvedHit, DirectionTag, SeverityTag);

	// ---- Layer 1: physics impulse on the impacted bone -----------------------
	if (bEnablePhysicsImpulse)
	{
		USkeletalMeshComponent* Mesh = GetTargetMesh();
		if (Mesh)
		{
			const FName ImpactBoneRaw = HitData.HitResult.BoneName.IsNone() ? FallbackImpactBone : HitData.HitResult.BoneName;
			const FName SimBone = ResolveSimulationBone(Mesh, ImpactBoneRaw);

			if (!SimBone.IsNone())
			{
				// Damage-scale the magnitude, clamped to avoid sending a light hit
				// flying or letting a finisher punch the body through the floor.
				const float DamageScale = FMath::Clamp(
					ResolvedHit.HealthDamage / FMath::Max(ReferenceDamage, 1.f),
					DamageImpulseScaleMin,
					DamageImpulseScaleMax);

				const float SeverityScale = GetSeverityImpulseMultiplier(SeverityTag);

				const FVector Direction = ComputeImpulseVector(HitData, Mesh, ImpactBoneRaw);
				const float Magnitude = BaseImpulseMagnitude * DamageScale * SeverityScale;

				// Turn on simulation from SimBone down, ramp blend weight to the
				// designer-configured level, and apply the impulse at the actual
				// impact point so the rotation arc looks right.
				Mesh->SetAllBodiesBelowSimulatePhysics(SimBone, true, /*bIncludeSelf=*/true);
				Mesh->SetAllBodiesBelowPhysicsBlendWeight(SimBone, PhysicsBlendWeight, /*bSkipCustomPhysicsType=*/false, /*bIncludeSelf=*/true);

				const FVector ImpactPoint = HitData.HitResult.ImpactPoint.IsNearlyZero()
					? Mesh->GetBoneLocation(ImpactBoneRaw)
					: HitData.HitResult.ImpactPoint;

				Mesh->AddImpulseAtLocation(Direction * Magnitude, ImpactPoint, ImpactBoneRaw);

				ActiveSimulationBone = SimBone;

				// Schedule the staircase: hold -> ramp out -> finalize.
				if (UWorld* World = GetWorld())
				{
					FTimerManager& Timers = World->GetTimerManager();
					Timers.ClearTimer(BlendOutStartTimer);
					Timers.ClearTimer(BlendOutFinalizeTimer);

					Timers.SetTimer(BlendOutStartTimer, this, &USFHitReactionComponent::BeginPhysicsBlendOut, FMath::Max(PhysicsHoldTime, 0.001f), false);
					Timers.SetTimer(BlendOutFinalizeTimer, this, &USFHitReactionComponent::FinalizePhysicsBlend, FMath::Max(PhysicsHoldTime + BlendOutTime, 0.002f), false);
				}
			}
		}
	}

	// ---- Layer 2: directional anim montage -----------------------------------
	if (bEnableDirectionalMontage)
	{
		if (UAnimMontage* Montage = GetMontageFor(DirectionTag, SeverityTag))
		{
			if (USkeletalMeshComponent* Mesh = GetTargetMesh())
			{
				if (UAnimInstance* Anim = Mesh->GetAnimInstance())
				{
					if (bInterruptExistingMontage || !Anim->IsAnyMontagePlaying())
					{
						const float Rate = MontagePlayRate * GetSeverityPlayRateMultiplier(SeverityTag);
						Anim->Montage_Play(Montage, Rate);
					}
				}
			}
		}
	}

	OnHitReactionPlayed.Broadcast(HitData, ResolvedHit, DirectionTag, SeverityTag);
}

void USFHitReactionComponent::TriggerReaction(FGameplayTag DirectionTag, FGameplayTag SeverityTag, FName ImpactBone, FVector ImpulseWorldDirection, float ImpulseScale)
{
	// Designer entry point -- synthesize a minimal FSFHitData/FSFResolvedHit
	// and route through the main handler so the same layers fire. This keeps
	// the reaction path single-sourced even when BP wants to fake a hit
	// (e.g. for cinematic moments).
	FSFHitData Data;
	Data.TargetActor = GetOwner();
	Data.AttackType = (SeverityTag == FSignalForgeGameplayTags::Get().HitReact_Severity_Heavy)
		? ESFAttackType::Heavy : ESFAttackType::Light;
	Data.HitDirection = ImpulseWorldDirection.GetSafeNormal();
	Data.HitResult.BoneName = ImpactBone;
	if (USkeletalMeshComponent* Mesh = GetTargetMesh())
	{
		Data.HitResult.ImpactPoint = Mesh->GetBoneLocation(ImpactBone);
		Data.HitResult.ImpactNormal = -ImpulseWorldDirection.GetSafeNormal();
	}

	FSFResolvedHit Resolved;
	Resolved.Outcome = ESFHitOutcome::Hit;
	Resolved.HealthDamage = ReferenceDamage * FMath::Max(ImpulseScale, 0.f);
	Resolved.DamageMultiplier = 1.f;
	Resolved.bCauseStagger = (SeverityTag == FSignalForgeGameplayTags::Get().HitReact_Severity_Stagger);
	Resolved.bIsFinisher = (SeverityTag == FSignalForgeGameplayTags::Get().HitReact_Severity_Launch);

	HandleIncomingHit(Data, Resolved);
}

// ============================================================================
// Internals
// ============================================================================

USkeletalMeshComponent* USFHitReactionComponent::GetTargetMesh() const
{
	if (const ACharacter* Char = Cast<ACharacter>(GetOwner()))
	{
		return Char->GetMesh();
	}
	if (AActor* Owner = GetOwner())
	{
		return Owner->FindComponentByClass<USkeletalMeshComponent>();
	}
	return nullptr;
}

bool USFHitReactionComponent::IsOwnerDead() const
{
	if (const ASFCharacterBase* Char = Cast<ASFCharacterBase>(GetOwner()))
	{
		return Char->IsDead();
	}
	return false;
}

FName USFHitReactionComponent::ResolveSimulationBone(USkeletalMeshComponent* Mesh, FName ImpactBone) const
{
	if (!Mesh || ImpactBone.IsNone())
	{
		return NAME_None;
	}

	// Walk up the bone hierarchy until we find one NOT in BonesToNotSimulate.
	// This lets designers blacklist root/pelvis and still get a clean shoulder
	// hit to simulate from "spine_03" or wherever the hierarchy lands.
	FName Current = ImpactBone;
	int32 SafetyCounter = 0;
	while (!Current.IsNone() && SafetyCounter < 32)
	{
		bool bBlocked = false;
		for (const FName& Block : BonesToNotSimulate)
		{
			if (Current == Block)
			{
				bBlocked = true;
				break;
			}
		}
		if (!bBlocked)
		{
			return Current;
		}
		Current = Mesh->GetParentBone(Current);
		++SafetyCounter;
	}

	// Everything up the chain was blocked -- bail and simulate nothing.
	return NAME_None;
}

FVector USFHitReactionComponent::ComputeImpulseVector(const FSFHitData& HitData, USkeletalMeshComponent* Mesh, FName ImpactBone) const
{
	// Priority order:
	//   1. Explicit HitData.HitDirection if it's already set by the caller.
	//   2. Vector from source actor -> impact point (most physically correct).
	//   3. Vector from source actor -> target actor (fallback when impact
	//      point wasn't recorded, e.g. designer-faked hits).
	//   4. World forward (last-resort sentinel; should never hit in practice).
	if (!HitData.HitDirection.IsNearlyZero())
	{
		return HitData.HitDirection.GetSafeNormal();
	}

	if (HitData.SourceActor)
	{
		const FVector SourceLoc = HitData.SourceActor->GetActorLocation();
		FVector ImpactLoc = HitData.HitResult.ImpactPoint;
		if (ImpactLoc.IsNearlyZero() && Mesh)
		{
			ImpactLoc = Mesh->GetBoneLocation(ImpactBone);
		}
		if (!ImpactLoc.IsNearlyZero())
		{
			const FVector Dir = (ImpactLoc - SourceLoc).GetSafeNormal();
			if (!Dir.IsNearlyZero())
			{
				return Dir;
			}
		}

		if (const AActor* Owner = GetOwner())
		{
			const FVector Dir = (Owner->GetActorLocation() - SourceLoc).GetSafeNormal();
			if (!Dir.IsNearlyZero())
			{
				return Dir;
			}
		}
	}

	return FVector::ForwardVector;
}

float USFHitReactionComponent::GetSeverityImpulseMultiplier(FGameplayTag SeverityTag) const
{
	if (const float* Found = SeverityImpulseMultipliers.Find(SeverityTag))
	{
		return *Found;
	}
	return 1.f;
}

float USFHitReactionComponent::GetSeverityPlayRateMultiplier(FGameplayTag SeverityTag) const
{
	if (const float* Found = SeverityPlayRateMultipliers.Find(SeverityTag))
	{
		return *Found;
	}
	return 1.f;
}

UAnimMontage* USFHitReactionComponent::GetMontageFor(FGameplayTag DirectionTag, FGameplayTag SeverityTag) const
{
	const FSFHitReactionMontageRow* Row = MontagesByDirection.Find(DirectionTag);
	if (!Row)
	{
		return nullptr;
	}

	// Try the exact severity first, then walk down to lighter severities so a
	// designer who only authored "Heavy" still gets a montage on a "Stagger"
	// or "Launch" hit instead of falling silently back to physics only.
	if (const TObjectPtr<UAnimMontage>* Found = Row->BySeverityToMontage.Find(SeverityTag))
	{
		return Found->Get();
	}

	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
	const FGameplayTag FallbackOrder[] = {
		Tags.HitReact_Severity_Heavy,
		Tags.HitReact_Severity_Light
	};
	for (const FGameplayTag& Fallback : FallbackOrder)
	{
		if (Fallback == SeverityTag)
		{
			continue;
		}
		if (const TObjectPtr<UAnimMontage>* Found = Row->BySeverityToMontage.Find(Fallback))
		{
			return Found->Get();
		}
	}

	return nullptr;
}

void USFHitReactionComponent::BeginPhysicsBlendOut()
{
	if (ActiveSimulationBone.IsNone())
	{
		return;
	}
	if (USkeletalMeshComponent* Mesh = GetTargetMesh())
	{
		// Drop blend weight toward zero. The mesh keeps simulating until
		// FinalizePhysicsBlend turns simulation off, which lets the body
		// settle naturally rather than snapping back to pose.
		Mesh->SetAllBodiesBelowPhysicsBlendWeight(ActiveSimulationBone, 0.f, /*bSkipCustomPhysicsType=*/false, /*bIncludeSelf=*/true);
	}
}

void USFHitReactionComponent::FinalizePhysicsBlend()
{
	if (ActiveSimulationBone.IsNone())
	{
		return;
	}
	if (USkeletalMeshComponent* Mesh = GetTargetMesh())
	{
		Mesh->SetAllBodiesBelowSimulatePhysics(ActiveSimulationBone, false, /*bIncludeSelf=*/true);
		Mesh->SetAllBodiesBelowPhysicsBlendWeight(ActiveSimulationBone, 0.f, /*bSkipCustomPhysicsType=*/false, /*bIncludeSelf=*/true);
	}
	ActiveSimulationBone = NAME_None;
}
