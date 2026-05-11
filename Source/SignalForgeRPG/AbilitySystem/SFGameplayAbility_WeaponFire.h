#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SFGameplayAbility.h"
#include "GameplayTagContainer.h"
#include "SFGameplayAbility_WeaponFire.generated.h"

class ASFCharacterBase;
class ASFWeaponActor;
class ASFProjectileBase;
class UGameplayEffect;
class UCameraShakeBase;
class UAnimMontage;
struct FHitResult;
struct FSFRangedWeaponConfig;
class USFEquipmentComponent;
class USFWeaponData;

/**
 * Default ranged-weapon fire ability.
 *
 * Reads its tuning from the currently equipped weapon's FSFRangedWeaponConfig
 * (fire mode, RPM, spread, recoil, falloff, hitscan vs projectile, pellet count,
 * headshot multiplier, camera shake, cues). Weapons grant this ability when they
 * are equipped, so every rifle / pistol / shotgun can share the same "bullets feel"
 * code path while still being individually tunable.
 *
 * Bullets feel:
 *  - Cycle time derived from RPM, blocking subsequent triggers during the cycle.
 *  - Spread cone driven by HipfireSpreadDegrees / AdsSpreadDegrees.
 *  - Vertical + horizontal recoil applied to the controller per shot.
 *  - Camera shake + GameplayCues for muzzle flash, tracer and shell ejection.
 *  - Distance-based damage falloff applied via SetByCaller magnitudes.
 *  - Headshot detection via bone name on the hit result.
 *  - Burst mode fires BurstCount rounds spaced by BurstIntervalSeconds.
 *  - Empty clip plays optional dry-fire montage + cue.
 */
UCLASS()
class SIGNALFORGERPG_API USFGameplayAbility_WeaponFire : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGameplayAbility_WeaponFire();

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
	/** Fallback config used when the equipped weapon has no override (or there is no weapon). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponFire|Defaults", meta = (ClampMin = "0.0"))
	float DefaultDamage = 18.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponFire|Defaults")
	TSubclassOf<UGameplayEffect> DefaultDamageEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponFire|Defaults")
	TSubclassOf<UCameraShakeBase> DefaultCameraShake;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponFire|Defaults")
	TSubclassOf<ASFProjectileBase> DefaultProjectileClass;

	/** Trace channel used for hitscan firing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponFire|Hitscan")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** Bones treated as headshots for damage scaling. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponFire|Hitscan")
	TArray<FName> HeadshotBones = { TEXT("head"), TEXT("neck"), TEXT("neck_01") };

	/** Camera shake radius around the firing player. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponFire|Feedback", meta = (ClampMin = "100.0"))
	float CameraShakeRadius = 2500.0f;

private:
	/** Top-level "pull the trigger" path: handles single/burst/auto branching. */
	void HandleTriggerPull();

	/** Fire one logical shot (which may include multiple pellets for shotguns). */
	void FireOneShot();

	void FireHitscanPellet(
		ASFCharacterBase* Character,
		ASFWeaponActor* WeaponActor,
		const FSFRangedWeaponConfig& Config,
		const FVector& MuzzleLocation,
		const FRotator& AimRotation);

	void FireProjectilePellet(
		ASFCharacterBase* Character,
		ASFWeaponActor* WeaponActor,
		const FSFRangedWeaponConfig& Config,
		const FVector& MuzzleLocation,
		const FRotator& AimRotation);

	void ApplyHitscanDamage(
		ASFCharacterBase* Character,
		const FSFRangedWeaponConfig& Config,
		const FHitResult& Hit,
		const FVector& MuzzleLocation);

	void DispatchMuzzleAndTracerCues(
		ASFCharacterBase* Character,
		ASFWeaponActor* WeaponActor,
		const FSFRangedWeaponConfig& Config,
		const FVector& MuzzleLocation,
		const FRotator& MuzzleRotation,
		const FVector& EndLocation);

	void ApplyRecoilAndShake(
		ASFCharacterBase* Character,
		const FSFRangedWeaponConfig& Config);

	/** Determines if the equipped weapon is in ADS state. */
	bool IsAimingDownSights(ASFCharacterBase* Character) const;

	/** Compute a spread direction within a cone around BaseRotation. */
	FRotator ApplyConeSpread(const FRotator& BaseRotation, float HalfAngleDegrees) const;

	/** Pulls Config + Weapon + Equipment from the avatar; returns false if not a ranged weapon. */
	bool ResolveRangedContext(
		ASFCharacterBase* Character,
		USFEquipmentComponent*& OutEquipment,
		USFWeaponData*& OutWeaponData,
		ASFWeaponActor*& OutWeaponActor,
		FSFRangedWeaponConfig& OutConfig) const;

	/** Attempt to consume ammo from the weapon instance; returns true on success. */
	bool TryConsumeAmmo(USFEquipmentComponent* Equipment, const FSFRangedWeaponConfig& Config) const;

	/** Fire dry-click cue + montage + sound when out of ammo. */
	void HandleEmptyClick(ASFCharacterBase* Character, const FSFRangedWeaponConfig& Config);

	/** Finishes the ability cleanly (clears cached state). */
	void FinishAbility(bool bWasCancelled);

private:
	FGameplayAbilitySpecHandle CachedHandle;
	const FGameplayAbilityActorInfo* CachedActorInfo = nullptr;
	FGameplayAbilityActivationInfo CachedActivationInfo;

	FTimerHandle BurstTimerHandle;
	FTimerHandle CycleTimerHandle;

	int32 BurstShotsRemaining = 0;
	bool bIsCycling = false;
};
