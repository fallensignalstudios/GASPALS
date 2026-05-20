#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SFGameplayAbility.h"
#include "SFGameplayAbility_Dash.generated.h"

class UAnimMontage;
class UNiagaraSystem;
class ASFCharacterBase;
class UCharacterMovementComponent;

/**
 * Movement burst ability ("Destiny-style" dodge/dash).
 *
 * Tuned for crisp, weighty horizontal blink:
 *   - Zeros current velocity before launch so the impulse is consistent.
 *   - Suspends gravity for DashDuration to prevent vertical dip mid-dash.
 *   - Sets BrakingFrictionFactor to zero during the dash to keep speed snappy.
 *   - Adds State.Movement.Dashing and (optionally) State.Invulnerable.Dodge
 *     for the dash window so damage handlers / animation BPs can react.
 *   - Direction defaults to LastMovementInput; falls back to forward when
 *     there is no input (forward dash) so neutral dashes still trigger.
 *
 * Authoring:
 *   - Assign ActivationMontage on the base class to play the dodge animation
 *     (DashMontage stays as a legacy fallback for content not yet migrated).
 *   - Configure Cost / Cooldown Gameplay Effects on the GA in the editor so
 *     CommitAbility handles Echo cost and dash cooldown.
 */
UCLASS()
class SIGNALFORGERPG_API USFGameplayAbility_Dash : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGameplayAbility_Dash();

	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	/** Launch impulse magnitude applied along the resolved dash direction. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash", meta = (ClampMin = "0.0"))
	float DashStrength = 1500.0f;

	/** How long the "dash window" lasts (gravity off, friction zero, i-frames on). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash", meta = (ClampMin = "0.05", ClampMax = "1.5"))
	float DashDuration = 0.30f;

	/** When true, the character takes no damage during the dash window (i-frames). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	bool bGrantInvulnerabilityDuringDash = true;

	/** When true, vertical velocity is cleared on activation (keeps the dash horizontal even if airborne). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	bool bZeroVelocityBeforeLaunch = true;

	/** When true, gravity is disabled during DashDuration (LaunchCharacter alone otherwise dips). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	bool bSuspendGravityDuringDash = true;

	/** Legacy montage; prefer ActivationMontage on the base class. Falls back to this when set. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	TObjectPtr<UAnimMontage> DashMontage = nullptr;

	/** Legacy Niagara; prefer ActivationVFX on the base class. Falls back to this when set. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	TObjectPtr<UNiagaraSystem> DashNiagara = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dash")
	bool bUseLastMovementInputForDash = true;

private:
	bool TryGetDashCharacter(const FGameplayAbilityActorInfo* ActorInfo, ASFCharacterBase*& OutCharacter) const;
	FVector ResolveDashDirection(const ASFCharacterBase* Character) const;

	void BeginDashWindow(ASFCharacterBase* Character);
	void EndDashWindow();

	UFUNCTION()
	void HandleDashWindowExpired();

	/** Cached so we can restore movement settings after the window. */
	TWeakObjectPtr<UCharacterMovementComponent> CachedMovementComp;
	float CachedGravityScale = 1.0f;
	float CachedBrakingFrictionFactor = 1.0f;
	bool bDashWindowActive = false;
	FTimerHandle DashWindowTimer;
};
