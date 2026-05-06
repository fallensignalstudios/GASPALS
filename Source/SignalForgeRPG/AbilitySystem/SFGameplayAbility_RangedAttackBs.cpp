#include "AbilitySystem/SFGameplayAbility_RangedAttackBs.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Characters/SFCharacterBase.h"
#include "Combat/SFProjectileBase.h"
#include "Combat/SFWeaponActor.h"
#include "Components/SFEquipmentComponent.h"
#include "Core/SignalForgeGameplayTags.h"
#include "Engine/World.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

USFGameplayAbility_RangedAttackBs::USFGameplayAbility_RangedAttackBs()
{
	const FSignalForgeGameplayTags& GameplayTags = FSignalForgeGameplayTags::Get();

	InputTag = GameplayTags.Input_Ability_1;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(GameplayTags.Ability_Attack_Light);
	SetAssetTags(AssetTags);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool USFGameplayAbility_RangedAttackBs::CanActivateAbility(
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

	ASFCharacterBase* Character = nullptr;
	if (!TryGetRangedCharacter(ActorInfo, Character))
	{
		return false;
	}

	return ProjectileClass != nullptr;
}

void USFGameplayAbility_RangedAttackBs::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ASFCharacterBase* Character = nullptr;
	if (!TryGetRangedCharacter(ActorInfo, Character) || !ProjectileClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	USFEquipmentComponent* EquipmentComponent = Character->GetEquipmentComponent();
	if (!EquipmentComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAnimMontage* FireMontage = EquipmentComponent->GetCurrentLightAttackMontage();

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CachedHandle = Handle;
	CachedActorInfo = ActorInfo;
	CachedActivationInfo = ActivationInfo;
	bHasHandledMontageExit = false;

	SpawnProjectileAndEffects(Character);

	if (FireMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this,
				NAME_None,
				FireMontage);

		if (!MontageTask)
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		MontageTask->OnCompleted.AddDynamic(this, &USFGameplayAbility_RangedAttackBs::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &USFGameplayAbility_RangedAttackBs::OnMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &USFGameplayAbility_RangedAttackBs::OnMontageCancelled);
		MontageTask->OnBlendOut.AddDynamic(this, &USFGameplayAbility_RangedAttackBs::OnMontageBlendOut);
		MontageTask->ReadyForActivation();
		return;
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}

void USFGameplayAbility_RangedAttackBs::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	ClearCachedActivationState();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool USFGameplayAbility_RangedAttackBs::TryGetRangedCharacter(
	const FGameplayAbilityActorInfo* ActorInfo,
	ASFCharacterBase*& OutCharacter) const
{
	OutCharacter = nullptr;

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		return false;
	}

	OutCharacter = Cast<ASFCharacterBase>(ActorInfo->AvatarActor.Get());
	return OutCharacter != nullptr;
}

void USFGameplayAbility_RangedAttackBs::HandleMontageFinished(bool bWasCancelled, bool bReplicateEndAbility)
{
	if (bHasHandledMontageExit || !IsActive())
	{
		return;
	}

	bHasHandledMontageExit = true;
	EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USFGameplayAbility_RangedAttackBs::ClearCachedActivationState()
{
	CachedHandle = FGameplayAbilitySpecHandle();
	CachedActorInfo = nullptr;
	CachedActivationInfo = FGameplayAbilityActivationInfo();
	bHasHandledMontageExit = false;
}

void USFGameplayAbility_RangedAttackBs::OnMontageCompleted()
{
	HandleMontageFinished(false, false);
}

void USFGameplayAbility_RangedAttackBs::OnMontageInterrupted()
{
	HandleMontageFinished(true, true);
}

void USFGameplayAbility_RangedAttackBs::OnMontageCancelled()
{
	HandleMontageFinished(true, true);
}

void USFGameplayAbility_RangedAttackBs::OnMontageBlendOut()
{
	HandleMontageFinished(false, false);
}

void USFGameplayAbility_RangedAttackBs::ResolveSpawnTransform(
	ASFCharacterBase* Character,
	FVector& OutSpawnLocation,
	FRotator& OutSpawnRotation,
	ASFWeaponActor*& OutWeaponActor) const
{
	OutWeaponActor = nullptr;

	const FVector ForwardVector = Character->GetActorForwardVector();
	OutSpawnLocation =
		Character->GetActorLocation() +
		(ForwardVector * FallbackSpawnDistance) +
		FVector(0.0f, 0.0f, FallbackSpawnHeightOffset);

	OutSpawnRotation = ForwardVector.Rotation();

	if (USFEquipmentComponent* EquipmentComponent = Character->GetEquipmentComponent())
	{
		OutWeaponActor = EquipmentComponent->GetEquippedWeaponActor();

		if (OutWeaponActor && OutWeaponActor->HasValidMuzzleSocket())
		{
			OutSpawnLocation = OutWeaponActor->GetMuzzleLocation();
			OutSpawnRotation = OutWeaponActor->GetMuzzleRotation();
		}
	}
}

void USFGameplayAbility_RangedAttackBs::SpawnMuzzleEffects(
	ASFCharacterBase* Character,
	ASFWeaponActor* WeaponActor,
	const FVector& SpawnLocation,
	const FRotator& SpawnRotation) const
{
	if (!MuzzleNiagara || !Character)
	{
		return;
	}

	if (WeaponActor)
	{
		const FName FXSocketName =
			WeaponActor->HasValidMuzzleFXSocket()
			? WeaponActor->GetMuzzleFXSocketName()
			: FName(TEXT("Muzzle"));

		if (USkeletalMeshComponent* WeaponSkeletalMesh = WeaponActor->GetSkeletalMeshComponent())
		{
			if (WeaponSkeletalMesh->DoesSocketExist(FXSocketName))
			{
				UNiagaraFunctionLibrary::SpawnSystemAttached(
					MuzzleNiagara,
					WeaponSkeletalMesh,
					FXSocketName,
					FVector::ZeroVector,
					FRotator::ZeroRotator,
					EAttachLocation::SnapToTarget,
					true);
				return;
			}
		}
		else if (UStaticMeshComponent* WeaponStaticMesh = WeaponActor->GetStaticMeshComponent())
		{
			if (WeaponStaticMesh->DoesSocketExist(FXSocketName))
			{
				UNiagaraFunctionLibrary::SpawnSystemAttached(
					MuzzleNiagara,
					WeaponStaticMesh,
					FXSocketName,
					FVector::ZeroVector,
					FRotator::ZeroRotator,
					EAttachLocation::SnapToTarget,
					true);
				return;
			}
		}
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		Character->GetWorld(),
		MuzzleNiagara,
		SpawnLocation,
		SpawnRotation);
}

void USFGameplayAbility_RangedAttackBs::SpawnProjectileAndEffects(ASFCharacterBase* Character)
{
	if (!Character || !ProjectileClass)
	{
		return;
	}

	FVector SpawnLocation = FVector::ZeroVector;
	FRotator SpawnRotation = FRotator::ZeroRotator;
	ASFWeaponActor* WeaponActor = nullptr;

	ResolveSpawnTransform(Character, SpawnLocation, SpawnRotation, WeaponActor);
	SpawnMuzzleEffects(Character, WeaponActor, SpawnLocation, SpawnRotation);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ASFProjectileBase* Projectile = Character->GetWorld()->SpawnActor<ASFProjectileBase>(
		ProjectileClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParams);

	if (Projectile)
	{
		Projectile->SetSourceActor(Character);
		Projectile->SetDamageEffect(ProjectileDamageEffect);
	}
}