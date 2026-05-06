#include "Combat/SFProjectileBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Components/StaticMeshComponent.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Input/SFPlayerController.h"
#include "Characters/SFCharacterBase.h"
#include "Characters/SFEnemyCharacter.h"

ASFProjectileBase::ASFProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->SetSphereRadius(16.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionSphere->SetNotifyRigidBodyCollision(true);

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(CollisionSphere);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 2000.0f;
	ProjectileMovement->MaxSpeed = 2000.0f;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
}

void ASFProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(LifeSeconds);

	if (CollisionSphere)
	{
		CollisionSphere->OnComponentHit.AddDynamic(this, &ASFProjectileBase::OnProjectileHit);
	}
}

void ASFProjectileBase::SetSourceActor(AActor* InSourceActor)
{
	SourceActor = InSourceActor;
}

void ASFProjectileBase::SetDamageEffect(TSubclassOf<UGameplayEffect> InDamageEffect)
{
	DamageEffectClass = InDamageEffect;
}

void ASFProjectileBase::OnProjectileHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	HandleImpact(OtherActor, Hit);
}

void ASFProjectileBase::HandleImpact(AActor* OtherActor, const FHitResult& Hit)
{
	if (!OtherActor || OtherActor == this || OtherActor == SourceActor)
	{
		return;
	}

	if (ImpactNiagara)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ImpactNiagara,
			Hit.ImpactPoint,
			Hit.ImpactNormal.Rotation());
	}

	if (DamageEffectClass && SourceActor && OtherActor)
	{
		UAbilitySystemComponent* SourceASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceActor);

		UAbilitySystemComponent* TargetASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);

		if (SourceASC && TargetASC)
		{
			FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
			EffectContext.AddSourceObject(this);

			const FGameplayEffectSpecHandle SpecHandle =
				SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, EffectContext);

			if (SpecHandle.IsValid())
			{
				if (ASFCharacterBase* SourceCharacter = Cast<ASFCharacterBase>(SourceActor))
				{
					if (ASFEnemyCharacter* HitEnemy = Cast<ASFEnemyCharacter>(OtherActor))
					{
						HitEnemy->SetLastDamagingCharacter(SourceCharacter);
					}
				}

				SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
			}
		}
	}

	if (ASFCharacterBase* SourceCharacter = Cast<ASFCharacterBase>(SourceActor))
	{
		if (ASFPlayerController* PlayerController = Cast<ASFPlayerController>(SourceCharacter->GetController()))
		{
			if (ASFCharacterBase* HitCharacter = Cast<ASFCharacterBase>(OtherActor))
			{
				PlayerController->SetTrackedEnemy(HitCharacter);
				UE_LOG(LogTemp, Warning, TEXT("Tracked enemy set from projectile hit: %s"), *GetNameSafe(HitCharacter));
			}
		}
	}

	Destroy();
}