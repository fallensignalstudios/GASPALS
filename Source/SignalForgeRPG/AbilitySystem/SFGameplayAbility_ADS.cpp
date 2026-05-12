#include "AbilitySystem/SFGameplayAbility_ADS.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Characters/SFCharacterBase.h"
#include "Combat/SFWeaponData.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SFEquipmentComponent.h"
#include "Core/SignalForgeGameplayTags.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "TimerManager.h"
#include "GameplayEffect.h"

USFGameplayAbility_ADS::USFGameplayAbility_ADS()
{
	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();

	InputTag = Tags.Input_ADS;
	ActivationPolicy = ESFAbilityActivationPolicy::WhileInputActive;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(Tags.Ability_Weapon_ADS);
	SetAssetTags(AssetTags);

	// While the ability is active, owner is treated as ADS by everyone (WeaponFire reads this).
	ActivationOwnedTags.AddTag(Tags.State_Weapon_ADS);

	// Don't ADS while reloading or while a weapon swap is in flight.
	ActivationBlockedTags.AddTag(Tags.State_Weapon_Reloading);
	ActivationBlockedTags.AddTag(Tags.State_Weapon_Switching);
	// Can't aim down sights while a melee swing is in flight.
	ActivationBlockedTags.AddTag(Tags.State_Weapon_MeleeSwinging);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

bool USFGameplayAbility_ADS::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		return false;
	}

	ASFCharacterBase* Character = Cast<ASFCharacterBase>(ActorInfo->AvatarActor.Get());
	if (!Character)
	{
		return false;
	}

	// Must have a ranged weapon equipped — pure melee swords have no business being ADSed.
	USFEquipmentComponent* Equipment = Character->GetEquipmentComponent();
	if (!Equipment || !Equipment->HasEquippedWeapon())
	{
		return false;
	}

	USFWeaponData* WeaponData = Equipment->GetCurrentWeaponData();
	if (!WeaponData)
	{
		return false;
	}

	// Weapons opt out of ADS by leaving AdsCameraFOV at 0 AND AdsSpringArmLength at 0.
	const FSFRangedWeaponConfig& Cfg = WeaponData->RangedConfig;
	if (Cfg.AdsCameraFOV <= 0.0f && Cfg.AdsSpringArmLength <= 0.0f)
	{
		return false;
	}

	return true;
}

void USFGameplayAbility_ADS::ActivateAbility(
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
	bIsBlendingOut = false;

	ASFCharacterBase* Character = ActorInfo && ActorInfo->AvatarActor.IsValid()
		? Cast<ASFCharacterBase>(ActorInfo->AvatarActor.Get())
		: nullptr;

	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Capture baseline so EndAbility can restore exactly.
	CaptureBaselineState(Character);

	// Pull target values from the equipped weapon's ranged config.
	if (USFEquipmentComponent* Equipment = Character->GetEquipmentComponent())
	{
		if (USFWeaponData* WeaponData = Equipment->GetCurrentWeaponData())
		{
			const FSFRangedWeaponConfig& Cfg = WeaponData->RangedConfig;

			TargetCameraFOV = Cfg.AdsCameraFOV > 0.0f ? Cfg.AdsCameraFOV : BaselineCameraFOV;
			TargetArmLength = Cfg.AdsSpringArmLength > 0.0f ? Cfg.AdsSpringArmLength : BaselineArmLength;
			TargetSocketOffset = Cfg.AdsSpringArmSocketOffset;
			TargetMaxWalkSpeed = BaselineMaxWalkSpeed * FMath::Max(0.1f, Cfg.AdsMovementSpeedMultiplier);

			BlendInSeconds = FMath::Max(0.01f, Cfg.AdsBlendInSeconds);
			BlendOutSeconds = FMath::Max(0.01f, Cfg.AdsBlendOutSeconds);

			if (Cfg.AdsPoseMontage)
			{
				if (USkeletalMeshComponent* Mesh = Character->GetMesh())
				{
					if (UAnimInstance* Anim = Mesh->GetAnimInstance())
					{
						Anim->Montage_Play(Cfg.AdsPoseMontage, 1.0f);
					}
				}
			}
		}
	}

	// Apply walk-speed slow immediately; the camera FOV / arm interp via timer below.
	if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = TargetMaxWalkSpeed;
	}

	// Optional attribute effect (e.g. +Accuracy, -SwayAmplitude).
	if (AdsAttributeEffectClass && ActorInfo->AbilitySystemComponent.IsValid())
	{
		const FGameplayEffectSpecHandle SpecHandle =
			MakeOutgoingGameplayEffectSpec(AdsAttributeEffectClass, GetAbilityLevel());
		if (SpecHandle.IsValid())
		{
			AdsEffectHandle = ApplyGameplayEffectSpecToOwner(
				Handle, ActorInfo, ActivationInfo, SpecHandle);
		}
	}

	// Fire the enter cue once.
	if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
		if (Tags.Cue_Weapon_ADS_Enter.IsValid())
		{
			FGameplayCueParameters Params;
			Params.Instigator = Character;
			Params.EffectCauser = Character;
			Params.Location = Character->GetActorLocation();
			ASC->ExecuteGameplayCue(Tags.Cue_Weapon_ADS_Enter, Params);
		}
	}

	// Start the interpolation timer that drives the visible blend.
	if (UWorld* World = Character->GetWorld())
	{
		World->GetTimerManager().SetTimer(
			InterpTimerHandle,
			this,
			&USFGameplayAbility_ADS::TickAdsBlend,
			InterpTickInterval,
			true);
	}

	BP_OnAdsEntered();
}

void USFGameplayAbility_ADS::InputReleased(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);
	BeginAdsExit();
}

void USFGameplayAbility_ADS::BeginAdsExit()
{
	if (!IsActive() || bIsBlendingOut)
	{
		return;
	}

	bIsBlendingOut = true;
	BP_OnAdsExited();

	if (CachedActorInfo && CachedActorInfo->AbilitySystemComponent.IsValid())
	{
		UAbilitySystemComponent* ASC = CachedActorInfo->AbilitySystemComponent.Get();
		const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
		if (Tags.Cue_Weapon_ADS_Exit.IsValid())
		{
			FGameplayCueParameters Params;
			Params.Instigator = CachedActorInfo->AvatarActor.Get();
			Params.EffectCauser = CachedActorInfo->AvatarActor.Get();
			if (CachedActorInfo->AvatarActor.IsValid())
			{
				Params.Location = CachedActorInfo->AvatarActor->GetActorLocation();
			}
			ASC->ExecuteGameplayCue(Tags.Cue_Weapon_ADS_Exit, Params);
		}
	}

	// EndAbility will restore baseline. The interp timer is cleared in EndAbility.
	EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, false);
}

void USFGameplayAbility_ADS::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// Clear the interp timer.
	ASFCharacterBase* Character = ActorInfo && ActorInfo->AvatarActor.IsValid()
		? Cast<ASFCharacterBase>(ActorInfo->AvatarActor.Get())
		: nullptr;

	if (Character)
	{
		if (UWorld* World = Character->GetWorld())
		{
			World->GetTimerManager().ClearTimer(InterpTimerHandle);
		}
	}

	// Restore camera + spring arm + walk speed if we ever cached a baseline.
	if (bCachedBaseline && Character)
	{
		UCameraComponent* Camera = nullptr;
		USpringArmComponent* Boom = nullptr;
		ResolveViewComponents(Character, Camera, Boom);

		if (Camera && Camera->FieldOfView > 0.0f)
		{
			Camera->SetFieldOfView(BaselineCameraFOV);
		}

		if (Boom)
		{
			Boom->TargetArmLength = BaselineArmLength;
			Boom->SocketOffset = BaselineSocketOffset;
		}

		if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
		{
			MoveComp->MaxWalkSpeed = BaselineMaxWalkSpeed;
		}
	}

	// Remove ADS attribute effect.
	if (AdsEffectHandle.IsValid() && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(AdsEffectHandle);
		AdsEffectHandle.Invalidate();
	}

	bCachedBaseline = false;
	bIsBlendingOut = false;
	CachedActorInfo = nullptr;
	CachedHandle = FGameplayAbilitySpecHandle();
	CachedActivationInfo = FGameplayAbilityActivationInfo();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USFGameplayAbility_ADS::TickAdsBlend()
{
	if (!CachedActorInfo || !CachedActorInfo->AvatarActor.IsValid())
	{
		return;
	}

	ASFCharacterBase* Character = Cast<ASFCharacterBase>(CachedActorInfo->AvatarActor.Get());
	if (!Character)
	{
		return;
	}

	UCameraComponent* Camera = nullptr;
	USpringArmComponent* Boom = nullptr;
	ResolveViewComponents(Character, Camera, Boom);

	// Blend-in time (we never actually run the blend-out branch here; EndAbility snaps the baseline
	// back instantly when input is released, but Camera-side springs / curves can still smooth via
	// UE's existing post-process or BP if desired).
	const float Speed = 1.0f / FMath::Max(0.01f, BlendInSeconds);

	if (Camera && TargetCameraFOV > 0.0f)
	{
		const float NewFOV = FMath::FInterpTo(
			Camera->FieldOfView,
			TargetCameraFOV,
			InterpTickInterval,
			Speed);
		Camera->SetFieldOfView(NewFOV);
	}

	if (Boom)
	{
		Boom->TargetArmLength = FMath::FInterpTo(
			Boom->TargetArmLength,
			TargetArmLength,
			InterpTickInterval,
			Speed);

		Boom->SocketOffset = FMath::VInterpTo(
			Boom->SocketOffset,
			TargetSocketOffset,
			InterpTickInterval,
			Speed);
	}
}

void USFGameplayAbility_ADS::CaptureBaselineState(ASFCharacterBase* Character)
{
	if (!Character)
	{
		return;
	}

	UCameraComponent* Camera = nullptr;
	USpringArmComponent* Boom = nullptr;
	ResolveViewComponents(Character, Camera, Boom);

	BaselineCameraFOV = Camera ? Camera->FieldOfView : 90.0f;
	BaselineArmLength = Boom ? Boom->TargetArmLength : 350.0f;
	BaselineSocketOffset = Boom ? Boom->SocketOffset : FVector::ZeroVector;

	BaselineMaxWalkSpeed = Character->GetCharacterMovement()
		? Character->GetCharacterMovement()->MaxWalkSpeed
		: Character->WalkSpeed;

	bCachedBaseline = true;
}

void USFGameplayAbility_ADS::ResolveViewComponents(
	ASFCharacterBase* Character,
	UCameraComponent*& OutCamera,
	USpringArmComponent*& OutBoom) const
{
	OutCamera = nullptr;
	OutBoom = nullptr;

	if (!Character)
	{
		return;
	}

	// Prefer a camera named "GameplayCamera" (matches ASFPlayerCharacter::GetGameplayCamera),
	// otherwise fall back to the first non-dialogue camera found on the actor.
	TArray<UCameraComponent*> Cameras;
	Character->GetComponents<UCameraComponent>(Cameras);
	for (UCameraComponent* Cam : Cameras)
	{
		if (!Cam)
		{
			continue;
		}
		if (Cam->GetName() == TEXT("GameplayCamera"))
		{
			OutCamera = Cam;
			break;
		}
	}
	if (!OutCamera)
	{
		for (UCameraComponent* Cam : Cameras)
		{
			if (Cam && Cam->GetName() != TEXT("DialogueCamera"))
			{
				OutCamera = Cam;
				break;
			}
		}
	}

	// Spring arm: take the first one. The player character has exactly one ("GameplayCameraBoom").
	TArray<USpringArmComponent*> Booms;
	Character->GetComponents<USpringArmComponent>(Booms);
	if (Booms.Num() > 0)
	{
		OutBoom = Booms[0];
	}
}
