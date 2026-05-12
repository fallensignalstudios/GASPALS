// Combat/SFWeaponInstanceTypes.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Combat/SFWeaponStatBlock.h"
#include "SFWeaponInstanceTypes.generated.h"

class USFWeaponPerkDefinition;
class USFWeaponData;

USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFWeaponTuningData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tuning", meta = (ClampMin = "0", ClampMax = "10"))
	int32 RangeBonus = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tuning", meta = (ClampMin = "0", ClampMax = "10"))
	int32 StabilityBonus = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tuning", meta = (ClampMin = "0", ClampMax = "10"))
	int32 HandlingBonus = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tuning", meta = (ClampMin = "0", ClampMax = "10"))
	int32 ReloadBonus = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tuning", meta = (ClampMin = "0", ClampMax = "10"))
	int32 MagazineBonus = 0;

	FSFWeaponStatBlock ToStatBlock() const
	{
		FSFWeaponStatBlock Result;
		Result.Damage = 0.0f;
		Result.Range = static_cast<float>(RangeBonus);
		Result.Stability = static_cast<float>(StabilityBonus);
		Result.Handling = static_cast<float>(HandlingBonus);
		Result.ReloadSpeed = static_cast<float>(ReloadBonus);
		Result.MagazineSize = static_cast<float>(MagazineBonus);
		Result.Accuracy = 0.0f;
		Result.RecoilControl = 0.0f;
		Result.AimAssist = 0.0f;
		Result.GuardEfficiency = 0.0f;
		return Result;
	}
};

USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFWeaponInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
	FGuid InstanceId = FGuid::NewGuid();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
	TSoftObjectPtr<USFWeaponData> WeaponDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rolls")
	TSoftObjectPtr<USFWeaponPerkDefinition> BarrelPerk;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rolls")
	TSoftObjectPtr<USFWeaponPerkDefinition> MagazinePerk;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rolls")
	TSoftObjectPtr<USFWeaponPerkDefinition> TraitColumn1Perk;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rolls")
	TSoftObjectPtr<USFWeaponPerkDefinition> TraitColumn2Perk;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rolls")
	TSoftObjectPtr<USFWeaponPerkDefinition> OriginTraitPerk;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rolls")
	TSoftObjectPtr<USFWeaponPerkDefinition> MasterworkPerk;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo")
	int32 AmmoInClip = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo")
	float LastAttackTime = 0.0f;

	/**
	 * Current beam battery charge for energy weapons (FSFBeamWeaponConfig).
	 * Initialized lazily to BatteryCapacity the first time the beam ability runs.
	 * A negative sentinel means "uninitialized" so we don't have to special-case full
	 * capacity per weapon-data load. Persists across holster / weapon switch since
	 * it lives on the instance, not on the ability.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Beam")
	float BeamBatteryCharge = -1.0f;

	/** True while the weapon is overheated and locked out from firing until charge recovers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Beam")
	bool bBeamOverheated = false;

	/** World time (seconds) of the most recent beam tick; drives recharge-delay gating. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Beam")
	float LastBeamFireWorldTime = 0.0f;

	/**
	 * Current light-combo step index. Increments per light swing, resets to 0 after
	 * MeleeConfig.ComboResetSeconds of idle. Persists across holster so a player who
	 * swaps mid-combo and back resumes where they left off (Bayonetta-style).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
	int32 MeleeLightComboStep = 0;

	/** Current heavy-combo step. Independent from light combo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
	int32 MeleeHeavyComboStep = 0;

	/** World time of the last successful swing input (any lane). Drives combo reset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
	float LastMeleeAttackWorldTime = 0.0f;

	/**
	 * Which hand swung last for paired weapons (0 = mainhand, 1 = offhand). The next swing
	 * picks the opposite hand. Ignored for non-paired weapons.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee|Paired")
	int32 LastSwingHandIndex = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression", meta = (ClampMin = "0"))
	int32 EnhancementLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression", meta = (ClampMin = "1"))
	int32 WeaponLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	FSFWeaponTuningData TuningData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	bool bLocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	bool bFavorite = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	FGameplayTagContainer InstanceTags;

	bool IsValid() const
	{
		return InstanceId.IsValid() && !WeaponDefinition.IsNull();
	}
};