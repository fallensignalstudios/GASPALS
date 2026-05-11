#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SFGameplayAbility.h"
#include "SFGameplayAbility_ThrowGrenade.generated.h"

class ASFGrenadeBase;
class ASFCharacterBase;

/**
 * Default grenade-throw ability.
 *
 * Input policy: WhileInputActive — the player presses to start "cooking" the grenade,
 * and releases to throw. The longer they hold, the further (and less safely) it flies.
 *
 *  - Spawns ASFGrenadeBase at GrenadeSpawnSocket on the owner mesh on release.
 *  - Throw velocity is interpolated between MinThrowSpeed and MaxThrowSpeed based on
 *    cook time (clamped to MaxCookSeconds; cooking past that point detonates in-hand).
 *  - Optional BlueprintImplementableEvent BP_UpdateArcPreview lets BP draw a parabolic
 *    arc indicator while cooking.
 *  - Adds State.Grenade.Cooking while charging so animation / UI can react.
 */
UCLASS()
class SIGNALFORGERPG_API USFGameplayAbility_ThrowGrenade : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGameplayAbility_ThrowGrenade();

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
	/** Grenade actor class spawned on throw. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade")
	TSubclassOf<ASFGrenadeBase> GrenadeClass;

	/** Socket on the owner mesh where the grenade should spawn from (e.g. throw hand). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade")
	FName GrenadeSpawnSocket = TEXT("hand_r");

	/** Whether holding the input cooks the grenade (true) or fires immediately on activate (false). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade")
	bool bAllowCooking = true;

	/** Maximum cook duration in seconds; past this point the grenade explodes in-hand. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade", meta = (ClampMin = "0.1"))
	float MaxCookSeconds = 2.5f;

	/** Tap throw (cook time <= 0) launch speed (cm/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Throw", meta = (ClampMin = "0.0"))
	float MinThrowSpeed = 900.0f;

	/** Max charge launch speed (cm/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Throw", meta = (ClampMin = "0.0"))
	float MaxThrowSpeed = 1800.0f;

	/** Upward bias applied to the throw vector, in degrees (0 = straight forward). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Throw", meta = (ClampMin = "-45.0", ClampMax = "60.0"))
	float UpwardThrowAngleDegrees = 15.0f;

	/** Optional throw montage (the actual spawn happens in InputReleased so this is purely cosmetic). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Feedback")
	TObjectPtr<UAnimMontage> ThrowMontage = nullptr;

	/** Blueprint hook called every tick while cooking, so BP can draw an arc preview. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Grenade")
	void BP_UpdateArcPreview(const FVector& StartLocation, const FVector& LaunchVelocity, float CookAlpha);

	/** Blueprint hook called when cooking starts. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Grenade")
	void BP_OnCookStart();

	/** Blueprint hook called when grenade is thrown. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Grenade")
	void BP_OnGrenadeThrown(ASFGrenadeBase* SpawnedGrenade, float CookSeconds);

private:
	UFUNCTION()
	void TickArcPreview();

	/** Spawn + launch the grenade with the elapsed cook time. */
	void ThrowGrenade(bool bExplodeInHand);

	float ComputeCookAlpha() const;
	FVector ComputeLaunchVelocity(ASFCharacterBase* Character, float CookAlpha) const;
	FVector ComputeSpawnLocation(ASFCharacterBase* Character) const;

	FGameplayAbilitySpecHandle CachedHandle;
	const FGameplayAbilityActorInfo* CachedActorInfo = nullptr;
	FGameplayAbilityActivationInfo CachedActivationInfo;

	double CookStartTime = 0.0;
	FTimerHandle ArcPreviewTimerHandle;
	FTimerHandle CookOverflowTimerHandle;
	bool bIsCooking = false;
	bool bHasThrown = false;
};
