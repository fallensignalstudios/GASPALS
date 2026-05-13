#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SFGameplayAbility.h"
#include "Combat/SFWeaponData.h"
#include "GameplayTagContainer.h"
#include "SFGameplayAbility_WeaponCast.generated.h"

class ASFCharacterBase;
class ASFProjectileBase;
class ASFWeaponActor;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAbilityTask_WaitInputRelease;
class UAnimMontage;
class UNiagaraComponent;
class USFEquipmentComponent;
class USFWeaponData;
struct FSFCasterWeaponConfig;

/**
 * Caster-weapon primary fire ability. Sibling of WeaponMelee / WeaponFire / WeaponBeam.
 *
 * Designer flow:
 *  1. Author a CastMontage. If charging is wanted, give it two sections: "Charge" (looping) and
 *     "Release". Place an SFAnimNotify_CastRelease on the frame of the Release section where the
 *     projectile should leave the character.
 *  2. Fill out FSFCasterWeaponConfig on the weapon DA. Required: ProjectileClass, CastMontage.
 *  3. Point USFWeaponData::PrimaryFireAbility at a BP subclass of this ability.
 *
 * Runtime flow (instant cast, bSupportsCharge=false):
 *  - Activate -> apply State.Weapon.Casting, play CastMontage end-to-end, wait for
 *    GameplayEvent.Cast.Release. On the event: commit mana cost, spawn projectile, fire release
 *    cue. On montage end: EndAbility.
 *
 * Runtime flow (charged cast, bSupportsCharge=true):
 *  - Activate -> apply State.Weapon.Casting + State.Weapon.Charging, play CastMontage starting at
 *    ChargeLoopSectionName (loops). Track charge time, drive ChargeCue magnitude.
 *  - On input release OR MaxChargeSeconds timer: jump montage to ReleaseSectionName, drop
 *    State.Weapon.Charging.
 *  - GameplayEvent.Cast.Release inside the release section: resolve tier from held duration,
 *    apply tier multipliers, commit mana (base + tier surcharge), spawn projectile.
 *  - Montage end -> EndAbility.
 *
 * Interruption (damage, swap, dispel): EndAbility(cancelled=true). State tags cleared, charge VFX
 * cleaned up, NO mana consumed, NO projectile spawned.
 */
UCLASS()
class SIGNALFORGERPG_API USFGameplayAbility_WeaponCast : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGameplayAbility_WeaponCast();

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
	/** Fallback projectile-base damage applied if the projectile DA does not define one
	 *  (or if you want to source damage from the ability rather than the projectile). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Caster|Damage", meta = (ClampMin = "0.0"))
	float FallbackBaseDamage = 25.0f;

	/** Cooldown / lockout time after release before the ability can re-activate. Optional --
	 *  most caster designs lean on the montage length as the natural lockout. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Caster|Cost", meta = (ClampMin = "0.0"))
	float PostReleaseLockoutSeconds = 0.0f;

private:
	// --- Cached activation state ---
	FGameplayAbilitySpecHandle CachedHandle;
	const FGameplayAbilityActorInfo* CachedActorInfo = nullptr;
	FGameplayAbilityActivationInfo CachedActivationInfo;
	TWeakObjectPtr<ASFCharacterBase> CachedCharacter;
	TWeakObjectPtr<USFEquipmentComponent> CachedEquipment;
	TWeakObjectPtr<UAnimMontage> CachedMontage;
	TWeakObjectPtr<UNiagaraComponent> ActiveChargeVFX;

	/** UPROPERTY so the GC keeps it alive. */
	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask = nullptr;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ReleaseEventTask = nullptr;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitInputRelease> InputReleaseTask = nullptr;

	/** World-clock seconds at which charge began (when bSupportsCharge). 0 if not charging. */
	double ChargeStartSeconds = 0.0;
	bool bIsCharging = false;
	bool bReleaseFired = false;
	bool bAppliedCastingTag = false;
	bool bAppliedChargingTag = false;
	FTimerHandle ChargeTimeoutHandle;

	// --- Helpers ---
	bool ResolveContext(
		ASFCharacterBase*& OutCharacter,
		USFEquipmentComponent*& OutEquipment,
		USFWeaponData*& OutWeaponData) const;

	/** Validates the resolved CasterConfig (montage + projectile non-null). */
	bool ValidateCasterConfig(const FSFCasterWeaponConfig& Config) const;

	/** Start the montage task. If charging, starts at ChargeLoopSection; else plays from the top. */
	bool StartMontage(const FSFCasterWeaponConfig& Config, ASFCharacterBase* Character, bool bStartCharging);

	/** Subscribe to the Cast.Release gameplay event. */
	void BindReleaseEvent();

	/** Subscribe to input-release for charge weapons. */
	void BindInputRelease();

	/** Compute charge tier index from held duration. -1 if not charging or no tiers. */
	int32 ComputeChargeTierIndex(const FSFCasterWeaponConfig& Config, float HeldSeconds) const;

	/** Compute released-charge duration clamped to MaxChargeSeconds. */
	float GetChargeSeconds(const FSFCasterWeaponConfig& Config) const;

	/** Jump to the release section. Idempotent. */
	void TransitionToRelease(const FSFCasterWeaponConfig& Config);

	/** Spawn the projectile at the resolved socket aimed along the camera vector.
	 *  Applies the tier multipliers (damage, speed, scale) and dispatches the release cue. */
	void SpawnProjectile(
		ASFCharacterBase* Character,
		USFEquipmentComponent* Equipment,
		const USFWeaponData* WeaponData,
		const FSFCasterWeaponConfig& Config,
		int32 TierIndex);

	/** Commit the mana cost (base + tier surcharge). Pure GE application; never blocks. */
	void ApplyManaCost(
		ASFCharacterBase* Character,
		const FSFCasterWeaponConfig& Config,
		int32 TierIndex);

	/** Resolve the world-space spawn transform from socket + actor / mesh. */
	FTransform ResolveSpawnTransform(
		const ASFCharacterBase* Character,
		USFEquipmentComponent* Equipment,
		const FSFCasterWeaponConfig& Config) const;

	/** Camera-aligned aim direction (eyes view point). */
	FVector ResolveAimDirection(const ASFCharacterBase* Character) const;

	void StartChargeVFX(ASFCharacterBase* Character, USFEquipmentComponent* Equipment, const FSFCasterWeaponConfig& Config);
	void StopChargeVFX();

	void AddStateTag(const FGameplayTag& Tag, bool& bFlag);
	void RemoveStateTag(const FGameplayTag& Tag, bool& bFlag);

	// --- Task callbacks ---
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnMontageBlendOut();

	UFUNCTION()
	void OnReleaseEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void OnInputReleased(float TimeHeld);

	/** Fires when the player exceeds MaxChargeSeconds without releasing -- auto-release. */
	void OnChargeTimeoutFired();

	void FinishAbility(bool bWasCancelled);
};
