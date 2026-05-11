#include "AbilitySystem/SFGameplayAbility_ThrowGrenade.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Characters/SFCharacterBase.h"
#include "Combat/SFGrenadeBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/SignalForgeGameplayTags.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
	constexpr float ArcPreviewIntervalSeconds = 0.05f;
}

USFGameplayAbility_ThrowGrenade::USFGameplayAbility_ThrowGrenade()
{
	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();

	InputTag = Tags.Input_Grenade;
	ActivationPolicy = ESFAbilityActivationPolicy::WhileInputActive;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(Tags.Ability_Grenade_Throw);
	SetAssetTags(AssetTags);

	FGameplayTagContainer GrantedTags;
	GrantedTags.AddTag(Tags.State_Grenade_Cooking);
	ActivationOwnedTags = GrantedTags;

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void USFGameplayAbility_ThrowGrenade::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CachedHandle = Handle;
	CachedActorInfo = ActorInfo;
	CachedActivationInfo = ActivationInfo;
	bHasThrown = false;
	bIsCooking = false;

	if (!GrenadeClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ASFCharacterBase* Character = ActorInfo && ActorInfo->AvatarActor.IsValid()
		? Cast<ASFCharacterBase>(ActorInfo->AvatarActor.Get())
		: nullptr;

	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (ThrowMontage)
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			if (UAnimInstance* Anim = Mesh->GetAnimInstance())
			{
				Anim->Montage_Play(ThrowMontage, 1.0f);
			}
		}
	}

	if (!bAllowCooking)
	{
		// Tap-to-throw: fire immediately at min speed and end.
		ThrowGrenade(false);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// Cook-in-hand path.
	bIsCooking = true;
	if (UWorld* World = GetWorld())
	{
		CookStartTime = World->GetTimeSeconds();

		World->GetTimerManager().SetTimer(
			ArcPreviewTimerHandle,
			this,
			&USFGameplayAbility_ThrowGrenade::TickArcPreview,
			ArcPreviewIntervalSeconds,
			true);

		World->GetTimerManager().SetTimer(
			CookOverflowTimerHandle,
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				// Held too long — explode in hand and end.
				ThrowGrenade(true);
				if (IsActive())
				{
					EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, true);
				}
			}),
			MaxCookSeconds,
			false);
	}

	BP_OnCookStart();
}

void USFGameplayAbility_ThrowGrenade::InputReleased(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);

	if (!bAllowCooking || bHasThrown)
	{
		return;
	}

	ThrowGrenade(false);

	if (IsActive())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void USFGameplayAbility_ThrowGrenade::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ArcPreviewTimerHandle);
		World->GetTimerManager().ClearTimer(CookOverflowTimerHandle);
	}

	bIsCooking = false;
	CachedHandle = FGameplayAbilitySpecHandle();
	CachedActorInfo = nullptr;
	CachedActivationInfo = FGameplayAbilityActivationInfo();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USFGameplayAbility_ThrowGrenade::TickArcPreview()
{
	if (!bIsCooking || !CachedActorInfo || !CachedActorInfo->AvatarActor.IsValid())
	{
		return;
	}

	ASFCharacterBase* Character = Cast<ASFCharacterBase>(CachedActorInfo->AvatarActor.Get());
	if (!Character)
	{
		return;
	}

	const float Alpha = ComputeCookAlpha();
	const FVector Start = ComputeSpawnLocation(Character);
	const FVector Velocity = ComputeLaunchVelocity(Character, Alpha);
	BP_UpdateArcPreview(Start, Velocity, Alpha);
}

void USFGameplayAbility_ThrowGrenade::ThrowGrenade(bool bExplodeInHand)
{
	if (bHasThrown)
	{
		return;
	}
	bHasThrown = true;

	if (!CachedActorInfo || !CachedActorInfo->AvatarActor.IsValid() || !GrenadeClass)
	{
		return;
	}

	ASFCharacterBase* Character = Cast<ASFCharacterBase>(CachedActorInfo->AvatarActor.Get());
	UWorld* World = Character ? Character->GetWorld() : nullptr;
	if (!Character || !World)
	{
		return;
	}

	const float Alpha = ComputeCookAlpha();
	const FVector SpawnLoc = ComputeSpawnLocation(Character);
	const FVector LaunchVelocity = ComputeLaunchVelocity(Character, Alpha);
	const float CookSeconds = Alpha * MaxCookSeconds;

	FActorSpawnParameters Params;
	Params.Owner = Character;
	Params.Instigator = Character;
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ASFGrenadeBase* Grenade = World->SpawnActor<ASFGrenadeBase>(
		GrenadeClass, SpawnLoc, Character->GetActorRotation(), Params);

	if (!Grenade)
	{
		return;
	}

	Grenade->SetThrower(Character);

	if (bExplodeInHand)
	{
		// Cooked too long — burn the entire fuse, regardless of value, then let BeginPlay detonate.
		Grenade->SetCookedTime(1e6f);
		Grenade->LaunchGrenade(FVector::ZeroVector);
	}
	else
	{
		Grenade->SetCookedTime(CookSeconds);
		Grenade->LaunchGrenade(LaunchVelocity);
	}

	BP_OnGrenadeThrown(Grenade, CookSeconds);
}

float USFGameplayAbility_ThrowGrenade::ComputeCookAlpha() const
{
	if (!bAllowCooking || MaxCookSeconds <= 0.0f)
	{
		return 1.0f;
	}

	if (CachedActorInfo && CachedActorInfo->AvatarActor.IsValid())
	{
		if (UWorld* World = CachedActorInfo->AvatarActor->GetWorld())
		{
			const float Elapsed = static_cast<float>(World->GetTimeSeconds() - CookStartTime);
			return FMath::Clamp(Elapsed / MaxCookSeconds, 0.0f, 1.0f);
		}
	}

	return 0.0f;
}

FVector USFGameplayAbility_ThrowGrenade::ComputeLaunchVelocity(ASFCharacterBase* Character, float CookAlpha) const
{
	if (!Character)
	{
		return FVector::ZeroVector;
	}

	FVector EyeLocation;
	FRotator EyeRotation;
	Character->GetActorEyesViewPoint(EyeLocation, EyeRotation);

	// Bias the aim vector upward for a natural arc.
	FRotator AdjustedRotation = EyeRotation;
	AdjustedRotation.Pitch += UpwardThrowAngleDegrees;

	const float Speed = FMath::Lerp(MinThrowSpeed, MaxThrowSpeed, FMath::Clamp(CookAlpha, 0.0f, 1.0f));
	return AdjustedRotation.Vector() * Speed;
}

FVector USFGameplayAbility_ThrowGrenade::ComputeSpawnLocation(ASFCharacterBase* Character) const
{
	if (!Character)
	{
		return FVector::ZeroVector;
	}

	if (USkeletalMeshComponent* Mesh = Character->GetMesh())
	{
		if (GrenadeSpawnSocket != NAME_None && Mesh->DoesSocketExist(GrenadeSpawnSocket))
		{
			return Mesh->GetSocketLocation(GrenadeSpawnSocket);
		}
	}

	// Fallback: a little in front of the eyes so it doesn't clip the head.
	FVector EyeLocation;
	FRotator EyeRotation;
	Character->GetActorEyesViewPoint(EyeLocation, EyeRotation);
	return EyeLocation + EyeRotation.Vector() * 40.0f;
}
