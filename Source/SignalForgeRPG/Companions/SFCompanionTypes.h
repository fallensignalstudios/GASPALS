#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SFCompanionTypes.generated.h"

/**
 * Combat stance preset. Drives a tag bundle the BT/tactics layer reads.
 * Tank: stay close to threats, taunt, prioritize survivability.
 * DPS:  flank, focus highest-threat target, cycle damage abilities.
 * Healer: stay near player, prioritize support/heal abilities.
 * Custom: stance is overridden by free-form tag rules — designer use only.
 */
UENUM(BlueprintType)
enum class ESFCompanionStance : uint8
{
	Tank   UMETA(DisplayName = "Tank"),
	DPS    UMETA(DisplayName = "DPS"),
	Healer UMETA(DisplayName = "Healer"),
	Custom UMETA(DisplayName = "Custom"),
};

/** Orthogonal aggression dial — independent of stance. */
UENUM(BlueprintType)
enum class ESFCompanionAggression : uint8
{
	Passive    UMETA(DisplayName = "Passive"),    // Don't engage unless attacked
	Defensive  UMETA(DisplayName = "Defensive"),  // Engage threats to player or self
	Aggressive UMETA(DisplayName = "Aggressive"), // Engage anything hostile in range
};

/** Engagement-distance preference. */
UENUM(BlueprintType)
enum class ESFCompanionEngagementRange : uint8
{
	StayClose  UMETA(DisplayName = "Stay Close"),
	Skirmish   UMETA(DisplayName = "Skirmish"),
	LongRange  UMETA(DisplayName = "Long Range"),
};

/**
 * Discrete order types issued via radial wheel or hotkey.
 * Companion BT consumes the active order until it's completed/cancelled.
 */
UENUM(BlueprintType)
enum class ESFCompanionOrderType : uint8
{
	None             UMETA(DisplayName = "None"),
	Follow           UMETA(DisplayName = "Follow"),         // Default — follow player
	HoldPosition     UMETA(DisplayName = "Hold Position"),
	MoveToLocation   UMETA(DisplayName = "Move To Location"),
	AttackTarget     UMETA(DisplayName = "Attack Target"),
	FocusFire        UMETA(DisplayName = "Focus Fire"),     // Stay on target until dead/cancelled
	UseAbility       UMETA(DisplayName = "Use Ability"),
	RevivePlayer     UMETA(DisplayName = "Revive Player"),
	RetreatToPlayer  UMETA(DisplayName = "Retreat To Player"),
};

/**
 * Single replicated order. Companions hold one active order at a time.
 * Sequence number is bumped on each issue so the BT can detect
 * "new order arrived even if same type" without a separate flag.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFCompanionOrder
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Companion|Order")
	ESFCompanionOrderType Type = ESFCompanionOrderType::Follow;

	/** Monotonic, server-assigned. BT compares against last-handled to detect re-issued orders. */
	UPROPERTY(BlueprintReadOnly, Category = "Companion|Order")
	int32 Sequence = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Companion|Order")
	TWeakObjectPtr<AActor> TargetActor;

	UPROPERTY(BlueprintReadOnly, Category = "Companion|Order")
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Companion|Order")
	FGameplayTag AbilityTag;

	bool IsValidOrder() const { return Type != ESFCompanionOrderType::None; }
};

/**
 * Designer-tunable thresholds the tactics layer evaluates each tick / on
 * health change and exposes as blackboard booleans. Lets BT subtrees
 * (heal, taunt, retreat) read "is this true right now" instead of doing
 * their own math, and lets per-companion archetypes differ without
 * editing trees.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFCompanionTacticsThresholds
{
	GENERATED_BODY()

	/** 0..1 fraction of MaxHealth at which the player counts as "low" — fires heal/peel barks and BT branches. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactics|Thresholds", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PlayerLowHealthPct = 0.35f;

	/** 0..1 fraction of MaxHealth at which the companion itself should retreat. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactics|Thresholds", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SelfLowHealthPct = 0.25f;

	/** 0..1 fraction of MaxHealth at which an allied target counts as "low" — Healer-stance support trigger. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactics|Thresholds", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AllyLowHealthPct = 0.40f;

	/** Distance at which Tank stance should taunt / interpose between player and threat. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactics|Thresholds", meta = (ClampMin = "50.0"))
	float TauntInterposeRange = 800.f;

	/** Min seconds between commanded ability uses (per companion) — softens spam. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactics|Thresholds", meta = (ClampMin = "0.0"))
	float AbilityCommandCooldown = 1.5f;
};
