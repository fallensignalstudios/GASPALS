#include "Core/SFAttributeSetBase.h"

#include "AbilitySystemComponent.h"
#include "Characters/SFCharacterBase.h"
#include "Components/SFStatRegenComponent.h"
#include "Core/SignalForgeGameplayTags.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "Kismet/GameplayStatics.h"
#include "UI/SFDamageNumberSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogSFDamagePipeline, Log, All);

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

		UE_LOG(LogSFDamagePipeline, Log,
			TEXT("PostGEExec Damage meta branch: actor=%s LocalDamage=%.2f HealthBefore=%.2f ShieldsBefore=%.2f"),
			*GetNameSafe(OwnerActor), LocalDamage, GetHealth(), GetShields());

		if (LocalDamage <= 0.0f)
		{
			return;
		}

		ApplyShieldedDamage(LocalDamage, Character);

		// Destiny-style damage-number floater: only show when the local player
		// was the instigator. Filtering here (rather than in the subsystem)
		// keeps the spec-context lookup cheap and avoids spawning widgets on
		// remote/AI damage events. Reads IsCrit / IsWeakpointHit / FinalDamage
		// SetByCallers populated by SFDamageExecutionCalculation; falls back
		// to LocalDamage if FinalDamage wasn't written (e.g. raw Damage meta
		// hits that bypass the exec calc).
		if (UWorld* World = GetWorld())
		{
			const FGameplayEffectSpec& Spec = Data.EffectSpec;
			AActor* Instigator = Spec.GetContext().GetInstigator();
			APawn* LocalPawn = UGameplayStatics::GetPlayerPawn(World, 0);
			if (Instigator && LocalPawn && Instigator == LocalPawn)
			{
				const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
				const bool bCrit = Spec.GetSetByCallerMagnitude(Tags.Data_IsCrit, false, 0.0f) > 0.0f;
				const bool bWeakpoint = Spec.GetSetByCallerMagnitude(Tags.Data_IsWeakpointHit, false, 0.0f) > 0.0f;
				const float FinalDamage = Spec.GetSetByCallerMagnitude(Tags.Data_FinalDamage, false, LocalDamage);

				// Prefer the actual impact point from the hit result for accurate
				// placement; fall back to target center + half-height so floaters
				// still appear above the head for AoE / projectile splash damage
				// where no per-bone hit was recorded.
				FVector ImpactLocation = FVector::ZeroVector;
				if (const FHitResult* HitResult = Spec.GetContext().GetHitResult())
				{
					ImpactLocation = HitResult->ImpactPoint;
				}
				if (ImpactLocation.IsNearlyZero() && OwnerActor)
				{
					ImpactLocation = OwnerActor->GetActorLocation();
					if (Character)
					{
						ImpactLocation.Z += Character->GetSimpleCollisionHalfHeight();
					}
				}

				if (APlayerController* LocalPC = UGameplayStatics::GetPlayerController(World, 0))
				{
					if (ULocalPlayer* LP = LocalPC->GetLocalPlayer())
					{
						if (USFDamageNumberSubsystem* DN = LP->GetSubsystem<USFDamageNumberSubsystem>())
						{
							DN->ShowDamageNumber(FinalDamage, bCrit, bWeakpoint, ImpactLocation);
						}
					}
				}
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		// Negative magnitude == incoming damage. Re-route through the shield-first pipeline
		// so designer GEs that modify Health directly still observe shields, hit react,
		// and death. Positive magnitudes (heals) are clamped and pass through unchanged --
		// unless the same spec is also a damage spec (Data.BaseDamage SetByCaller), in which
		// case the positive Health modifier is a designer mistake (e.g. a stray Health
		// modifier alongside the Damage execution calculation) and we must revert it,
		// otherwise every damage hit silently re-heals the target to MaxHealth.
		const float Magnitude = Data.EvaluatedData.Magnitude;
		UE_LOG(LogSFDamagePipeline, Log,
			TEXT("PostGEExec Health branch: actor=%s Magnitude=%.2f HealthNow=%.2f MaxHealth=%.2f"),
			*GetNameSafe(OwnerActor), Magnitude, GetHealth(), GetMaxHealth());
		if (Magnitude < 0.0f)
		{
			// Undo the raw health write that the GE just performed, then re-apply via
			// the shielded path. We subtract Magnitude (it's negative -> adds health back).
			const float HealthAfterRawHit = GetHealth();
			const float HealthBeforeRawHit = FMath::Clamp(HealthAfterRawHit - Magnitude, 0.0f, GetMaxHealth());
			UE_LOG(LogSFDamagePipeline, Log,
				TEXT("  -> RawWrite restore: HealthAfterRaw=%.2f HealthBeforeRaw=%.2f (will SetHealth then ApplyShieldedDamage(%.2f))"),
				HealthAfterRawHit, HealthBeforeRawHit, -Magnitude);
			SetHealth(HealthBeforeRawHit);

			ApplyShieldedDamage(-Magnitude, Character);
		}
		else if (Magnitude > 0.0f)
		{
			// Is this a damage spec? Damage specs carry the Data.BaseDamage SetByCaller
			// magnitude (set by USFCombatComponent at apply time). If present, this positive
			// Health modifier is a misconfiguration on the damage GE that would silently
			// un-do the damage. Revert it so the Damage meta branch is the single source of
			// truth for that hit.
			const FGameplayEffectSpec& Spec = Data.EffectSpec;
			const FSignalForgeGameplayTags& SFTags = FSignalForgeGameplayTags::Get();
			const bool bIsDamageSpec = Spec.GetSetByCallerMagnitude(SFTags.Data_BaseDamage, false, 0.0f) > 0.0f;
			if (bIsDamageSpec)
			{
				const float HealthAfterRawWrite = GetHealth();
				const float HealthBeforeRawWrite = FMath::Clamp(HealthAfterRawWrite - Magnitude, 0.0f, GetMaxHealth());
				UE_LOG(LogSFDamagePipeline, Warning,
					TEXT("  -> SUPPRESSED unintended heal: damage GE '%s' has a positive Health modifier (+%.2f). Reverting %.2f -> %.2f. Open the GE and remove the Health modifier; keep only the Damage execution calculation."),
					*GetNameSafe(Spec.Def), Magnitude, HealthAfterRawWrite, HealthBeforeRawWrite);
				SetHealth(HealthBeforeRawWrite);
			}
			else
			{
				SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
			}
		}
		else
		{
			SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
		}
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

void USFAttributeSetBase::ApplyShieldedDamage(float IncomingDamage, ASFCharacterBase* Character)
{
	if (IncomingDamage <= 0.0f)
	{
		return;
	}

	const float ShieldsBeforeDamage = GetShields();
	const float HealthBeforeDamage = GetHealth();
	float RemainingDamage = IncomingDamage;

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

	UE_LOG(LogSFDamagePipeline, Log,
		TEXT("ApplyShieldedDamage: actor=%s Incoming=%.2f ShieldsBefore=%.2f ShieldsAfter=%.2f HealthBefore=%.2f HealthAfter=%.2f"),
		*GetNameSafe(Character), IncomingDamage, ShieldsBeforeDamage, GetShields(),
		HealthBeforeDamage, GetHealth());

	const bool bShieldsWereDamaged = GetShields() < ShieldsBeforeDamage;
	const bool bHealthWasDamaged = GetHealth() < HealthBeforeDamage;

	if (!Character)
	{
		return;
	}

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