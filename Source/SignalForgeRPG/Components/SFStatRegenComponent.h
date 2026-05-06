#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SFStatRegenComponent.generated.h"

class ASFCharacterBase;
class UGameplayEffect;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SIGNALFORGERPG_API USFStatRegenComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFStatRegenComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY()
	TObjectPtr<ASFCharacterBase> OwnerCharacter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Regen")
	float RegenTickInterval = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Regen|Health")
	float HealthRegenRate = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Regen|Health")
	float HealthRegenDelay = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Regen|Shields")
	float ShieldsRegenRate = 15.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Regen|Shields")
	float ShieldsRegenDelay = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Regen|Stamina")
	float StaminaRegenRate = 25.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Regen|Stamina")
	float StaminaRegenDelay = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Regen|Echo")
	float EchoRegenRate = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Regen|Echo")
	float EchoRegenDelay = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Regen|Guard")
	float GuardRegenRate = 12.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Regen|Guard")
	float GuardRegenDelay = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Regen|Effects")
	TSubclassOf<UGameplayEffect> HealthRegenEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Regen|Effects")
	TSubclassOf<UGameplayEffect> ShieldsRegenEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Regen|Effects")
	TSubclassOf<UGameplayEffect> StaminaRegenEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Regen|Effects")
	TSubclassOf<UGameplayEffect> EchoRegenEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Regen|Effects")
	TSubclassOf<UGameplayEffect> GuardRegenEffect;

	UPROPERTY(BlueprintReadOnly, Category = "Regen|State")
	bool bSprintBlockingStaminaRegen = false;

	float LastHealthDamageTime = -BIG_NUMBER;
	float LastShieldDamageTime = -BIG_NUMBER;
	float LastStaminaSpendTime = -BIG_NUMBER;
	float LastEchoSpendTime = -BIG_NUMBER;
	float LastGuardDamageTime = -BIG_NUMBER;

	FTimerHandle RegenTickHandle;

	void TickRegen();

	bool CanRegenHealth(float CurrentTime) const;
	bool CanRegenShields(float CurrentTime) const;
	bool CanRegenStamina(float CurrentTime) const;
	bool CanRegenEcho(float CurrentTime) const;
	bool CanRegenGuard(float CurrentTime) const;

	void ApplyRegenEffect(TSubclassOf<UGameplayEffect> EffectClass, float Magnitude) const;

public:
	UFUNCTION(BlueprintCallable, Category = "Regen")
	void NotifyDamageTaken(bool bAffectedHealth, bool bAffectedShields);

	UFUNCTION(BlueprintCallable, Category = "Regen")
	void NotifyGuardDamaged();

	UFUNCTION(BlueprintCallable, Category = "Regen")
	void NotifyStaminaSpent();

	UFUNCTION(BlueprintCallable, Category = "Regen")
	void NotifyEchoSpent();

	UFUNCTION(BlueprintCallable, Category = "Regen")
	void NotifySprintStarted();

	UFUNCTION(BlueprintCallable, Category = "Regen")
	void NotifySprintStopped();
};