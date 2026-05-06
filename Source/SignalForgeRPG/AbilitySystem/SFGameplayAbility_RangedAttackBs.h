#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SFGameplayAbility.h"
#include "SFGameplayAbility_RangedAttackBs.generated.h"

class ASFProjectileBase;
class UGameplayEffect;
class UNiagaraSystem;
class ASFCharacterBase;
class ASFWeaponActor;

UCLASS()
class SIGNALFORGERPG_API USFGameplayAbility_RangedAttackBs : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGameplayAbility_RangedAttackBs();

	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged Attack")
	TSubclassOf<ASFProjectileBase> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged Attack")
	TSubclassOf<UGameplayEffect> ProjectileDamageEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged Attack", meta = (ClampMin = "0.0"))
	float FallbackSpawnDistance = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged Attack")
	float FallbackSpawnHeightOffset = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ranged Attack")
	TObjectPtr<UNiagaraSystem> MuzzleNiagara = nullptr;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnMontageBlendOut();

	void SpawnProjectileAndEffects(ASFCharacterBase* Character);

private:
	bool TryGetRangedCharacter(const FGameplayAbilityActorInfo* ActorInfo, ASFCharacterBase*& OutCharacter) const;
	void HandleMontageFinished(bool bWasCancelled, bool bReplicateEndAbility);
	void ClearCachedActivationState();

	void ResolveSpawnTransform(
		ASFCharacterBase* Character,
		FVector& OutSpawnLocation,
		FRotator& OutSpawnRotation,
		ASFWeaponActor*& OutWeaponActor) const;

	void SpawnMuzzleEffects(
		ASFCharacterBase* Character,
		ASFWeaponActor* WeaponActor,
		const FVector& SpawnLocation,
		const FRotator& SpawnRotation) const;

private:
	FGameplayAbilitySpecHandle CachedHandle;
	const FGameplayAbilityActorInfo* CachedActorInfo = nullptr;
	FGameplayAbilityActivationInfo CachedActivationInfo;
	bool bHasHandledMontageExit = false;
};