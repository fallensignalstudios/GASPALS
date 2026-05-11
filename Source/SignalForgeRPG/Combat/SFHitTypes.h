#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SFHitTypes.generated.h"

UENUM(BlueprintType)
enum class ESFAttackType : uint8
{
	Light	UMETA(DisplayName = "Light"),
	Heavy	UMETA(DisplayName = "Heavy"),
	Special UMETA(DisplayName = "Special"),
	Force	UMETA(DisplayName = "Force")
};

UENUM(BlueprintType)
enum class ESFHitOutcome : uint8
{
	Invalid		 UMETA(DisplayName = "Invalid"),
	Hit			 UMETA(DisplayName = "Hit"),
	Blocked		 UMETA(DisplayName = "Blocked"),
	PerfectParry UMETA(DisplayName = "Perfect Parry"),
	Graze		 UMETA(DisplayName = "Graze"),
	Immune		 UMETA(DisplayName = "Immune")
};

USTRUCT(BlueprintType)
struct FSFHitData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(BlueprintReadWrite)
	ESFAttackType AttackType = ESFAttackType::Light;

	UPROPERTY(BlueprintReadWrite)
	FHitResult HitResult;

	UPROPERTY(BlueprintReadWrite)
	FVector HitDirection = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite)
	float AttackTime = 0.0f;

	UPROPERTY(BlueprintReadWrite)
	FGameplayTagContainer AttackTags;

	UPROPERTY(BlueprintReadWrite)
	bool bIsBlockable = true;

	UPROPERTY(BlueprintReadWrite)
	bool bIgnoreGuard = false;

	UPROPERTY(BlueprintReadWrite)
	float PoiseDamageScale = 1.0f;

	UPROPERTY(BlueprintReadWrite)
	float BreakGuardBonus = 0.0f;

	/** Current combo chain index when this hit was thrown (0 = first attack in a chain). */
	UPROPERTY(BlueprintReadWrite)
	int32 ComboIndex = 0;

	/** True if the hit landed on a registered weakpoint bone. */
	UPROPERTY(BlueprintReadWrite)
	bool bIsWeakpointHit = false;
};

USTRUCT(BlueprintType)
struct FSFResolvedHit
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	ESFHitOutcome Outcome = ESFHitOutcome::Invalid;

	UPROPERTY(BlueprintReadWrite)
	float HealthDamage = 0.0f;

	UPROPERTY(BlueprintReadWrite)
	float PoiseDamageToTarget = 0.0f;

	UPROPERTY(BlueprintReadWrite)
	float PoiseDamageToSource = 0.0f;

	UPROPERTY(BlueprintReadWrite)
	bool bCauseStagger = false;

	UPROPERTY(BlueprintReadWrite)
	bool bCauseGuardBreak = false;

	UPROPERTY(BlueprintReadWrite)
	bool bTriggerSlowMo = false;

	/** Damage multiplier applied by the resolver (crit/weakpoint/finisher). 1.0 = no scaling. */
	UPROPERTY(BlueprintReadWrite)
	float DamageMultiplier = 1.0f;

	/** If true, treat this as a finisher / cinematic execution hit. */
	UPROPERTY(BlueprintReadWrite)
	bool bIsFinisher = false;

	UPROPERTY(BlueprintReadWrite)
	FGameplayTagContainer ResultTagsOnTarget;

	UPROPERTY(BlueprintReadWrite)
	FGameplayTagContainer ResultTagsOnSource;
};

/**
 * Designer-tunable cinematic feedback profile.
 * Drives hit stop, slow motion, camera shake and base GameplayCue intensity
 * per attack type. Light attacks should feel snappy; heavy attacks should
 * land like a freight train.
 */
USTRUCT(BlueprintType)
struct FSFCinematicFeedback
{
	GENERATED_BODY()

	/** Time dilation applied locally during hit stop (0.0 = full freeze, 1.0 = none). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float HitStopTimeDilation = 0.05f;

	/** Duration of the hit stop, in real seconds (not dilated). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float HitStopDuration = 0.07f;

	/** Global time dilation used by slow-mo (parry / finisher). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float SlowMoTimeDilation = 0.30f;

	/** Real-time duration of slow-mo windows. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float SlowMoDuration = 0.25f;

	/** Base camera-shake scale for an ordinary hit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float CameraShakeScale = 1.0f;
};