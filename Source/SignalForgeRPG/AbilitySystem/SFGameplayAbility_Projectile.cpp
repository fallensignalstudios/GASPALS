#include "AbilitySystem/SFGameplayAbility_Projectile.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Characters/SFCharacterBase.h"
#include "Combat/SFProjectileBase.h"
#include "Components/SFEquipmentComponent.h"
#include "Core/SignalForgeGameplayTags.h"
#include "Engine/World.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

USFGameplayAbility_Projectile::USFGameplayAbility_Projectile()
{
	const FSignalForgeGameplayTags& GameplayTags = FSignalForgeGameplayTags::Get();

	InputTag = GameplayTags.Input_Ability_4;

	FGameplayTagContainer AssetTags;
	// You could introduce a dedicated Ability.Projectile tag later.
	SetAssetTags(AssetTags);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool USFGameplayAbility_Projectile::CanActivateAbility(
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
	if (!TryGetProjectileCharacter(ActorInfo, Character))
	{
		return false;
	}

	return ProjectileClass != nullptr;
}

void USFGameplayAbility_Projectile::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ASFCharacterBase* Character = nullptr;
	if (!TryGetProjectileCharacter(ActorInfo, Character) || !ProjectileClass)
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

	UAnimMontage* CastMontage = EquipmentComponent->GetCurrentAbilityMontage();

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CachedHandle = Handle;
	CachedActorInfo = ActorInfo;
	CachedActivationInfo = ActivationInfo;
	bHasHandledMontageExit = false;

	// For v1, spawn the projectile immediately rather than syncing to an anim notify.
	SpawnProjectileAndEffects(Character);

	if (CastMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this,
				NAME_None,
				CastMontage);

		if (!MontageTask)
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		MontageTask->OnCompleted.AddDynamic(this, &USFGameplayAbility_Projectile::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &USFGameplayAbility_Projectile::OnMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &USFGameplayAbility_Projectile::OnMontageCancelled);
		MontageTask->OnBlendOut.AddDynamic(this, &USFGameplayAbility_Projectile::OnMontageBlendOut);
		MontageTask->ReadyForActivation();

		return;
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}

void USFGameplayAbility_Projectile::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	ClearCachedActivationState();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool USFGameplayAbility_Projectile::TryGetProjectileCharacter(
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

void USFGameplayAbility_Projectile::HandleMontageFinished(bool bWasCancelled, bool bReplicateEndAbility)
{
	if (bHasHandledMontageExit || !IsActive())
	{
		return;
	}

	bHasHandledMontageExit = true;
	EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USFGameplayAbility_Projectile::ClearCachedActivationState()
{
	CachedHandle = FGameplayAbilitySpecHandle();
	CachedActorInfo = nullptr;
	CachedActivationInfo = FGameplayAbilityActivationInfo();
	bHasHandledMontageExit = false;
}

void USFGameplayAbility_Projectile::OnMontageCompleted()
{
	HandleMontageFinished(false, false);
}

void USFGameplayAbility_Projectile::OnMontageInterrupted()
{
	HandleMontageFinished(true, true);
}

void USFGameplayAbility_Projectile::OnMontageCancelled()
{
	HandleMontageFinished(true, true);
}

void USFGameplayAbility_Projectile::OnMontageBlendOut()
{
	HandleMontageFinished(false, false);
}

void USFGameplayAbility_Projectile::SpawnProjectileAndEffects(ASFCharacterBase* Character)
{
	if (!Character || !ProjectileClass)
	{
		return;
	}

	const FVector ForwardVector = Character->GetActorForwardVector();
	const FVector SpawnLocation =
		Character->GetActorLocation() +
		(ForwardVector * SpawnDistance) +
		FVector(0.0f, 0.0f, SpawnHeightOffset);

	const FRotator SpawnRotation = ForwardVector.Rotation();

	if (CastNiagara)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			Character->GetWorld(),
			CastNiagara,
			SpawnLocation,
			SpawnRotation);
	}

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