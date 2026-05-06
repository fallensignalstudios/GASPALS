#include "AbilitySystem/Effects/Execution/SFDamageExecutionCalculation.h"

#include "Core/SFAttributeSetBase.h"
#include "Core/SignalForgeGameplayTags.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"

struct FSFDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(Damage);

	DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower);
	DECLARE_ATTRIBUTE_CAPTUREDEF(AbilityPower);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CritChance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CritMultiplier);
	DECLARE_ATTRIBUTE_CAPTUREDEF(WeakpointBonus);

	DECLARE_ATTRIBUTE_CAPTUREDEF(Armor);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DamageReduction);
	DECLARE_ATTRIBUTE_CAPTUREDEF(DodgeChance);

	DECLARE_ATTRIBUTE_CAPTUREDEF(Guard);
	DECLARE_ATTRIBUTE_CAPTUREDEF(MaxGuard);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Poise);
	DECLARE_ATTRIBUTE_CAPTUREDEF(PoiseDamage);

	FSFDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(USFAttributeSetBase, Damage, Source, true);

		DEFINE_ATTRIBUTE_CAPTUREDEF(USFAttributeSetBase, AttackPower, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USFAttributeSetBase, AbilityPower, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USFAttributeSetBase, CritChance, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USFAttributeSetBase, CritMultiplier, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USFAttributeSetBase, WeakpointBonus, Source, true);

		DEFINE_ATTRIBUTE_CAPTUREDEF(USFAttributeSetBase, Armor, Target, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USFAttributeSetBase, DamageReduction, Target, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USFAttributeSetBase, DodgeChance, Target, true);

		DEFINE_ATTRIBUTE_CAPTUREDEF(USFAttributeSetBase, Guard, Target, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USFAttributeSetBase, MaxGuard, Target, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USFAttributeSetBase, Poise, Target, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USFAttributeSetBase, PoiseDamage, Source, true);
	}
};

static const FSFDamageStatics& DamageStatics()
{
	static FSFDamageStatics Statics;
	return Statics;
}

USFDamageExecutionCalculation::USFDamageExecutionCalculation()
{
	const FSFDamageStatics& Statics = DamageStatics();

	RelevantAttributesToCapture.Add(Statics.DamageDef);

	RelevantAttributesToCapture.Add(Statics.AttackPowerDef);
	RelevantAttributesToCapture.Add(Statics.AbilityPowerDef);
	RelevantAttributesToCapture.Add(Statics.CritChanceDef);
	RelevantAttributesToCapture.Add(Statics.CritMultiplierDef);
	RelevantAttributesToCapture.Add(Statics.WeakpointBonusDef);

	RelevantAttributesToCapture.Add(Statics.ArmorDef);
	RelevantAttributesToCapture.Add(Statics.DamageReductionDef);
	RelevantAttributesToCapture.Add(Statics.DodgeChanceDef);

	RelevantAttributesToCapture.Add(Statics.GuardDef);
	RelevantAttributesToCapture.Add(Statics.MaxGuardDef);
	RelevantAttributesToCapture.Add(Statics.PoiseDef);
	RelevantAttributesToCapture.Add(Statics.PoiseDamageDef);
}

void USFDamageExecutionCalculation::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput
) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	const FSFDamageStatics& Statics = DamageStatics();
	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();

	float Damage = 0.0f;

	// 1) Start from captured Damage attribute and SetByCaller base damage
	float CapturedDamage = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		Statics.DamageDef, EvalParams, CapturedDamage);

	Damage += FMath::Max(CapturedDamage, 0.0f);
	Damage += FMath::Max(
		Spec.GetSetByCallerMagnitude(Tags.Data_BaseDamage, false, 0.0f),
		0.0f);

	// 2) Capture offensive and defensive stats
	float AttackPower = 0.0f;
	float AbilityPower = 0.0f;
	float CritChance = 0.0f;
	float CritMultiplier = 1.0f;
	float WeakpointBonus = 0.0f;

	float Armor = 0.0f;
	float DamageReduction = 0.0f;
	float DodgeChance = 0.0f;

	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		Statics.AttackPowerDef, EvalParams, AttackPower);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		Statics.AbilityPowerDef, EvalParams, AbilityPower);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		Statics.CritChanceDef, EvalParams, CritChance);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		Statics.CritMultiplierDef, EvalParams, CritMultiplier);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		Statics.WeakpointBonusDef, EvalParams, WeakpointBonus);

	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		Statics.ArmorDef, EvalParams, Armor);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		Statics.DamageReductionDef, EvalParams, DamageReduction);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		Statics.DodgeChanceDef, EvalParams, DodgeChance);

	AttackPower = FMath::Max(AttackPower, 0.0f);
	AbilityPower = FMath::Max(AbilityPower, 0.0f);
	CritChance = FMath::Clamp(CritChance, 0.0f, 1.0f);
	CritMultiplier = FMath::Max(CritMultiplier, 1.0f);
	WeakpointBonus = FMath::Max(WeakpointBonus, 0.0f);

	Armor = FMath::Max(Armor, 0.0f);
	DamageReduction = FMath::Clamp(DamageReduction, 0.0f, 1.0f);
	DodgeChance = FMath::Clamp(DodgeChance, 0.0f, 1.0f);

	// 3) AttackPower / AbilityPower scaling via SetByCaller
	const float AttackPowerScale = FMath::Max(
		Spec.GetSetByCallerMagnitude(Tags.Data_AttackPowerScale, false, 0.0f),
		0.0f);
	const float AbilityPowerScale = FMath::Max(
		Spec.GetSetByCallerMagnitude(Tags.Data_AbilityPowerScale, false, 0.0f),
		0.0f);

	Damage += AttackPower * AttackPowerScale;
	Damage += AbilityPower * AbilityPowerScale;

	// 4) Weakpoint handling
	const bool bIsWeakpointHit =
		Spec.GetSetByCallerMagnitude(Tags.Data_IsWeakpointHit, false, 0.0f) > 0.0f;

	if (bIsWeakpointHit)
	{
		Damage += WeakpointBonus;
	}

	// 5) Crit adjustments: base + SetByCaller bonuses
	CritChance += FMath::Max(
		Spec.GetSetByCallerMagnitude(Tags.Data_BonusCritChance, false, 0.0f),
		0.0f);
	CritMultiplier += FMath::Max(
		Spec.GetSetByCallerMagnitude(Tags.Data_BonusCritMultiplier, false, 0.0f),
		0.0f);

	if (bIsWeakpointHit)
	{
		CritChance = FMath::Clamp(CritChance + 0.25f, 0.0f, 1.0f);
	}

	// 6) Dodge check on target
	const bool bDodged = FMath::FRand() < DodgeChance;
	if (bDodged)
	{
		Damage = 0.0f;
	}

	// 7) Crit roll if we still have damage
	if (Damage > 0.0f)
	{
		const bool bCrit = FMath::FRand() < CritChance;
		if (bCrit)
		{
			Damage *= CritMultiplier;
		}
	}

	// 8) Armor and damage reduction, with optional ignore flags
	const bool bIgnoreArmor =
		Spec.GetSetByCallerMagnitude(Tags.Data_IgnoreArmor, false, 0.0f) > 0.0f;
	const bool bIgnoreDamageReduction =
		Spec.GetSetByCallerMagnitude(Tags.Data_IgnoreDamageReduction, false, 0.0f) > 0.0f;

	if (Damage > 0.0f && !bIgnoreArmor)
	{
		Damage = FMath::Max(Damage - Armor, 0.0f);
	}

	if (Damage > 0.0f && !bIgnoreDamageReduction)
	{
		Damage *= (1.0f - DamageReduction);
	}

	// 9) Guard + poise handling

	// Read set-by-caller flags
	const bool bIsBlockable =
		Spec.GetSetByCallerMagnitude(Tags.Data_IsBlockable, false, 1.0f) > 0.0f;
	const bool bIgnoreGuard =
		Spec.GetSetByCallerMagnitude(Tags.Data_IgnoreGuard, false, 0.0f) > 0.0f;

	// Check if target is blocking (via state tag)
	const bool bTargetBlocking = EvalParams.TargetTags->HasTag(Tags.State_Blocking);

	// Compute poise damage from source PoiseDamage + scale
	float SourcePoiseDamage = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		Statics.PoiseDamageDef, EvalParams, SourcePoiseDamage);
	SourcePoiseDamage = FMath::Max(SourcePoiseDamage, 0.0f);

	const float PoiseScale = FMath::Max(
		Spec.GetSetByCallerMagnitude(Tags.Data_PoiseDamageScale, false, 1.0f),
		0.0f);
	const float BreakGuardBonus = FMath::Max(
		Spec.GetSetByCallerMagnitude(Tags.Data_BreakGuardBonus, false, 0.0f),
		0.0f);

	float PoiseDamage = SourcePoiseDamage * PoiseScale;
	if (bTargetBlocking)
	{
		PoiseDamage += BreakGuardBonus;
	}

	// Route some or all damage into Guard when blocking
	if (Damage > 0.0f && bIsBlockable && bTargetBlocking && !bIgnoreGuard)
	{
		float TargetGuard = 0.0f;
		float TargetMaxGuard = 0.0f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
			Statics.GuardDef, EvalParams, TargetGuard);
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
			Statics.MaxGuardDef, EvalParams, TargetMaxGuard);

		TargetGuard = FMath::Clamp(TargetGuard, 0.0f, TargetMaxGuard);

		const float DamageToGuard = FMath::Min(Damage, TargetGuard);
		const float DamageToHealth = FMath::Max(Damage - DamageToGuard, 0.0f);

		if (DamageToGuard > 0.0f)
		{
			OutExecutionOutput.AddOutputModifier(
				FGameplayModifierEvaluatedData(
					USFAttributeSetBase::GetGuardAttribute(),
					EGameplayModOp::Additive,
					-DamageToGuard
				)
			);
		}

		Damage = DamageToHealth;
	}

	// Apply poise damage directly to Poise
	if (PoiseDamage > 0.0f)
	{
		OutExecutionOutput.AddOutputModifier(
			FGameplayModifierEvaluatedData(
				USFAttributeSetBase::GetPoiseAttribute(),
				EGameplayModOp::Additive,
				-PoiseDamage
			)
		);
	}

	// 10) Final clamp and output Damage meta attribute
	Damage = FMath::Max(Damage, 0.0f);

	if (Damage > 0.0f)
	{
		OutExecutionOutput.AddOutputModifier(
			FGameplayModifierEvaluatedData(
				USFAttributeSetBase::GetDamageAttribute(),
				EGameplayModOp::Additive,
				Damage
			)
		);
	}
}