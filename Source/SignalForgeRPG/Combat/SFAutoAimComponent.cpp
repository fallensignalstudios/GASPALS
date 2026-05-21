#include "Combat/SFAutoAimComponent.h"

#include "Characters/SFCharacterBase.h"
#include "Combat/SFWeaponData.h"
#include "Components/SFEquipmentComponent.h"
#include "Components/CapsuleComponent.h"
#include "Core/SFAttributeSetBase.h"
#include "Engine/World.h"
#include "Engine/HitResult.h"
#include "EngineUtils.h"
#include "Faction/SFFactionStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

USFAutoAimComponent::USFAutoAimComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Tick during gameplay; safe to disable in cinematics by toggling bEnabled.
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void USFAutoAimComponent::BeginPlay()
{
	Super::BeginPlay();

	CachedSelf = Cast<ASFCharacterBase>(GetOwner());
	if (ASFCharacterBase* Self = CachedSelf.Get())
	{
		CachedPC = Cast<APlayerController>(Self->GetController());
	}
}

void USFAutoAimComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bEnabled)
	{
		CachedTarget.Reset();
		return;
	}

	// Refresh the PC pointer in case the player gets re-possessed mid-game.
	if (!CachedPC.IsValid())
	{
		if (ASFCharacterBase* Self = CachedSelf.Get())
		{
			CachedPC = Cast<APlayerController>(Self->GetController());
		}
	}

	ResolveWeaponOverrides();
	RefreshCachedTarget();
}

void USFAutoAimComponent::ResolveWeaponOverrides()
{
	ASFCharacterBase* Self = CachedSelf.Get();
	if (!Self)
	{
		return;
	}

	// Best-effort pull from the equipped weapon's ranged config. If the equipment
	// component is missing or the weapon has no ranged config, we just keep the
	// component defaults \u2014 better than crashing on a weaponless pawn.
	USFEquipmentComponent* Equipment = Self->GetEquipmentComponent();
	if (!Equipment)
	{
		return;
	}

	USFWeaponData* WeaponData = Equipment->GetCurrentWeaponData();
	if (!WeaponData)
	{
		return;
	}

	const FSFAutoAimConfig& Cfg = WeaponData->RangedConfig.AutoAim;
	if (!Cfg.bOverrideComponentDefaults)
	{
		return;
	}

	MaxTargetRange = Cfg.MaxTargetRange;
	MagnetismAngleDeg = Cfg.MagnetismAngleDeg;
	HipfireMagnetismFraction = Cfg.HipfireMagnetismFraction;
	StickyAngleDeg = Cfg.StickyAngleDeg;
	MinStickyMultiplier = Cfg.MinStickyMultiplier;
	NudgeAngleDeg = Cfg.NudgeAngleDeg;
	NudgeStrength = Cfg.NudgeStrength;
}

void USFAutoAimComponent::RefreshCachedTarget()
{
	CachedTarget.Reset();
	CachedTargetAngleRad = PI; // sentinel = "no target"

	ASFCharacterBase* Self = CachedSelf.Get();
	UWorld* World = GetWorld();
	if (!Self || !World)
	{
		return;
	}

	// Use eye view for cone apex \u2014 matches how WeaponFire / WeaponBeam aim.
	FVector EyeLoc;
	FRotator EyeRot;
	Self->GetActorEyesViewPoint(EyeLoc, EyeRot);
	const FVector EyeFwd = EyeRot.Vector();

	// Use the widest of the three half-angles as the candidate cone so we only
	// iterate the world once per tick. Individual queries (magnetism / sticky /
	// nudge) re-test against their own narrower cones later.
	const float WidestDeg = FMath::Max3(MagnetismAngleDeg, StickyAngleDeg, NudgeAngleDeg);
	const float ConeRad = FMath::DegreesToRadians(WidestDeg);

	// Iterate all ASFCharacterBase actors. The cast-iterator skips static geometry,
	// so the cost is "number of NPCs + player" which is bounded.
	float BestScore = TNumericLimits<float>::Max();
	AActor* BestTarget = nullptr;
	float BestAngleRad = PI;

	for (TActorIterator<ASFCharacterBase> It(World); It; ++It)
	{
		ASFCharacterBase* Candidate = *It;
		if (!Candidate || Candidate == Self)
		{
			continue;
		}

		const float Score = ScoreCandidate(Candidate, EyeLoc, EyeFwd, ConeRad);
		if (Score < 0.0f)
		{
			continue; // disqualified
		}

		if (Score < BestScore)
		{
			BestScore = Score;
			BestTarget = Candidate;

			// Recompute angle for the picked target so we can hand it to the sticky/nudge code without re-scoring.
			const FVector ToTarget = (GetTargetAimPoint(Candidate) - EyeLoc).GetSafeNormal();
			BestAngleRad = FMath::Acos(FMath::Clamp(FVector::DotProduct(EyeFwd, ToTarget), -1.0f, 1.0f));
		}
	}

	if (BestTarget)
	{
		CachedTarget = BestTarget;
		CachedTargetAngleRad = BestAngleRad;

		if (bVerboseLogging)
		{
			UE_LOG(LogTemp, Verbose, TEXT("[AutoAim] target=%s angle=%.2f\u00b0 score=%.3f"),
				*BestTarget->GetName(),
				FMath::RadiansToDegrees(BestAngleRad),
				BestScore);
		}
	}
}

float USFAutoAimComponent::ScoreCandidate(const AActor* Candidate, const FVector& EyeLoc, const FVector& EyeFwd, float AcceptConeRadians) const
{
	const ASFCharacterBase* CandChar = Cast<ASFCharacterBase>(Candidate);
	if (!CandChar)
	{
		return -1.0f;
	}

	// Dead pawns don't auto-aim. ASFCharacterBase exposes bIsDead via... actually we read it via
	// the health attribute: a pawn at Health == 0 should be ignored. Defensive: GetAttributeSet may be null.
	if (const USFAttributeSetBase* Attrs = CandChar->GetAttributeSet())
	{
		if (Attrs->GetHealth() <= 0.0f)
		{
			return -1.0f;
		}
	}

	// Faction gate: must be hostile to self. If the faction system isn't wired up on either side,
	// AreHostile returns false and we silently skip \u2014 which is the safer default than aim-assisting onto allies.
	if (!USFFactionStatics::AreHostile(GetOwner(), Candidate))
	{
		return -1.0f;
	}

	const FVector AimPoint = GetTargetAimPoint(Candidate);
	const FVector ToTarget = AimPoint - EyeLoc;
	const float Distance = ToTarget.Size();
	if (Distance > MaxTargetRange || Distance <= KINDA_SMALL_NUMBER)
	{
		return -1.0f;
	}

	const FVector ToTargetN = ToTarget / Distance;
	const float CosAngle = FVector::DotProduct(EyeFwd, ToTargetN);
	const float AngleRad = FMath::Acos(FMath::Clamp(CosAngle, -1.0f, 1.0f));

	if (AngleRad > AcceptConeRadians)
	{
		return -1.0f; // outside the widest cone we care about
	}

	// Line of sight (optional but on by default). Trace from eye to aim point, ignore self + candidate.
	if (bRequireLineOfSight)
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(SFAutoAimLOS), false);
		Params.AddIgnoredActor(GetOwner());
		Params.AddIgnoredActor(Candidate);

		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, EyeLoc, AimPoint, LineOfSightChannel, Params))
		{
			// Something solid is between us \u2014 disqualify so we don't assist onto enemies behind walls.
			return -1.0f;
		}
	}

	// Scoring: angle weighted heavier than distance \u2014 Destiny prioritizes "closest to reticle" over
	// "closest to me" because that's what actually feels like the player picked the target.
	const float AngleWeight = 5.0f;
	const float DistanceWeight = 1.0f / FMath::Max(100.0f, MaxTargetRange);
	return AngleRad * AngleWeight + Distance * DistanceWeight;
}

FVector USFAutoAimComponent::GetTargetAimPoint(const AActor* Target) const
{
	if (!Target)
	{
		return FVector::ZeroVector;
	}

	// Prefer the chest \u2014 use the capsule top minus a half-half height so we aim at center mass,
	// not the feet (where ActorLocation typically sits for ACharacter pawns).
	if (const ACharacter* Char = Cast<ACharacter>(Target))
	{
		if (const UCapsuleComponent* Capsule = Char->GetCapsuleComponent())
		{
			const float Half = Capsule->GetScaledCapsuleHalfHeight();
			return Char->GetActorLocation() + FVector(0.0f, 0.0f, Half * 0.35f);
		}
	}

	return Target->GetActorLocation();
}

FRotator USFAutoAimComponent::GetMagnetizedAimDirectionForFireMode(const FVector& BaseEyeLocation, const FRotator& BaseEyeRotation, bool bIsAimingDownSights) const
{
	const float Strength = bIsAimingDownSights ? 1.0f : FMath::Clamp(HipfireMagnetismFraction, 0.0f, 1.0f);
	return GetMagnetizedAimDirection(BaseEyeLocation, BaseEyeRotation, Strength);
}

FRotator USFAutoAimComponent::GetMagnetizedAimDirection(const FVector& BaseEyeLocation, const FRotator& BaseEyeRotation, float MagnetismStrength) const
{
	if (!bEnabled || MagnetismStrength <= 0.0f)
	{
		return BaseEyeRotation;
	}

	const AActor* Target = CachedTarget.Get();
	if (!Target)
	{
		return BaseEyeRotation;
	}

	// Only apply magnetism when the target is inside the (narrower) magnetism cone.
	// Sticky candidates are caught by RefreshCachedTarget but might still be outside the magnetism cone.
	const float MagnetismRad = FMath::DegreesToRadians(MagnetismAngleDeg);
	if (CachedTargetAngleRad > MagnetismRad)
	{
		return BaseEyeRotation;
	}

	// Strength ramp: targets dead-on get 100% magnetism, targets on the cone edge get 0.
	// This avoids a "pop" at the edge of the cone. Quadratic falloff feels best.
	const float EdgeRatio = FMath::Clamp(CachedTargetAngleRad / FMath::Max(KINDA_SMALL_NUMBER, MagnetismRad), 0.0f, 1.0f);
	const float CenterCloseness = 1.0f - EdgeRatio;
	const float Strength = FMath::Clamp(MagnetismStrength * CenterCloseness * CenterCloseness, 0.0f, 1.0f);

	const FVector AimPoint = GetTargetAimPoint(Target);
	const FRotator TargetRot = (AimPoint - BaseEyeLocation).Rotation();

	// Lerp between base and target rotation. Shortest-path interp is what we want here.
	return FMath::Lerp(BaseEyeRotation, TargetRot, Strength);
}

float USFAutoAimComponent::GetLookInputMultiplier() const
{
	if (!bEnabled)
	{
		return 1.0f;
	}

	const AActor* Target = CachedTarget.Get();
	if (!Target)
	{
		return 1.0f;
	}

	const float StickyRad = FMath::DegreesToRadians(StickyAngleDeg);
	if (CachedTargetAngleRad > StickyRad)
	{
		return 1.0f;
	}

	// Inside the sticky cone: ramp the multiplier from 1.0 at the edge to MinStickyMultiplier at center.
	const float EdgeRatio = FMath::Clamp(CachedTargetAngleRad / FMath::Max(KINDA_SMALL_NUMBER, StickyRad), 0.0f, 1.0f);
	const float CenterCloseness = 1.0f - EdgeRatio;
	return FMath::Lerp(1.0f, MinStickyMultiplier, CenterCloseness);
}

void USFAutoAimComponent::RequestReticleNudge()
{
	if (!bEnabled)
	{
		return;
	}

	APlayerController* PC = CachedPC.Get();
	ASFCharacterBase* Self = CachedSelf.Get();
	const AActor* Target = CachedTarget.Get();
	if (!PC || !Self || !Target)
	{
		return;
	}

	const float NudgeRad = FMath::DegreesToRadians(NudgeAngleDeg);
	if (CachedTargetAngleRad > NudgeRad)
	{
		return;
	}

	FVector EyeLoc;
	FRotator EyeRot;
	Self->GetActorEyesViewPoint(EyeLoc, EyeRot);

	const FRotator TargetRot = (GetTargetAimPoint(Target) - EyeLoc).Rotation();
	const FRotator CurrentControlRot = PC->GetControlRotation();

	// Lerp by NudgeStrength so the snap is satisfying but not jarring.
	const FRotator NudgedRot = FMath::Lerp(CurrentControlRot, TargetRot, FMath::Clamp(NudgeStrength, 0.0f, 1.0f));

	PC->SetControlRotation(NudgedRot);

	if (bVerboseLogging)
	{
		UE_LOG(LogTemp, Log, TEXT("[AutoAim] reticle nudge \u2192 %s (angle %.2f\u00b0)"),
			*Target->GetName(),
			FMath::RadiansToDegrees(CachedTargetAngleRad));
	}
}
