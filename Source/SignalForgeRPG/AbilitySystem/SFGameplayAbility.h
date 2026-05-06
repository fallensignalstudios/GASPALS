#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "SFGameplayAbility.generated.h"

class UTexture2D;
class UWorld;
class UAnimMontage;
class UNiagaraSystem;
class USoundBase;
class ACharacter;

UENUM(BlueprintType)
enum class ESFAbilityActivationPolicy : uint8
{
	OnInputTriggered,
	WhileInputActive
};

UCLASS(Abstract)
class SIGNALFORGERPG_API USFGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	USFGameplayAbility();

	UFUNCTION(BlueprintPure, Category = "SF|Ability")
	ESFAbilityActivationPolicy GetActivationPolicy() const
	{
		return ActivationPolicy;
	}

	UFUNCTION(BlueprintPure, Category = "SF|Ability")
	const FGameplayTag& GetInputTag() const
	{
		return InputTag;
	}

	UFUNCTION(BlueprintPure, Category = "SF|Ability")
	bool HasInputTag() const
	{
		return InputTag.IsValid();
	}

	UFUNCTION(BlueprintPure, Category = "SF|UI")
	UTexture2D* GetAbilityIcon() const
	{
		return AbilityIcon;
	}

	UFUNCTION(BlueprintPure, Category = "SF|UI")
	bool HasCooldownTag() const
	{
		return CooldownTag.IsValid();
	}

	UFUNCTION(BlueprintPure, Category = "SF|UI")
	const FGameplayTag& GetCooldownTag() const
	{
		return CooldownTag;
	}

	UFUNCTION(BlueprintPure, Category = "SF|UI")
	bool ShouldShowInAbilityBar() const
	{
		return bShowInAbilityBar;
	}

	UFUNCTION(BlueprintPure, Category = "SF|Animation")
	UAnimMontage* GetActivationMontage() const
	{
		return ActivationMontage;
	}

	UFUNCTION(BlueprintPure, Category = "SF|VFX")
	UNiagaraSystem* GetActivationVFX() const
	{
		return ActivationVFX;
	}

	UFUNCTION(BlueprintPure, Category = "SF|Audio")
	USoundBase* GetActivationSFX() const
	{
		return ActivationSFX;
	}

protected:
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	void NotifyAbilityUIChanged(const FGameplayAbilityActorInfo* ActorInfo) const;

	bool PlayAbilityMontage(float PlayRate = 1.0f, FName StartSectionName = NAME_None);
	void StopAbilityMontage(float OverrideBlendOutTime = 0.2f);
	void SpawnAbilityVFX() const;
	void PlayAbilitySFX() const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|Input")
	ESFAbilityActivationPolicy ActivationPolicy = ESFAbilityActivationPolicy::OnInputTriggered;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|Input", meta = (Categories = "InputTag"))
	FGameplayTag InputTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|UI")
	TObjectPtr<UTexture2D> AbilityIcon = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|UI", meta = (Categories = "Cooldown"))
	FGameplayTag CooldownTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|UI")
	bool bShowInAbilityBar = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|Animation")
	TObjectPtr<UAnimMontage> ActivationMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|VFX")
	TObjectPtr<UNiagaraSystem> ActivationVFX = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|VFX")
	FName ActivationVFXAttachSocket = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|VFX")
	FVector ActivationVFXRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|VFX")
	FRotator ActivationVFXRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|VFX")
	FVector ActivationVFXRelativeScale = FVector(1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SF|Audio")
	TObjectPtr<USoundBase> ActivationSFX = nullptr;
};