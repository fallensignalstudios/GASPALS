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

	UPROPERTY(BlueprintReadWrite)
	FGameplayTagContainer ResultTagsOnTarget;

	UPROPERTY(BlueprintReadWrite)
	FGameplayTagContainer ResultTagsOnSource;
};