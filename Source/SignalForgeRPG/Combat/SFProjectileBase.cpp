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
#include "AbilitySystem/SFGameplayAbility.h"
#include "Combat/SFCombatantInterface.h"
#include "Core/SFAttributeSetBase.h"
#include "Core/SignalForgeGameplayTags.h"
#include "Engine/OverlapResult.h"
#include "Faction/SFFactionStatics.h"
#include "TimerManager.h"

#if !UE_BUILD_SHIPPING
#include "DrawDebugHelpers.h"
#include "Core/SignalForgeLogChannels.h"

// Runtime-toggleable projectile diagnostics. Defaults to 0 so live play is clean; flip to 1 via
// the console (`SF.Projectile.DrawDebug 1`) to re-enable the cyan trajectory line, red impact
// spheres, and BeginPlay/OnHit verbose logging without rebuilding. Stripped entirely in shipping.
static TAutoConsoleVariable<int32> CVarSFProjectileDrawDebug(
	TEXT("SF.Projectile.DrawDebug"),
	0,
	TEXT("Draw projectile trajectory lines + impact spheres and emit verbose projectile logs. 0 = off (default), 1 = on."),
	ECVF_Cheat);
#endif

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

	// Safety: if SetSourceActor() wasn't called before our first physics tick (e.g. the spawn
	// resolved a hit synchronously), fall back to the spawning Owner so the faction gate and
	// damage path still have a valid attacker reference.
	if (!SourceActor)
	{
		if (AActor* MyOwner = GetOwner())
		{
			SourceActor = MyOwner;
		}
		else if (AActor* MyInstigator = GetInstigator())
		{
			SourceActor = MyInstigator;
		}
	}

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

#if !UE_BUILD_SHIPPING
	// --- DIAGNOSTIC: dump collision/movement state so we can see why hits aren't firing.
	if (CollisionSphere && ProjectileMovement)
	{
		const FName ProfileName = CollisionSphere->GetCollisionProfileName();
		const ECollisionEnabled::Type CE = CollisionSphere->GetCollisionEnabled();
		const bool bNotifyHit = CollisionSphere->GetBodyInstance() ? CollisionSphere->GetBodyInstance()->bNotifyRigidBodyCollision : false;
		const FVector Vel = ProjectileMovement->Velocity;
		const bool bDrawDebug = CVarSFProjectileDrawDebug.GetValueOnGameThread() > 0;
		UE_LOG(LogSFCombat, Verbose,
			TEXT("SFProjectile::BeginPlay -> %s profile=%s collision=%d notifyHit=%d radius=%.1f initSpeed=%.1f velocity=%s |V|=%.1f source=%s owner=%s"),
			*GetName(),
			*ProfileName.ToString(),
			(int32)CE,
			bNotifyHit ? 1 : 0,
			CollisionSphere->GetUnscaledSphereRadius(),
			ProjectileMovement->InitialSpeed,
			*Vel.ToCompactString(),
			Vel.Size(),
			SourceActor ? *SourceActor->GetName() : TEXT("NULL"),
			GetOwner() ? *GetOwner()->GetName() : TEXT("NULL"));

		// Persistent trajectory line (5s) so user can see where projectile is actually going.
		if (bDrawDebug)
		{
			const FVector StartLoc = GetActorLocation();
			const FVector EndLoc = StartLoc + GetActorForwardVector() * 5000.0f;
			DrawDebugLine(GetWorld(), StartLoc, EndLoc, FColor::Cyan, false, 5.0f, 0, 1.5f);
			DrawDebugSphere(GetWorld(), StartLoc, 12.0f, 8, FColor::Cyan, false, 5.0f, 0, 1.5f);
		}
	}

	// Delayed sanity check: if projectile is still alive after 0.1s, log its position+velocity.
	if (UWorld* W = GetWorld())
	{
		FTimerHandle TmpHandle;
		TWeakObjectPtr<ASFProjectileBase> WeakThis(this);
		W->GetTimerManager().SetTimer(TmpHandle, [WeakThis]()
		{
			if (ASFProjectileBase* Self = WeakThis.Get())
			{
				const FVector Loc = Self->GetActorLocation();
				const FVector Vel = Self->ProjectileMovement ? Self->ProjectileMovement->Velocity : FVector::ZeroVector;
				UE_LOG(LogSFCombat, Verbose,
					TEXT("SFProjectile::AliveAt+0.1s -> %s loc=%s |V|=%.1f vel=%s"),
					*Self->GetName(), *Loc.ToCompactString(), Vel.Size(), *Vel.ToCompactString());
			}
		}, 0.1f, false);
	}
#endif
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
#if !UE_BUILD_SHIPPING
	UE_LOG(LogSFCombat, Verbose,
		TEXT("SFProjectile::OnHit -> %s OtherActor=%s OtherComp=%s ImpactPoint=%s source=%s owner=%s damageEffectClass=%s baseDamage=%.1f"),
		*GetName(),
		OtherActor ? *OtherActor->GetName() : TEXT("NULL"),
		OtherComp ? *OtherComp->GetName() : TEXT("NULL"),
		*Hit.ImpactPoint.ToCompactString(),
		SourceActor ? *SourceActor->GetName() : TEXT("NULL"),
		GetOwner() ? *GetOwner()->GetName() : TEXT("NULL"),
		DamageEffectClass ? *DamageEffectClass->GetName() : TEXT("NULL"),
		BaseDamage);

	if (CVarSFProjectileDrawDebug.GetValueOnGameThread() > 0)
	{
		DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 16.0f, 8, FColor::Red, false, 3.0f, 0, 1.5f);
	}
#endif

	// Filter self/source hits so a misaligned muzzle doesn't self-destruct the bullet.
	if (!OtherActor || OtherActor == this || OtherActor == SourceActor || OtherActor == GetOwner())
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogSFCombat, Verbose,
			TEXT("SFProjectile::OnHit -> SELF-FILTER skipped this hit (other=%s)"),
			OtherActor ? *OtherActor->GetName() : TEXT("NULL"));
#endif
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

	// Friend-foe gate: if the projectile hit a non-hostile character, suppress
	// damage but still play the impact cue (so allies still see the shot land
	// in world without being harmed). The projectile will still detonate per
	// its normal lifecycle (radial impact + cleanup are handled by the caller).
	bool bSuppressDamage = false;
	if (OtherActor && OtherActor->Implements<USFCombatantInterface>())
	{
		if (ISFCombatantInterface::Execute_IsDead(OtherActor))
		{
			bSuppressDamage = true;
#if !UE_BUILD_SHIPPING
			UE_LOG(LogSFCombat, Warning, TEXT("SFProjectile::HandleImpact -> SUPPRESS (target dead): %s"), *OtherActor->GetName());
#endif
		}
		else if (!bAllowFriendlyFire && !USFFactionStatics::AreHostile(SourceActor, OtherActor))
		{
			bSuppressDamage = true;
#if !UE_BUILD_SHIPPING
			UE_LOG(LogSFCombat, Warning,
				TEXT("SFProjectile::HandleImpact -> SUPPRESS (faction gate: not hostile). source=%s target=%s allowFF=%d"),
				SourceActor ? *SourceActor->GetName() : TEXT("NULL"),
				*OtherActor->GetName(),
				bAllowFriendlyFire ? 1 : 0);
#endif
		}
	}

#if !UE_BUILD_SHIPPING
	UE_LOG(LogSFCombat, Warning,
		TEXT("SFProjectile::HandleImpact -> entering damage block: DEC=%s source=%s target=%s suppress=%d"),
		DamageEffectClass ? *DamageEffectClass->GetName() : TEXT("NULL"),
		SourceActor ? *SourceActor->GetName() : TEXT("NULL"),
		OtherActor ? *OtherActor->GetName() : TEXT("NULL"),
		bSuppressDamage ? 1 : 0);
#endif

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

	// Damage attribution -- gated by friend-foe.
	if (DamageEffectClass && SourceActor && OtherActor && !bSuppressDamage)
	{
		UAbilitySystemComponent* SourceASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceActor);

		UAbilitySystemComponent* TargetASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);

		// Snapshot shields BEFORE damage so we can fire shield-impact/break cues
		// from the projectile hit site once damage resolves.
		const float ShieldsBefore = TargetASC
			? TargetASC->GetNumericAttribute(USFAttributeSetBase::GetShieldsAttribute())
			: 0.0f;

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

				// Attribute the kill to the shooter on ANY combatant we damage.
				// Faction hostility was already verified above, so this is the
				// right place to record attribution.
				if (OtherActor && OtherActor->Implements<USFCombatantInterface>())
				{
					ISFCombatantInterface::Execute_RegisterDamageInstigator(OtherActor, SourceActor);
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

			// Shield impact / break VFX share the same hit context. Helper no-ops if
			// shields did not move (no shields on target, or hit went straight to health).
			USFGameplayAbility::DispatchShieldHitCues(TargetASC, ShieldsBefore, CueParams);
		}
		else if (UGameplayCueManager* CueMgr = UAbilitySystemGlobals::Get().GetGameplayCueManager())
		{
			CueMgr->HandleGameplayCue(this, ImpactCueTag, EGameplayCueEvent::Executed, CueParams);
		}
	}

	// Track last-hit enemy on the source player controller for camera lock-on flow.
	// Only lock onto hostile targets so friendly fire doesn't hijack the camera.
	if (ASFCharacterBase* SourceCharacter = Cast<ASFCharacterBase>(SourceActor))
	{
		if (ASFPlayerController* PC = Cast<ASFPlayerController>(SourceCharacter->GetController()))
		{
			if (ASFCharacterBase* HitCharacter = Cast<ASFCharacterBase>(OtherActor))
			{
				if (USFFactionStatics::AreHostile(SourceCharacter, HitCharacter))
				{
					PC->SetTrackedEnemy(HitCharacter);
				}
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
