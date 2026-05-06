#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Companions/SFCompanionTypes.h"
#include "GameplayTagContainer.h"
#include "SFCompanionTacticsComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FSFCompanionStanceChanged,
	ESFCompanionStance, OldStance,
	ESFCompanionStance, NewStance);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FSFCompanionOrderIssued,
	const FSFCompanionOrder&, Order);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FSFCompanionThresholdCrossed,
	FGameplayTag, ThresholdTag,   // Companion.Threat.PlayerLowHealth, .SelfLowHealth, .AllyLowHealth
	bool, bNowActive);

/**
 * USFCompanionTacticsComponent
 *
 * Lives on the companion pawn. Holds:
 *   - Stance (Tank / DPS / Healer / Custom)
 *   - Aggression (Passive / Defensive / Aggressive)
 *   - Engagement range (StayClose / Skirmish / LongRange)
 *   - Active order (one at a time)
 *   - StanceTags: gameplay tag container the AI controller / BT services
 *     and GAS abilities can match against to gate behavior.
 *
 * Server-authoritative. Replicated. Player input flows through the
 * USFCompanionComponent on the PlayerState, which calls server-only
 * setters here.
 */
UCLASS(ClassGroup = (Companion), meta = (BlueprintSpawnableComponent))
class SIGNALFORGERPG_API USFCompanionTacticsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFCompanionTacticsComponent();

	//~ Begin UActorComponent
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UActorComponent

	// -------------------------------------------------------------------------
	// Stance / aggression / engagement
	// -------------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Companion|Tactics")
	ESFCompanionStance GetStance() const { return Stance; }

	UFUNCTION(BlueprintPure, Category = "Companion|Tactics")
	ESFCompanionAggression GetAggression() const { return Aggression; }

	UFUNCTION(BlueprintPure, Category = "Companion|Tactics")
	ESFCompanionEngagementRange GetEngagementRange() const { return EngagementRange; }

	UFUNCTION(BlueprintPure, Category = "Companion|Tactics")
	const FGameplayTagContainer& GetStanceTags() const { return ActiveStanceTags; }

	/** Authority-only. Re-tags ASC if owner has one so abilities re-evaluate next frame. */
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Companion|Tactics")
	void SetStance(ESFCompanionStance NewStance);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Companion|Tactics")
	void SetAggression(ESFCompanionAggression NewAggression);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Companion|Tactics")
	void SetEngagementRange(ESFCompanionEngagementRange NewRange);

	// -------------------------------------------------------------------------
	// Orders
	// -------------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Companion|Order")
	const FSFCompanionOrder& GetActiveOrder() const { return ActiveOrder; }

	/** Authority-only. Bumps Sequence and broadcasts. Pass an order with Type=None to clear. */
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Companion|Order")
	void IssueOrder(const FSFCompanionOrder& NewOrder);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Companion|Order")
	void ClearOrder();

	// -------------------------------------------------------------------------
	// Priority thresholds (HealthLow, AbilityCooldown gating, Taunt range)
	// -------------------------------------------------------------------------

	/** Designer-tunable; the tactics layer evaluates these and broadcasts on cross. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Companion|Tactics")
	FSFCompanionTacticsThresholds Thresholds;

	UFUNCTION(BlueprintPure, Category = "Companion|Tactics")
	const FSFCompanionTacticsThresholds& GetThresholds() const { return Thresholds; }

	UFUNCTION(BlueprintPure, Category = "Companion|Tactics")
	bool IsPlayerLowHealth() const { return bPlayerLowHealth; }

	UFUNCTION(BlueprintPure, Category = "Companion|Tactics")
	bool IsSelfLowHealth() const { return bSelfLowHealth; }

	/** Returns true if the commanded-ability cooldown has elapsed since the last use. */
	UFUNCTION(BlueprintPure, Category = "Companion|Tactics")
	bool CanCommandAbility() const;

	/** Authority-only. Stamps the last-ability time so CanCommandAbility() begins gating. */
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Companion|Tactics")
	void NotifyAbilityCommanded();

	/**
	 * Authority-only. Tactics layer calls this from a health-change hook on
	 * the owning pawn (and from the player's health hook if bound) to update
	 * derived flags and fire OnThresholdCrossed.
	 */
	void EvaluateHealthThresholds(float SelfPct, float PlayerPct);

	// -------------------------------------------------------------------------
	// Delegates
	// -------------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Companion|Tactics")
	FSFCompanionStanceChanged OnStanceChanged;

	UPROPERTY(BlueprintAssignable, Category = "Companion|Order")
	FSFCompanionOrderIssued OnOrderIssued;

	UPROPERTY(BlueprintAssignable, Category = "Companion|Tactics")
	FSFCompanionThresholdCrossed OnThresholdCrossed;

protected:
	// -------------------------------------------------------------------------
	// State (replicated)
	// -------------------------------------------------------------------------

	UPROPERTY(ReplicatedUsing = OnRep_Stance, EditAnywhere, BlueprintReadOnly, Category = "Companion|Tactics")
	ESFCompanionStance Stance = ESFCompanionStance::DPS;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Companion|Tactics")
	ESFCompanionAggression Aggression = ESFCompanionAggression::Defensive;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Companion|Tactics")
	ESFCompanionEngagementRange EngagementRange = ESFCompanionEngagementRange::Skirmish;

	UPROPERTY(ReplicatedUsing = OnRep_ActiveOrder, BlueprintReadOnly, Category = "Companion|Order")
	FSFCompanionOrder ActiveOrder;

	UPROPERTY(BlueprintReadOnly, Category = "Companion|Tactics")
	FGameplayTagContainer ActiveStanceTags;

	// -------------------------------------------------------------------------
	// Designer-tunable per-stance tag bundles
	// -------------------------------------------------------------------------

	/** Tags applied while in Tank stance. Default: Companion.Stance.Tank, Companion.Behavior.HoldFront, etc. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Companion|Tactics", meta = (Categories = "Companion"))
	FGameplayTagContainer TankStanceTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Companion|Tactics", meta = (Categories = "Companion"))
	FGameplayTagContainer DPSStanceTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Companion|Tactics", meta = (Categories = "Companion"))
	FGameplayTagContainer HealerStanceTags;

	/** Used when Stance == Custom. Designers fill these by hand. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Companion|Tactics", meta = (Categories = "Companion"))
	FGameplayTagContainer CustomStanceTags;

	UFUNCTION()
	void OnRep_Stance(ESFCompanionStance OldStance);

	UFUNCTION()
	void OnRep_ActiveOrder();

	void RefreshStanceTags();
	void ApplyStanceTagsToAbilitySystem();

	int32 NextOrderSequence = 1;
	int32 LastObservedOrderSequence = 0;

	// Derived threshold flags (server-driven; not replicated — each peer
	// recomputes locally from health attributes).
	bool bPlayerLowHealth = false;
	bool bSelfLowHealth = false;

	/** Game-time of the last commanded-ability fire, for cooldown gating. */
	double LastCommandedAbilityTime = -BIG_NUMBER;
};
