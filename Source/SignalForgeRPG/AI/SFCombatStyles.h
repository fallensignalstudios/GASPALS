#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SFCombatStyles.generated.h"

/**
 * High-level enemy combat-style preset. Picked on the enemy character (or
 * overridden on the Fire Weapon BT task). The BT task reads the matching
 * profile values and configures itself for that style.
 *
 * Custom lets designers author bespoke per-encounter tuning without picking
 * one of the named presets (e.g. boss-specific patterns).
 */
UENUM(BlueprintType)
enum class ESFCombatStyle : uint8
{
	/** No preset; the BT task uses the explicit fields authored on it directly. */
	Custom,

	/**
	 * Halo-grunt feel. ADS lead-in then a tight burst of shots, brief recovery.
	 * Used for standard rifle infantry.
	 */
	BurstShooter,

	/**
	 * Stormtrooper feel. No ADS, hip-fire single shots in rapid taps with high
	 * spread. Used for cannon-fodder enemies.
	 */
	HipFireSpammer,

	/**
	 * Destiny-vandal feel. Long ADS lead-in, single high-damage shot, long
	 * recovery. Used for designated marksmen and snipers.
	 */
	PrecisionMarksman,

	/**
	 * Halo-elite-sword feel. No ADS, fast single-swing per attack, short
	 * recovery so combos chain. Used for sword and energy-blade enemies.
	 */
	MeleeBrute,

	/**
	 * Knife-assassin / dreg feel. No ADS, two-hit flurry per activation, very
	 * short recovery. Used for light melee skirmishers.
	 */
	MeleeAssassin,
};

/**
 * Concrete tuning values produced by an ESFCombatStyle. Filled in by
 * SFBTTask_FireWeapon::ResolveStyleProfile so the task body only ever reads
 * one set of fields regardless of whether the designer picked a preset or
 * authored values directly.
 *
 * Field meanings match the corresponding UPROPERTYs on the BT task; see
 * SFBTTask_FireWeapon.h for the canonical docs.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFCombatStyleProfile
{
	GENERATED_BODY()

	/** Fire mode the task should run. 0=Tap, 1=Hold, 2=PressOnly, 3=Burst. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat Style")
	uint8 FireMode = 0;

	/** Seconds to hold input in Hold mode. Ignored otherwise. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat Style")
	float HoldDuration = 0.5f;

	/** Number of shots in Burst mode. Ignored otherwise. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat Style")
	int32 BurstShotCount = 3;

	/** Seconds between shots inside a burst. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat Style")
	float BurstInterShotDelay = 0.10f;

	/** True = press ADS and wait AdsLeadInSeconds before firing. False = hip fire. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat Style")
	bool bRequiresAds = true;

	/** ADS settle time before firing. Ignored when bRequiresAds is false. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat Style")
	float AdsLeadInSeconds = 0.35f;
};
