#include "AbilitySystem/SFGameplayAbility_Reload.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Characters/SFCharacterBase.h"
#include "Combat/SFAmmoType.h"
#include "Combat/SFWeaponData.h"
#include "Combat/SFWeaponInstanceTypes.h"
#include "Components/SFAmmoReserveComponent.h"
#include "Components/SFEquipmentComponent.h"
#include "Core/SignalForgeGameplayTags.h"
#include "Engine/World.h"
#include "TimerManager.h"

USFGameplayAbility_Reload::USFGameplayAbility_Reload()
{
	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();

	InputTag = Tags.Input_Reload;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(Tags.Ability_Weapon_Reload);
	SetAssetTags(AssetTags);

	// While reloading, suppress firing. Also block reload itself while a weapon swap is in flight
	// or a melee swing is mid-animation -- trying to slot a magazine while swinging a sword would
	// double-bind the right hand.
	FGameplayTagContainer BlockedTags;
	BlockedTags.AddTag(Tags.Ability_Weapon_PrimaryFire);
	BlockedTags.AddTag(Tags.Ability_Weapon_SecondaryFire);
	BlockedTags.AddTag(Tags.Ability_Weapon_MeleeLight);
	BlockedTags.AddTag(Tags.Ability_Weapon_MeleeHeavy);
	BlockedTags.AddTag(Tags.State_Weapon_Switching);
	BlockedTags.AddTag(Tags.State_Weapon_MeleeSwinging);
	ActivationBlockedTags = BlockedTags;

	FGameplayTagContainer GrantedTags;
	GrantedTags.AddTag(Tags.State_Weapon_Reloading);
	ActivationOwnedTags = GrantedTags;

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

bool USFGameplayAbility_Reload::CanActivateAbility(
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

	USFEquipmentComponent* Equipment = Character->GetEquipmentComponent();
	if (!Equipment)
	{
		return false;
	}

	USFWeaponData* WeaponData = Equipment->GetCurrentWeaponData();
	if (!WeaponData || WeaponData->AmmoConfig.ClipSize <= 0)
	{
		return false;
	}

	// Don't reload if already full.
	const FSFWeaponInstanceData Instance = Equipment->GetCurrentWeaponInstance();
	if (Instance.AmmoInClip >= WeaponData->AmmoConfig.ClipSize)
	{
		return false;
	}

	// If this weapon uses a pooled reserve, gate on reserve having anything left.
	// (A weapon with no AmmoType uses magic refill and can always reload.)
	if (USFAmmoType* AmmoType = WeaponData->AmmoConfig.AmmoType)
	{
		if (USFAmmoReserveComponent* Reserve = Character->GetAmmoReserveComponent())
		{
			return Reserve->GetAmmoCount(AmmoType) > 0;
		}
	}

	return true;
}

void USFGameplayAbility_Reload::ActivateAbility(
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

	USFEquipmentComponent* Equipment = nullptr;
	USFWeaponData* WeaponData = nullptr;
	if (!ResolveContext(Equipment, WeaponData))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
	if (ActorInfo->AbilitySystemComponent.IsValid() && Tags.State_Weapon_Reloading.IsValid())
	{
		ActorInfo->AbilitySystemComponent->AddLooseGameplayTag(Tags.State_Weapon_Reloading, 1);
	}

	// Optional cue (reload sound / animation feedback).
	if (Tags.Cue_Weapon_Reload.IsValid() && ActorInfo->AbilitySystemComponent.IsValid())
	{
		FGameplayCueParameters Params;
		Params.Instigator = ActorInfo->AvatarActor.Get();
		Params.Location = ActorInfo->AvatarActor.IsValid() ? ActorInfo->AvatarActor->GetActorLocation() : FVector::ZeroVector;
		ActorInfo->AbilitySystemComponent->ExecuteGameplayCue(Tags.Cue_Weapon_Reload, Params);
	}

	UAnimMontage* ReloadMontage = WeaponData->RangedConfig.ReloadMontage;

	if (ReloadMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this, NAME_None, ReloadMontage);

		if (MontageTask)
		{
			MontageTask->OnCompleted.AddDynamic(this, &USFGameplayAbility_Reload::OnReloadMontageCompleted);
			MontageTask->OnBlendOut.AddDynamic(this, &USFGameplayAbility_Reload::OnReloadMontageCompleted);
			MontageTask->OnInterrupted.AddDynamic(this, &USFGameplayAbility_Reload::OnReloadMontageInterrupted);
			MontageTask->OnCancelled.AddDynamic(this, &USFGameplayAbility_Reload::OnReloadMontageInterrupted);
			MontageTask->ReadyForActivation();
			return;
		}
	}

	// Fallback: pure timer-driven reload.
	if (UWorld* World = GetWorld())
	{
		const float ReloadSeconds = FMath::Max(0.2f, WeaponData->RangedConfig.ReloadSeconds);
		World->GetTimerManager().SetTimer(
			ReloadTimerHandle,
			FTimerDelegate::CreateWeakLambda(this, [this]() { FinishReload(true); }),
			ReloadSeconds,
			false);
	}
	else
	{
		FinishReload(true);
	}
}

void USFGameplayAbility_Reload::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReloadTimerHandle);
	}

	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
		if (Tags.State_Weapon_Reloading.IsValid())
		{
			ActorInfo->AbilitySystemComponent->RemoveLooseGameplayTag(Tags.State_Weapon_Reloading);
		}
	}

	CachedHandle = FGameplayAbilitySpecHandle();
	CachedActorInfo = nullptr;
	CachedActivationInfo = FGameplayAbilityActivationInfo();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USFGameplayAbility_Reload::OnReloadMontageCompleted()
{
	FinishReload(true);
}

void USFGameplayAbility_Reload::OnReloadMontageInterrupted()
{
	FinishReload(false);
}

void USFGameplayAbility_Reload::FinishReload(bool bRefillClip)
{
	if (bRefillClip)
	{
		USFEquipmentComponent* Equipment = nullptr;
		USFWeaponData* WeaponData = nullptr;
		if (ResolveContext(Equipment, WeaponData) && WeaponData->AmmoConfig.ClipSize > 0)
		{
			FSFWeaponInstanceData CurrentInstance = Equipment->GetCurrentWeaponInstance();
			const int32 ClipSize = WeaponData->AmmoConfig.ClipSize;
			const int32 NeededRounds = ClipSize - CurrentInstance.AmmoInClip;

			if (NeededRounds > 0)
			{
				USFAmmoType* AmmoType = WeaponData->AmmoConfig.AmmoType;
				if (AmmoType)
				{
					// Pull rounds from the carrier's reserve pool. If the reserve is empty,
					// the clip ends up with whatever was already loaded (zero refill).
					if (ASFCharacterBase* OwningCharacter = Cast<ASFCharacterBase>(CachedActorInfo->AvatarActor.Get()))
					{
						if (USFAmmoReserveComponent* Reserve = OwningCharacter->GetAmmoReserveComponent())
						{
							const int32 Drawn = Reserve->ConsumeAmmo(AmmoType, NeededRounds);
							CurrentInstance.AmmoInClip += Drawn;
						}
						else
						{
							CurrentInstance.AmmoInClip = ClipSize; // No reserve component — fall back to full refill.
						}
					}
				}
				else
				{
					// Weapon doesn't draw from a reserve (e.g. energy weapon) — free refill.
					CurrentInstance.AmmoInClip = ClipSize;
				}

				Equipment->UpdateActiveWeaponInstance(CurrentInstance);
			}
		}
	}

	if (IsActive())
	{
		EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, !bRefillClip);
	}
}

bool USFGameplayAbility_Reload::ResolveContext(
	USFEquipmentComponent*& OutEquipment,
	USFWeaponData*& OutWeaponData) const
{
	OutEquipment = nullptr;
	OutWeaponData = nullptr;

	if (!CachedActorInfo || !CachedActorInfo->AvatarActor.IsValid())
	{
		return false;
	}

	ASFCharacterBase* Character = Cast<ASFCharacterBase>(CachedActorInfo->AvatarActor.Get());
	if (!Character)
	{
		return false;
	}

	OutEquipment = Character->GetEquipmentComponent();
	if (!OutEquipment)
	{
		return false;
	}

	OutWeaponData = OutEquipment->GetCurrentWeaponData();
	return OutWeaponData != nullptr;
}
