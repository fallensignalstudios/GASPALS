#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SFGameplayAbility.h"
#include "SFGameplayAbility_ADS.generated.h"

class ASFCharacterBase;
class UCameraComponent;
class USpringArmComponent;

/**
 * Aim-down-sights gameplay ability.
 *
 * Hold-to-aim ability granted by ranged weapons. While the input is held it:
 *  - Applies the `State.Weapon.ADS` loose gameplay tag so `USFGameplayAbility_WeaponFire`
 *    automatically swaps to the tighter `AdsSpreadDegrees` cone and reduced recoil.
 *  - Smoothly interpolates camera FOV, spring-arm length, and socket offset toward the
 *    weapon's `FSFRangedWeaponConfig::AdsCameraFOV / AdsSpringArmLength / AdsSpringArmSocketOffset`.
 *  - Slows the player by `AdsMovementSpeedMultiplier` while aimed.
 *  - Optionally plays an `AdsPoseMontage` for raised-rifle additive pose.
 *  - Fires `Cue.Weapon.ADS.Enter` / `Cue.Weapon.ADS.Exit` gameplay cues for scope SFX,
 *    lens overlay, vignette burst, slight time-dilation, etc.
 *
 * Activation: WhileInputActive (RMB / left-trigger hold).
 * Net policy: LocalPredicted — the local player must feel ADS snap immediately.
 *
 * If no ranged weapon is equipped (or the equipped weapon's FSFRangedWeaponConfig has
 * `AdsCameraFOV == 0`), this ability ends early so the camera isn't hijacked unnecessarily.
 */
UCLASS()
class SIGNALFORGERPG_API USFGameplayAbility_ADS : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGameplayAbility_ADS();

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
	/** Tick rate (seconds) of the ADS interpolation timer. Smaller = smoother. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ADS", meta = (ClampMin = "0.005", ClampMax = "0.1"))
	float InterpTickInterval = 0.016f;

	/** Optional GameplayEffect applied while ADS (e.g. accuracy attribute boost). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ADS")
	TSubclassOf<UGameplayEffect> AdsAttributeEffectClass;

	/** Hook for Blueprints — fires once when ADS engages, after cached state is captured. */
	UFUNCTION(BlueprintImplementableEvent, Category = "ADS")
	void BP_OnAdsEntered();

	/** Hook for Blueprints — fires when ADS releases, before camera blends back. */
	UFUNCTION(BlueprintImplementableEvent, Category = "ADS")
	void BP_OnAdsExited();

private:
	/** Per-tick blend toward the target ADS pose (called from a looping timer). */
	UFUNCTION()
	void TickAdsBlend();

	/** Resolves camera + spring arm components on the avatar (works with SFPlayerCharacter). */
	void ResolveViewComponents(ASFCharacterBase* Character, UCameraComponent*& OutCamera, USpringArmComponent*& OutBoom) const;

	/** Captures the current camera/spring-arm values so EndAbility can restore them. */
	void CaptureBaselineState(ASFCharacterBase* Character);

	/** Begins blending out to the captured baseline, then ends the ability. */
	void BeginAdsExit();

	FGameplayAbilitySpecHandle CachedHandle;
	const FGameplayAbilityActorInfo* CachedActorInfo = nullptr;
	FGameplayAbilityActivationInfo CachedActivationInfo;

	FTimerHandle InterpTimerHandle;
	FActiveGameplayEffectHandle AdsEffectHandle;

	// --- Baseline (pre-ADS) ---
	float BaselineCameraFOV = 90.0f;
	float BaselineArmLength = 350.0f;
	FVector BaselineSocketOffset = FVector::ZeroVector;
	float BaselineMaxWalkSpeed = 450.0f;

	// --- Target (read from FSFRangedWeaponConfig at activation) ---
	float TargetCameraFOV = 55.0f;
	float TargetArmLength = 150.0f;
	FVector TargetSocketOffset = FVector::ZeroVector;
	float TargetMaxWalkSpeed = 250.0f;

	// --- Blend tracking ---
	float BlendInSeconds = 0.18f;
	float BlendOutSeconds = 0.12f;
	bool bIsBlendingOut = false;
	bool bCachedBaseline = false;
};
