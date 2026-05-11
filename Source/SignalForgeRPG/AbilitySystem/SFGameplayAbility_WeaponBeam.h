#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SFGameplayAbility.h"
#include "Combat/SFWeaponData.h"
#include "GameplayTagContainer.h"
#include "SFGameplayAbility_WeaponBeam.generated.h"

class ASFCharacterBase;
class ASFWeaponActor;
class UGameplayEffect;
class UCameraShakeBase;
class USFEquipmentComponent;
class USFWeaponData;
struct FHitResult;
struct FSFBeamWeaponConfig;
struct FSFRangedWeaponConfig;

/**
 * Continuous-beam fire ability (energy rifle, plasma cutter, trace rifle).
 *
 * Behaviour:
 *  - On trigger pull: ignite the beam (spawn loop cue), add State.Weapon.Firing tag,
 *    start a repeating tick timer at (RangedConfig.RoundsPerMinute / 60) Hz.
 *  - Each tick: line-trace muzzle->aim, apply BeamConfig.DamagePerTick, drain battery,
 *    apply per-tick recoil (RangedConfig recoil divided by ticks-per-second so DPS-
 *    equivalent muzzle climb matches bullet weapons of the same RPM).
 *  - On trigger release / overheat / cancel: stop the timer, fire BeamStop cue, clear tag.
 *  - When idle (not firing) for >= RechargeDelaySeconds the battery recharges at
 *    BeamConfig.RechargeUnitsPerSecond. Battery + overheat state live on the weapon
 *    instance so they survive holster / weapon switch.
 *
 * Rate-of-fire is unified with the rest of the system: RPM drives the beam tick rate
 * exactly like it drives cycle time for single/burst/full-auto weapons.
 */
UCLASS()
class SIGNALFORGERPG_API USFGameplayAbility_WeaponBeam : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGameplayAbility_WeaponBeam();

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

	virtual void InputReleased(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	/** Fallback damage effect when BeamConfig leaves DamageEffectClass null (shared with bullets). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Beam|Defaults")
	TSubclassOf<UGameplayEffect> DefaultDamageEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Beam|Defaults")
	TSubclassOf<UCameraShakeBase> DefaultCameraShake;

	/** Trace channel used for the beam hitscan. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Beam|Hitscan")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** Bones treated as headshots for damage scaling (only used if BeamConfig.bAllowHeadshots). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Beam|Hitscan")
	TArray<FName> HeadshotBones = { TEXT("head"), TEXT("neck"), TEXT("neck_01") };

	/** Camera shake radius around the firing player (per tick if FireCameraShake set on the ranged config). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Beam|Feedback", meta = (ClampMin = "100.0"))
	float CameraShakeRadius = 2500.0f;

	/**
	 * Minimum ticks per second. Caps RPM->Hz conversion against silly values like 12 RPM
	 * (which would give a 0.2 Hz beam). Anything <2 Hz feels like a slow strobe, not a beam.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Beam|Tuning", meta = (ClampMin = "1.0", ClampMax = "120.0"))
	float MinTicksPerSecond = 5.0f;

	/** Maximum ticks per second. Caps absurdly high RPM values (would otherwise stall the timer manager). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Beam|Tuning", meta = (ClampMin = "1.0", ClampMax = "240.0"))
	float MaxTicksPerSecond = 60.0f;

private:
	void BeginBeam();
	void BeamTick();
	void StopBeam(bool bOverheated);

	void ApplyBeamHit(
		ASFCharacterBase* Character,
		const FSFRangedWeaponConfig& RangedConfig,
		const FSFBeamWeaponConfig& BeamConfig,
		const FHitResult& Hit,
		const FVector& MuzzleLocation);

	void ApplyBeamRecoilForTick(
		ASFCharacterBase* Character,
		const FSFRangedWeaponConfig& RangedConfig,
		float TicksPerSecond);

	void DispatchBeamCue(const FGameplayTag& Tag, const FVector& Location, const FVector& Normal);

	/** Initialize/refresh battery (creates capacity on first run, drains 0 -> overheated). */
	void EnsureBatteryInitialized(USFEquipmentComponent* Equipment, const FSFBeamWeaponConfig& BeamConfig);

	bool ResolveBeamContext(
		ASFCharacterBase*& OutCharacter,
		USFEquipmentComponent*& OutEquipment,
		USFWeaponData*& OutWeaponData,
		ASFWeaponActor*& OutWeaponActor,
		FSFRangedWeaponConfig& OutRangedConfig,
		FSFBeamWeaponConfig& OutBeamConfig) const;

	bool IsAimingDownSights(ASFCharacterBase* Character) const;
	FRotator ApplyConeSpread(const FRotator& BaseRotation, float HalfAngleDegrees) const;

	/** Cleanly end the ability (clears cached state). */
	void FinishAbility(bool bWasCancelled);

private:
	FGameplayAbilitySpecHandle CachedHandle;
	const FGameplayAbilityActorInfo* CachedActorInfo = nullptr;
	FGameplayAbilityActivationInfo CachedActivationInfo;

	FTimerHandle BeamTickTimerHandle;

	/** True while the trigger is held and the beam is actively firing (not in overheat lockout). */
	bool bBeamActive = false;

	/** True while the player still holds the input. Released sets this false so the next BeamTick will stop. */
	bool bTriggerHeld = false;

	/** Cached tick rate (Hz) for the active beam; used for per-tick recoil scaling. */
	float CachedTicksPerSecond = 20.0f;
};
