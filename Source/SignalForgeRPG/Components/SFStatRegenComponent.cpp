#include "Components/SFStatRegenComponent.h"

#include "AbilitySystemComponent.h"
#include "Characters/SFCharacterBase.h"
#include "Core/SFAttributeSetBase.h"
#include "Core/SignalForgeGameplayTags.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "TimerManager.h"

USFStatRegenComponent::USFStatRegenComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USFStatRegenComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ASFCharacterBase>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			RegenTickHandle,
			this,
			&USFStatRegenComponent::TickRegen,
			RegenTickInterval,
			true
		);
	}
}

void USFStatRegenComponent::TickRegen()
{
	if (!OwnerCharacter || OwnerCharacter->IsDead())
	{
		return;
	}

	const USFAttributeSetBase* Attributes = OwnerCharacter->GetAttributeSet();
	if (!Attributes)
	{
		return;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	if (CanRegenHealth(CurrentTime) && Attributes->GetHealth() < Attributes->GetMaxHealth())
	{
		ApplyRegenEffect(HealthRegenEffect, HealthRegenRate * RegenTickInterval);
	}

	if (CanRegenShields(CurrentTime) && Attributes->GetShields() < Attributes->GetMaxShields())
	{
		ApplyRegenEffect(ShieldsRegenEffect, ShieldsRegenRate * RegenTickInterval);
	}

	if (CanRegenStamina(CurrentTime) && Attributes->GetStamina() < Attributes->GetMaxStamina())
	{
		ApplyRegenEffect(StaminaRegenEffect, StaminaRegenRate * RegenTickInterval);
	}

	if (CanRegenEcho(CurrentTime) && Attributes->GetEcho() < Attributes->GetMaxEcho())
	{
		ApplyRegenEffect(EchoRegenEffect, EchoRegenRate * RegenTickInterval);
	}

	if (CanRegenGuard(CurrentTime) && Attributes->GetGuard() < Attributes->GetMaxGuard())
	{
		ApplyRegenEffect(GuardRegenEffect, GuardRegenRate * RegenTickInterval);
	}
}

bool USFStatRegenComponent::CanRegenHealth(float CurrentTime) const
{
	return (CurrentTime - LastHealthDamageTime) >= HealthRegenDelay;
}

bool USFStatRegenComponent::CanRegenShields(float CurrentTime) const
{
	return (CurrentTime - LastShieldDamageTime) >= ShieldsRegenDelay;
}

bool USFStatRegenComponent::CanRegenStamina(float CurrentTime) const
{
	return !bSprintBlockingStaminaRegen
		&& ((CurrentTime - LastStaminaSpendTime) >= StaminaRegenDelay);
}

bool USFStatRegenComponent::CanRegenEcho(float CurrentTime) const
{
	return (CurrentTime - LastEchoSpendTime) >= EchoRegenDelay;
}

bool USFStatRegenComponent::CanRegenGuard(float CurrentTime) const
{
	return (CurrentTime - LastGuardDamageTime) >= GuardRegenDelay;
}

void USFStatRegenComponent::ApplyRegenEffect(TSubclassOf<UGameplayEffect> EffectClass, float Magnitude) const
{
	if (!OwnerCharacter || !EffectClass)
	{
		return;
	}

	UAbilitySystemComponent* ASC = OwnerCharacter->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, 1.0f, EffectContext);
	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		return;
	}

	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
	SpecHandle.Data->SetSetByCallerMagnitude(Tags.Data_RegenAmount, Magnitude);

	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void USFStatRegenComponent::NotifyDamageTaken(bool bAffectedHealth, bool bAffectedShields)
{
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	if (bAffectedHealth)
	{
		LastHealthDamageTime = Now;
	}

	if (bAffectedShields)
	{
		LastShieldDamageTime = Now;
	}
}

void USFStatRegenComponent::NotifyGuardDamaged()
{
	LastGuardDamageTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
}

void USFStatRegenComponent::NotifyStaminaSpent()
{
	LastStaminaSpendTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
}

void USFStatRegenComponent::NotifyEchoSpent()
{
	LastEchoSpendTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
}

void USFStatRegenComponent::NotifySprintStarted()
{
	bSprintBlockingStaminaRegen = true;
	LastStaminaSpendTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
}

void USFStatRegenComponent::NotifySprintStopped()
{
	bSprintBlockingStaminaRegen = false;
	LastStaminaSpendTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
}

void USFStatRegenComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RegenTickHandle);
	}

	Super::EndPlay(EndPlayReason);
}