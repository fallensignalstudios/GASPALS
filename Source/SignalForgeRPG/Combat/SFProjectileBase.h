#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "SFProjectileBase.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UGameplayEffect;
class UNiagaraSystem;
class USoundBase;

/**
 * Generic ranged projectile.
 *
 * Supports bullets, plasma bolts, rockets and grenade fragments.
 * Damage is applied via a GameplayEffect with optional falloff multiplier and
 * an optional radial blast on impact. Audio cues, gameplay cues, and source-actor
 * attribution let the cinematic combat layer (USFCombatComponent) react to hits.
 */
UCLASS()
class SIGNALFORGERPG_API ASFProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	ASFProjectileBase();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SetSourceActor(AActor* InSourceActor);

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SetDamageEffect(TSubclassOf<UGameplayEffect> InDamageEffect);

	/** Designer/runtime hook: configure base damage applied per hit. */
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SetBaseDamage(float InDamage) { BaseDamage = InDamage; }

	/** Configures linear damage falloff between Start and End distances down to MinMultiplier. */
	UFUNCTION(BlueprintCallable, Category = "Projectile|Falloff")
	void SetDamageFalloff(float InStart, float InEnd, float InMinMultiplier);

	/** Per-shot opt-in: when true, this projectile bypasses the friend-foe gate at impact. */
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SetAllowFriendlyFire(bool bInAllow) { bAllowFriendlyFire = bInAllow; }

	UFUNCTION(BlueprintPure, Category = "Projectile")
	AActor* GetSourceActor() const { return SourceActor; }

	/** Multiply the projectile's InitialSpeed (and MaxSpeed if needed) by Multiplier. Must be
	 *  called before BeginPlay finishes -- use SpawnActorDeferred + this + FinishSpawning. */
	UFUNCTION(BlueprintCallable, Category = "Projectile|Movement")
	void SetSpeedMultiplier(float Multiplier);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0.1"))
	float LifeSeconds = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Movement", meta = (ClampMin = "0.0"))
	float InitialSpeed = 7500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Movement", meta = (ClampMin = "0.0"))
	float MaxSpeed = 12000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Movement", meta = (ClampMin = "0.0"))
	float GravityScale = 0.0f;

	/** If true, projectile can bounce off surfaces rather than detonate on first contact. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Movement")
	bool bRicochet = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Movement", meta = (ClampMin = "0", EditCondition = "bRicochet"))
	int32 MaxRicochets = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Movement", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bRicochet"))
	float RicochetSpeedRetention = 0.65f;

	/** Base damage applied through DamageEffectClass on hit (SetByCaller magnitude). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Damage", meta = (ClampMin = "0.0"))
	float BaseDamage = 18.0f;

	/** Distance at which damage starts falling off (cm from spawn). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Damage", meta = (ClampMin = "0.0"))
	float FalloffStartDistance = 2500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Damage", meta = (ClampMin = "0.0"))
	float FalloffEndDistance = 7500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Damage", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinFalloffMultiplier = 0.35f;

	/** Optional radial damage on impact (e.g. rockets, plasma orbs). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Radial", meta = (ClampMin = "0.0"))
	float RadialRadius = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Radial", meta = (ClampMin = "0.0", EditCondition = "RadialRadius > 0.0"))
	float RadialFalloff = 1.0f;

	/** Optional impact VFX. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Feedback")
	TObjectPtr<UNiagaraSystem> ImpactNiagara;

	/** Optional looping in-flight VFX (attached). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Feedback")
	TObjectPtr<UNiagaraSystem> TrailNiagara;

	/** Optional impact sound. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Feedback")
	TObjectPtr<USoundBase> ImpactSound;

	/** Optional launch sound (one-shot at spawn). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Feedback")
	TObjectPtr<USoundBase> LaunchSound;

	/** Optional gameplay cue executed on the hit target ASC (override of the default Cue_Hit_Impact). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Feedback", meta = (Categories = "SignalForge.Cue"))
	FGameplayTag ImpactCueTagOverride;

	UPROPERTY()
	TObjectPtr<AActor> SourceActor;

	UPROPERTY()
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/**
	 * If true, hits on non-hostile characters still deal damage. Off by default; weapons opt in
	 * via FSFRangedWeaponConfig::bAllowFriendlyFire and propagate the flag at spawn time.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Damage")
	bool bAllowFriendlyFire = false;

	/** Tracks where we spawned so damage falloff can be applied. */
	FVector SpawnLocation = FVector::ZeroVector;

	/** Ricochets used so far. */
	int32 RicochetCount = 0;

	UFUNCTION()
	void OnProjectileHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

	/** Calculates damage multiplier at a given world hit location. */
	float ComputeFalloffMultiplier(const FVector& HitLocation) const;

	/** Applies single-target damage + cues to OtherActor. */
	void HandleImpact(AActor* OtherActor, const FHitResult& Hit);

	/** Applies radial damage (used when RadialRadius > 0). */
	void HandleRadialImpact(const FHitResult& Hit);
};
