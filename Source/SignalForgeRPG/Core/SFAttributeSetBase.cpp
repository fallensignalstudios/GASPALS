#include "Core/SFAttributeSetBase.h"

#include "AbilitySystemComponent.h"
#include "Characters/SFCharacterBase.h"
#include "Components/SFStatRegenComponent.h"
#include "GameplayEffectExtension.h"

USFAttributeSetBase::USFAttributeSetBase()
{
}

void USFAttributeSetBase::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetEchoAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxEcho());
	}
	else if (Attribute == GetShieldsAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxShields());
	}
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}
	else if (Attribute == GetGuardAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxGuard());
	}
	else if (Attribute == GetMaxHealthAttribute()
		|| Attribute == GetMaxEchoAttribute()
		|| Attribute == GetMaxShieldsAttribute()
		|| Attribute == GetMaxStaminaAttribute()
		|| Attribute == GetMaxGuardAttribute()
		|| Attribute == GetDamageAttribute()
		|| Attribute == GetAttackPowerAttribute()
		|| Attribute == GetAbilityPowerAttribute()
		|| Attribute == GetWeakpointBonusAttribute()
		|| Attribute == GetArmorAttribute()
		|| Attribute == GetPoiseAttribute()
		|| Attribute == GetPoiseDamageAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetCritChanceAttribute()
		|| Attribute == GetDamageReductionAttribute()
		|| Attribute == GetDodgeChanceAttribute()
		|| Attribute == GetCooldownReductionAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, 1.0f);
	}
	else if (Attribute == GetCritMultiplierAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	else if (Attribute == GetMoveSpeedMultiplierAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.1f, 3.0f);
	}
	else if (Attribute == GetAttackSpeedMultiplierAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.1f, 5.0f);
	}
}

void USFAttributeSetBase::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	AActor* OwnerActor = ASC->GetAvatarActor();
	ASFCharacterBase* Character = Cast<ASFCharacterBase>(OwnerActor);

	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		const float LocalDamage = GetDamage();
		SetDamage(0.0f);

		if (LocalDamage <= 0.0f)
		{
			return;
		}

		const float ShieldsBeforeDamage = GetShields();
		const float HealthBeforeDamage = GetHealth();
		float RemainingDamage = LocalDamage;

		if (ShieldsBeforeDamage > 0.0f)
		{
			const float ShieldDamage = FMath::Min(ShieldsBeforeDamage, RemainingDamage);
			SetShields(FMath::Clamp(ShieldsBeforeDamage - ShieldDamage, 0.0f, GetMaxShields()));
			RemainingDamage -= ShieldDamage;
		}

		if (RemainingDamage > 0.0f)
		{
			SetHealth(FMath::Clamp(GetHealth() - RemainingDamage, 0.0f, GetMaxHealth()));
		}

		const bool bShieldsWereDamaged = GetShields() < ShieldsBeforeDamage;
		const bool bHealthWasDamaged = GetHealth() < HealthBeforeDamage;

		if (Character)
		{
			if (ShieldsBeforeDamage > 0.0f && GetShields() <= 0.0f)
			{
				// TODO: Shield break event / gameplay cue / UI hook
			}

			if (GetHealth() <= 0.0f)
			{
				if (!Character->IsDead())
				{
					Character->HandleDeath();
				}
			}
			else if (bHealthWasDamaged)
			{
				Character->HandleHitReact();
			}

			if (Character->GetStatRegenComponent())
			{
				Character->GetStatRegenComponent()->NotifyDamageTaken(
					bHealthWasDamaged,
					bShieldsWereDamaged
				);
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetEchoAttribute())
	{
		SetEcho(FMath::Clamp(GetEcho(), 0.0f, GetMaxEcho()));
	}
	else if (Data.EvaluatedData.Attribute == GetShieldsAttribute())
	{
		SetShields(FMath::Clamp(GetShields(), 0.0f, GetMaxShields()));
	}
	else if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		SetStamina(FMath::Clamp(GetStamina(), 0.0f, GetMaxStamina()));
	}
	else if (Data.EvaluatedData.Attribute == GetGuardAttribute())
	{
		const float NewGuard = FMath::Clamp(GetGuard(), 0.0f, GetMaxGuard());
		const float PreviousGuard = NewGuard - Data.EvaluatedData.Magnitude;
		SetGuard(NewGuard);

		if (Character && Character->GetStatRegenComponent() && NewGuard < PreviousGuard)
		{
			Character->GetStatRegenComponent()->NotifyGuardDamaged();
		}

		if (Character && PreviousGuard > 0.0f && NewGuard <= 0.0f)
		{
			Character->HandleGuardBreak();
		}
	}
	else if (Data.EvaluatedData.Attribute == GetPoiseAttribute())
	{
		const float NewPoise = FMath::Max(GetPoise(), 0.0f);
		const float PreviousPoise = NewPoise - Data.EvaluatedData.Magnitude;
		SetPoise(NewPoise);

		if (Character && PreviousPoise > 0.0f && NewPoise <= 0.0f)
		{
			Character->HandlePoiseBreak();
		}
	}
}