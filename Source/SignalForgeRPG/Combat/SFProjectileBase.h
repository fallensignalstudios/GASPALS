#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SFProjectileBase.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UGameplayEffect;
class UNiagaraSystem;

UCLASS()
class SIGNALFORGERPG_API ASFProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	ASFProjectileBase();

	virtual void BeginPlay() override;

	void SetSourceActor(AActor* InSourceActor);
	void SetDamageEffect(TSubclassOf<UGameplayEffect> InDamageEffect);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float LifeSeconds = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UNiagaraSystem> ImpactNiagara;

	UPROPERTY()
	TObjectPtr<AActor> SourceActor;

	UPROPERTY()
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UFUNCTION()
	void OnProjectileHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

	void HandleImpact(AActor* OtherActor, const FHitResult& Hit);
};