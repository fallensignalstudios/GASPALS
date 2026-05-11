#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

struct FSignalForgeGameplayTags
{
public:
	static const FSignalForgeGameplayTags& Get();
	static void InitializeNativeGameplayTags();

	/** Attributes */
	FGameplayTag Attribute_Health;
	FGameplayTag Attribute_MaxHealth;
	FGameplayTag Attribute_Echo;
	FGameplayTag Attribute_MaxEcho;
	FGameplayTag Attribute_Stamina;
	FGameplayTag Attribute_MaxStamina;
	FGameplayTag Attribute_Shields;
	FGameplayTag Attribute_MaxShields;

	FGameplayTag Attribute_Damage;

	FGameplayTag Attribute_AttackPower;
	FGameplayTag Attribute_AbilityPower;
	FGameplayTag Attribute_CritChance;
	FGameplayTag Attribute_CritMultiplier;
	FGameplayTag Attribute_WeakpointBonus;

	FGameplayTag Attribute_Armor;
	FGameplayTag Attribute_DamageReduction;
	FGameplayTag Attribute_DodgeChance;

	FGameplayTag Attribute_MoveSpeedMultiplier;
	FGameplayTag Attribute_AttackSpeedMultiplier;
	FGameplayTag Attribute_CooldownReduction;

	FGameplayTag Attribute_Guard;
	FGameplayTag Attribute_MaxGuard;
	FGameplayTag Attribute_Poise;
	FGameplayTag Attribute_PoiseDamage;

	/** Damage / execution / regen data */
	FGameplayTag Data_BaseDamage;
	FGameplayTag Data_AttackPowerScale;
	FGameplayTag Data_AbilityPowerScale;
	FGameplayTag Data_IsWeakpointHit;
	FGameplayTag Data_BonusCritChance;
	FGameplayTag Data_BonusCritMultiplier;
	FGameplayTag Data_IgnoreArmor;
	FGameplayTag Data_IgnoreDamageReduction;

	FGameplayTag Data_PoiseDamageScale;
	FGameplayTag Data_IsBlockable;
	FGameplayTag Data_IgnoreGuard;
	FGameplayTag Data_BreakGuardBonus;

	FGameplayTag Data_RegenAmount;

	/** Ability identity */
	FGameplayTag Ability_Attack_Light;
	FGameplayTag Ability_Attack_Heavy;
	FGameplayTag Ability_Movement_Dash;
	FGameplayTag Ability_Movement_Sprint;
	FGameplayTag Ability_Ranged_Projectile;

	/** State */
	FGameplayTag State_Attacking;
	FGameplayTag State_Movement_Sprinting;
	FGameplayTag State_Blocking;
	FGameplayTag State_Broken;

	/** Cinematic / hit feedback */
	FGameplayTag Cue_Hit_Impact;
	FGameplayTag Cue_Hit_Critical;
	FGameplayTag Cue_Hit_Weakpoint;
	FGameplayTag Cue_Hit_Block;
	FGameplayTag Cue_Hit_Parry;
	FGameplayTag Cue_Hit_GuardBreak;
	FGameplayTag Cue_Hit_Finisher;

	/** Hit reaction direction (relative to target's forward) */
	FGameplayTag HitReact_Direction_Front;
	FGameplayTag HitReact_Direction_Back;
	FGameplayTag HitReact_Direction_Left;
	FGameplayTag HitReact_Direction_Right;

	/** Hit reaction severity */
	FGameplayTag HitReact_Severity_Light;
	FGameplayTag HitReact_Severity_Heavy;
	FGameplayTag HitReact_Severity_Stagger;
	FGameplayTag HitReact_Severity_Launch;

	/** Ranged weapon abilities */
	FGameplayTag Ability_Weapon_PrimaryFire;
	FGameplayTag Ability_Weapon_SecondaryFire;
	FGameplayTag Ability_Weapon_Reload;
	FGameplayTag Ability_Weapon_ADS;
	FGameplayTag Ability_Grenade_Throw;

	/** Ranged weapon state */
	FGameplayTag State_Weapon_Firing;
	FGameplayTag State_Weapon_Reloading;
	FGameplayTag State_Weapon_ADS;
	FGameplayTag State_Weapon_Empty;
	FGameplayTag State_Weapon_Overheated;
	FGameplayTag State_Grenade_Cooking;

	/** Weapon / grenade GameplayCues */
	FGameplayTag Cue_Weapon_MuzzleFlash;
	FGameplayTag Cue_Weapon_ShellEject;
	FGameplayTag Cue_Weapon_Reload;
	FGameplayTag Cue_Weapon_Empty;
	FGameplayTag Cue_Weapon_Tracer;
	FGameplayTag Cue_Weapon_ADS_Enter;
	FGameplayTag Cue_Weapon_ADS_Exit;
	FGameplayTag Cue_Grenade_Explode;
	FGameplayTag Cue_Grenade_Bounce;

	/** Inputs for ranged weapons */
	FGameplayTag Input_PrimaryFire;
	FGameplayTag Input_SecondaryFire;
	FGameplayTag Input_Reload;
	FGameplayTag Input_Grenade;
	FGameplayTag Input_ADS;

	/** Damage scaling SetByCallers (ranged) */
	FGameplayTag Data_FalloffMultiplier;
	FGameplayTag Data_RangedDistance;

	/** Input */
	FGameplayTag Input_Ability_Sprint;
	FGameplayTag Input_Ability_1;
	FGameplayTag Input_Ability_2;
	FGameplayTag Input_Ability_3;
	FGameplayTag Input_Ability_4;
	FGameplayTag Input_Ability_5;
	FGameplayTag Input_Ability_6;
	FGameplayTag Input_Ability_7;
	FGameplayTag Input_Ability_8;
	FGameplayTag Input_Ability_9;
	FGameplayTag Input_Ability_10;
	FGameplayTag Input_Ability_Block;

	/** Equipment slots */
	FGameplayTag Equipment_Slot_Head;
	FGameplayTag Equipment_Slot_Chest;
	FGameplayTag Equipment_Slot_Arms;
	FGameplayTag Equipment_Slot_Legs;
	FGameplayTag Equipment_Slot_Boots;
	FGameplayTag Equipment_Slot_PrimaryWeapon;
	FGameplayTag Equipment_Slot_SecondaryWeapon;
	FGameplayTag Equipment_Slot_HeavyWeapon;

private:
	static FSignalForgeGameplayTags GameplayTags;
	static bool bIsInitialized;
};