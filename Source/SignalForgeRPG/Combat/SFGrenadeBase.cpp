#include "Combat/SFGrenadeBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Core/SignalForgeGameplayTags.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayCueManager.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

ASFGrenadeBase::ASFGrenadeBase()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->SetSphereRadius(8.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionSphere->SetNotifyRigidBodyCollision(true);

	GrenadeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrenadeMesh"));
	GrenadeMesh->SetupAttachment(CollisionSphere);
	GrenadeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->Bounciness = Bounciness;
	ProjectileMovement->Friction = Friction;
	ProjectileMovement->ProjectileGravityScale = GravityScale;
	ProjectileMovement->bRotationFollowsVelocity = false;
	ProjectileMovement->bIsHomingProjectile = false;
	ProjectileMovement->InitialSpeed = 0.f;
	ProjectileMovement->MaxSpeed = 4000.f;
}

void ASFGrenadeBase::BeginPlay()
{
	Super::BeginPlay();

	// Push designer values into the movement component.
	if (ProjectileMovement)
	{
		ProjectileMovement->Bounciness = Bounciness;
		ProjectileMovement->Friction = Friction;
		ProjectileMovement->ProjectileGravityScale = GravityScale;
		ProjectileMovement->OnProjectileBounce.AddDynamic(this, &ASFGrenadeBase::OnBounce);
	}

	if (CollisionSphere)
	{
		CollisionSphere->OnComponentHit.AddDynamic(this, &ASFGrenadeBase::OnHit);
		if (Thrower)
		{
			CollisionSphere->IgnoreActorWhenMoving(Thrower, true);
		}
	}

	// Start (or skip) the fuse — cook-in-hand may have already burned it.
	if (UWorld* World = GetWorld())
	{
		const float RemainingFuse = FMath::Max(0.0f, FuseSeconds - CookedSeconds);
		if (RemainingFuse <= 0.0f)
		{
			// Detonate next tick so spawn-time effects can play.
			World->GetTimerManager().SetTimerForNextTick(this, &ASFGrenadeBase::Explode);
		}
		else
		{
			World->GetTimerManager().SetTimer(
				FuseTimerHandle,
				this,
				&ASFGrenadeBase::Explode,
				RemainingFuse,
				false);
		}
	}
}

void ASFGrenadeBase::SetThrower(AActor* InThrower)
{
	Thrower = InThrower;
	if (Thrower && CollisionSphere)
	{
		CollisionSphere->IgnoreActorWhenMoving(Thrower, true);
	}
}

void ASFGrenadeBase::SetCookedTime(float CookedSecondsIn)
{
	CookedSeconds = FMath::Max(0.0f, CookedSecondsIn);
}

void ASFGrenadeBase::LaunchGrenade(const FVector& LaunchVelocity)
{
	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = LaunchVelocity;
	}
}

void ASFGrenadeBase::OnHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (bExplodeOnImpact && OtherActor && OtherActor != Thrower && !bHasExploded)
	{
		Explode();
	}
}

void ASFGrenadeBase::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (BounceSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BounceSound, ImpactResult.ImpactPoint);
	}
	DispatchBounceCue(ImpactResult);
}

void ASFGrenadeBase::Explode()
{
	if (bHasExploded)
	{
		return;
	}
	bHasExploded = true;

	UWorld* World = GetWorld();
	if (!World)
	{
		Destroy();
		return;
	}

	const FVector ExplosionLocation = GetActorLocation();

	if (ExplosionNiagara)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, ExplosionNiagara, ExplosionLocation);
	}
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, ExplosionLocation);
	}

	DispatchExplodeCue(ExplosionLocation);

	// Sweep for damage targets within the outer radius.
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GrenadeExplode), false, this);
	if (Thrower)
	{
		Params.AddIgnoredActor(Thrower);
	}
	World->OverlapMultiByObjectType(
		Overlaps,
		ExplosionLocation,
		FQuat::Identity,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllDynamicObjects),
		FCollisionShape::MakeSphere(FMath::Max(InnerRadius, OuterRadius)),
		Params);

	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();

	UAbilitySystemComponent* SourceASC = Thrower
		? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Thrower)
		: nullptr;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (!Target || Target == this || Target == Thrower)
		{
			continue;
		}

		const float Distance = FVector::Dist(Target->GetActorLocation(), ExplosionLocation);
		float Multiplier = 1.0f;
		if (Distance > InnerRadius)
		{
			if (Distance >= OuterRadius)
			{
				continue;
			}
			const float Alpha = (Distance - InnerRadius) / FMath::Max(1.0f, (OuterRadius - InnerRadius));
			Multiplier = FMath::Pow(1.0f - Alpha, DamageFalloff);
		}

		const float FinalDamage = BaseDamage * Multiplier;

		// Apply damage via gameplay effect if both sides have ASCs.
		if (DamageEffectClass && SourceASC)
		{
			if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target))
			{
				FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
				Context.AddSourceObject(this);

				FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, Context);
				if (SpecHandle.IsValid())
				{
					if (Tags.Data_BaseDamage.IsValid())
					{
						SpecHandle.Data->SetSetByCallerMagnitude(Tags.Data_BaseDamage, FinalDamage);
					}
					SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
				}
			}
		}

		// Knockback for physics-driven props / ragdolls.
		if (KnockbackStrength > 0.0f)
		{
			if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Target->GetRootComponent()))
			{
				if (PrimComp->IsSimulatingPhysics())
				{
					const FVector Direction = (Target->GetActorLocation() - ExplosionLocation).GetSafeNormal();
					PrimComp->AddImpulse(Direction * KnockbackStrength * Multiplier, NAME_None, true);
				}
			}
		}
	}

	Destroy();
}

void ASFGrenadeBase::DispatchBounceCue(const FHitResult& Hit)
{
	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
	const FGameplayTag CueTag = BounceCueOverride.IsValid() ? BounceCueOverride : Tags.Cue_Grenade_Bounce;
	if (!CueTag.IsValid())
	{
		return;
	}

	FGameplayCueParameters Params;
	Params.Instigator = Thrower;
	Params.EffectCauser = this;
	Params.Location = Hit.ImpactPoint;
	Params.Normal = Hit.ImpactNormal;
	Params.PhysicalMaterial = Hit.PhysMaterial;
	Params.RawMagnitude = 1.0f;

	if (UGameplayCueManager* CueMgr = UAbilitySystemGlobals::Get().GetGameplayCueManager())
	{
		CueMgr->HandleGameplayCue(this, CueTag, EGameplayCueEvent::Executed, Params);
	}
}

void ASFGrenadeBase::DispatchExplodeCue(const FVector& Location)
{
	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
	const FGameplayTag CueTag = ExplosionCueOverride.IsValid() ? ExplosionCueOverride : Tags.Cue_Grenade_Explode;
	if (!CueTag.IsValid())
	{
		return;
	}

	FGameplayCueParameters Params;
	Params.Instigator = Thrower;
	Params.EffectCauser = this;
	Params.Location = Location;
	Params.RawMagnitude = BaseDamage;

	if (UGameplayCueManager* CueMgr = UAbilitySystemGlobals::Get().GetGameplayCueManager())
	{
		CueMgr->HandleGameplayCue(this, CueTag, EGameplayCueEvent::Executed, Params);
	}
}
