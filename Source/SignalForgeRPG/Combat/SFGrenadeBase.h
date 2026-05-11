#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "SFGrenadeBase.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;
class UNiagaraSystem;
class USoundBase;
class UGameplayEffect;

/**
 * Throwable grenade base actor.
 *
 * Provides:
 *  - Physics-driven flight via UProjectileMovementComponent with bounce + friction.
 *  - Per-bounce GameplayCue dispatch (Cue.Grenade.Bounce) for clinks/audio.
 *  - Fuse timer (FuseSeconds) that triggers Explode().
 *  - Optional cook-in-hand: SetCookedTime() lets the thrower pre-burn the fuse.
 *  - Optional explode-on-impact for sticky / contact grenades.
 *  - Radial damage on explode with inner-radius (full damage) + outer-radius (falloff).
 *  - GameplayCue.Grenade.Explode dispatch on detonation for the cinematic combat layer.
 *
 * The associated ability (USFGameplayAbility_ThrowGrenade) handles the input flow,
 * spawn transform, and throw velocity. This actor only knows about flight + detonation.
 */
UCLASS()
class SIGNALFORGERPG_API ASFGrenadeBase : public AActor
{
	GENERATED_BODY()

public:
	ASFGrenadeBase();

	virtual void BeginPlay() override;

	/** Configure the source of the throw so damage is attributed correctly. */
	UFUNCTION(BlueprintCallable, Category = "Grenade")
	void SetThrower(AActor* InThrower);

	/** Reduces the fuse by `CookedSeconds`. Call before BeginPlay (during spawn) or right after. */
	UFUNCTION(BlueprintCallable, Category = "Grenade")
	void SetCookedTime(float CookedSeconds);

	/** Override the fuse duration before BeginPlay. */
	UFUNCTION(BlueprintCallable, Category = "Grenade")
	void SetFuseTime(float InFuseSeconds) { FuseSeconds = FMath::Max(0.0f, InFuseSeconds); }

	/** Launch impulse helper (called by the throw ability). */
	UFUNCTION(BlueprintCallable, Category = "Grenade")
	void LaunchGrenade(const FVector& LaunchVelocity);

	UFUNCTION(BlueprintCallable, Category = "Grenade")
	void Explode();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grenade")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grenade")
	TObjectPtr<UStaticMeshComponent> GrenadeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grenade")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	/** Time until detonation in seconds. Reduced by cook-in-hand time. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade")
	float FuseSeconds = 3.5f;

	/** If true, detonates on first impact (sticky / impact grenade). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade")
	bool bExplodeOnImpact = false;

	/** Distance (cm) at which the grenade applies full damage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Damage", meta = (ClampMin = "0.0"))
	float InnerRadius = 250.0f;

	/** Distance (cm) at which damage falls to zero. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Damage", meta = (ClampMin = "0.0"))
	float OuterRadius = 700.0f;

	/** Falloff exponent applied between inner and outer radius (1.0 = linear). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Damage", meta = (ClampMin = "0.1"))
	float DamageFalloff = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Damage", meta = (ClampMin = "0.0"))
	float BaseDamage = 120.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/** Optional knockback impulse magnitude applied to physics actors in the blast. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Damage", meta = (ClampMin = "0.0"))
	float KnockbackStrength = 1000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Movement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Bounciness = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Movement", meta = (ClampMin = "0.0"))
	float Friction = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Movement")
	float GravityScale = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Feedback")
	TObjectPtr<UNiagaraSystem> ExplosionNiagara;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Feedback")
	TObjectPtr<USoundBase> ExplosionSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Feedback")
	TObjectPtr<USoundBase> BounceSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Feedback", meta = (Categories = "SignalForge.Cue"))
	FGameplayTag ExplosionCueOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade|Feedback", meta = (Categories = "SignalForge.Cue"))
	FGameplayTag BounceCueOverride;

	UPROPERTY()
	TObjectPtr<AActor> Thrower;

	/** Time already burned off the fuse by cooking in hand. */
	float CookedSeconds = 0.0f;

	FTimerHandle FuseTimerHandle;
	bool bHasExploded = false;

	UFUNCTION()
	void OnHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

	UFUNCTION()
	void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

	void DispatchBounceCue(const FHitResult& Hit);
	void DispatchExplodeCue(const FVector& Location);
};
