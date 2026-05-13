#include "Combat/SFProjectileBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "GameplayCueManager.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Input/SFPlayerController.h"
#include "Characters/SFCharacterBase.h"
#include "Characters/SFEnemyCharacter.h"
#include "Core/SignalForgeGameplayTags.h"
#include "Engine/OverlapResult.h"

ASFProjectileBase::ASFProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->SetSphereRadius(8.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	CollisionSphere->SetNotifyRigidBodyCollision(true);

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(CollisionSphere);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = InitialSpeed;
	ProjectileMovement->MaxSpeed = MaxSpeed;
	ProjectileMovement->ProjectileGravityScale = GravityScale;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
}

void ASFProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	SpawnLocation = GetActorLocation();
	SetLifeSpan(LifeSeconds);

	// Push designer-tunable values to the movement component at runtime
	// (constructor defaults may be overridden in the asset).
	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = InitialSpeed;
		ProjectileMovement->MaxSpeed = MaxSpeed;
		ProjectileMovement->ProjectileGravityScale = GravityScale;
		ProjectileMovement->bShouldBounce = bRicochet;
		if (bRicochet)
		{
			ProjectileMovement->Bounciness = RicochetSpeedRetention;
		}
	}

	if (CollisionSphere)
	{
		CollisionSphere->OnComponentHit.AddDynamic(this, &ASFProjectileBase::OnProjectileHit);

		// Always ignore our owner/instigator/source so we don't blow up the muzzle of the gun.
		if (SourceActor)
		{
			CollisionSphere->IgnoreActorWhenMoving(SourceActor, true);
		}
		if (AActor* MyOwner = GetOwner())
		{
			CollisionSphere->IgnoreActorWhenMoving(MyOwner, true);
		}
		if (AActor* MyInstigator = GetInstigator())
		{
			CollisionSphere->IgnoreActorWhenMoving(MyInstigator, true);
		}
	}

	if (TrailNiagara)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			TrailNiagara,
			GetRootComponent(),
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true);
	}

	if (LaunchSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, LaunchSound, GetActorLocation());
	}
}

void ASFProjectileBase::SetSpeedMultiplier(float Multiplier)
{
	if (Multiplier <= 0.0f || FMath::IsNearlyEqual(Multiplier, 1.0f))
	{
		return;
	}
	InitialSpeed *= Multiplier;
	if (MaxSpeed > 0.0f && MaxSpeed < InitialSpeed)
	{
		MaxSpeed = InitialSpeed;
	}
}

void ASFProjectileBase::SetSourceActor(AActor* InSourceActor)
{
	SourceActor = InSourceActor;
	if (SourceActor && CollisionSphere)
	{
		CollisionSphere->IgnoreActorWhenMoving(SourceActor, true);
	}
}

void ASFProjectileBase::SetDamageEffect(TSubclassOf<UGameplayEffect> InDamageEffect)
{
	DamageEffectClass = InDamageEffect;
}

void ASFProjectileBase::SetDamageFalloff(float InStart, float InEnd, float InMinMultiplier)
{
	FalloffStartDistance = FMath::Max(0.0f, InStart);
	FalloffEndDistance = FMath::Max(FalloffStartDistance, InEnd);
	MinFalloffMultiplier = FMath::Clamp(InMinMultiplier, 0.0f, 1.0f);
}

void ASFProjectileBase::OnProjectileHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	// Filter self/source hits so a misaligned muzzle doesn't self-destruct the bullet.
	if (!OtherActor || OtherActor == this || OtherActor == SourceActor || OtherActor == GetOwner())
	{
		return;
	}

	if (RadialRadius > 0.0f)
	{
		HandleRadialImpact(Hit);
		Destroy();
		return;
	}

	HandleImpact(OtherActor, Hit);

	if (bRicochet && RicochetCount < MaxRicochets)
	{
		++RicochetCount;
		// Allow the bounce; ProjectileMovement handles the reflect.
		return;
	}

	Destroy();
}

float ASFProjectileBase::ComputeFalloffMultiplier(const FVector& HitLocation) const
{
	if (FalloffEndDistance <= 0.0f || FalloffEndDistance <= FalloffStartDistance)
	{
		return 1.0f;
	}

	const float Distance = FVector::Dist(SpawnLocation, HitLocation);
	if (Distance <= FalloffStartDistance)
	{
		return 1.0f;
	}
	if (Distance >= FalloffEndDistance)
	{
		return MinFalloffMultiplier;
	}

	const float Alpha = (Distance - FalloffStartDistance) / FMath::Max(1.0f, (FalloffEndDistance - FalloffStartDistance));
	return FMath::Lerp(1.0f, MinFalloffMultiplier, Alpha);
}

void ASFProjectileBase::HandleImpact(AActor* OtherActor, const FHitResult& Hit)
{
	const FSignalForgeGameplayTags& SFTags = FSignalForgeGameplayTags::Get();

	// VFX
	if (ImpactNiagara)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ImpactNiagara,
			Hit.ImpactPoint,
			Hit.ImpactNormal.Rotation());
	}

	// SFX
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Hit.ImpactPoint);
	}

	// Damage attribution
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
			EffectContext.AddHitResult(Hit);

			const FGameplayEffectSpecHandle SpecHandle =
				SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, EffectContext);

			if (SpecHandle.IsValid())
			{
				const float FalloffMult = ComputeFalloffMultiplier(Hit.ImpactPoint);
				const float FinalDamage = BaseDamage * FalloffMult;

				if (SFTags.Data_BaseDamage.IsValid())
				{
					SpecHandle.Data->SetSetByCallerMagnitude(SFTags.Data_BaseDamage, FinalDamage);
				}
				if (SFTags.Data_FalloffMultiplier.IsValid())
				{
					SpecHandle.Data->SetSetByCallerMagnitude(SFTags.Data_FalloffMultiplier, FalloffMult);
				}

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

	// GameplayCue (impact). Default to the override if set, otherwise the standard impact cue.
	const FGameplayTag ImpactCueTag = ImpactCueTagOverride.IsValid()
		? ImpactCueTagOverride
		: SFTags.Cue_Hit_Impact;

	if (ImpactCueTag.IsValid())
	{
		FGameplayCueParameters CueParams;
		CueParams.Instigator = SourceActor;
		CueParams.EffectCauser = this;
		CueParams.Location = Hit.ImpactPoint;
		CueParams.Normal = Hit.ImpactNormal;
		CueParams.PhysicalMaterial = Hit.PhysMaterial;
		CueParams.RawMagnitude = BaseDamage;

		if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
		{
			TargetASC->ExecuteGameplayCue(ImpactCueTag, CueParams);
		}
		else if (UGameplayCueManager* CueMgr = UAbilitySystemGlobals::Get().GetGameplayCueManager())
		{
			CueMgr->HandleGameplayCue(this, ImpactCueTag, EGameplayCueEvent::Executed, CueParams);
		}
	}

	// Track last-hit enemy on the source player controller for camera lock-on flow.
	if (ASFCharacterBase* SourceCharacter = Cast<ASFCharacterBase>(SourceActor))
	{
		if (ASFPlayerController* PC = Cast<ASFPlayerController>(SourceCharacter->GetController()))
		{
			if (ASFCharacterBase* HitCharacter = Cast<ASFCharacterBase>(OtherActor))
			{
				PC->SetTrackedEnemy(HitCharacter);
			}
		}
	}
}

void ASFProjectileBase::HandleRadialImpact(const FHitResult& Hit)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Visual + audio first so they fire even if there are no valid targets.
	if (ImpactNiagara)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			ImpactNiagara,
			Hit.ImpactPoint,
			Hit.ImpactNormal.Rotation());
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Hit.ImpactPoint);
	}

	// Sweep for actors inside the blast radius.
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ProjectileRadial), false, this);
	if (SourceActor)
	{
		Params.AddIgnoredActor(SourceActor);
	}
	World->OverlapMultiByObjectType(
		Overlaps,
		Hit.ImpactPoint,
		FQuat::Identity,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllDynamicObjects),
		FCollisionShape::MakeSphere(RadialRadius),
		Params);

	const FSignalForgeGameplayTags& SFTags = FSignalForgeGameplayTags::Get();

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (!Target || Target == this || Target == SourceActor)
		{
			continue;
		}

		const float DistanceToTarget = FVector::Dist(Target->GetActorLocation(), Hit.ImpactPoint);
		const float DistanceAlpha = FMath::Clamp(DistanceToTarget / FMath::Max(1.0f, RadialRadius), 0.0f, 1.0f);
		const float RadialMultiplier = FMath::Pow(1.0f - DistanceAlpha, FMath::Max(0.01f, RadialFalloff));
		const float FinalDamage = BaseDamage * RadialMultiplier;

		if (DamageEffectClass && SourceActor)
		{
			if (UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceActor))
			{
				if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target))
				{
					FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
					EffectContext.AddSourceObject(this);
					EffectContext.AddHitResult(Hit);
					const FGameplayEffectSpecHandle SpecHandle =
						SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, EffectContext);
					if (SpecHandle.IsValid())
					{
						if (SFTags.Data_BaseDamage.IsValid())
						{
							SpecHandle.Data->SetSetByCallerMagnitude(SFTags.Data_BaseDamage, FinalDamage);
						}
						SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
					}
				}
			}
		}
	}
}
