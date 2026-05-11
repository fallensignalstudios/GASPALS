#include "Core/SignalForgeGameplayTags.h"
#include "GameplayTagsManager.h"

FSignalForgeGameplayTags FSignalForgeGameplayTags::GameplayTags;
bool FSignalForgeGameplayTags::bIsInitialized = false;

const FSignalForgeGameplayTags& FSignalForgeGameplayTags::Get()
{
	return GameplayTags;
}

void FSignalForgeGameplayTags::InitializeNativeGameplayTags()
{
	if (bIsInitialized)
	{
		return;
	}

	bIsInitialized = true;

	UGameplayTagsManager& TagManager = UGameplayTagsManager::Get();

	/** Attributes */
	GameplayTags.Attribute_Health = TagManager.AddNativeGameplayTag(
		FName("Attributes.Health"),
		TEXT("Current health value"));

	GameplayTags.Attribute_MaxHealth = TagManager.AddNativeGameplayTag(
		FName("Attributes.MaxHealth"),
		TEXT("Maximum health value"));

	GameplayTags.Attribute_Echo = TagManager.AddNativeGameplayTag(
		FName("Attributes.Echo"),
		TEXT("Current echo resource value"));

	GameplayTags.Attribute_MaxEcho = TagManager.AddNativeGameplayTag(
		FName("Attributes.MaxEcho"),
		TEXT("Maximum echo resource value"));

	GameplayTags.Attribute_Stamina = TagManager.AddNativeGameplayTag(
		FName("Attributes.Stamina"),
		TEXT("Current stamina value"));

	GameplayTags.Attribute_MaxStamina = TagManager.AddNativeGameplayTag(
		FName("Attributes.MaxStamina"),
		TEXT("Maximum stamina value"));

	GameplayTags.Attribute_Shields = TagManager.AddNativeGameplayTag(
		FName("Attributes.Shields"),
		TEXT("Current shields value"));

	GameplayTags.Attribute_MaxShields = TagManager.AddNativeGameplayTag(
		FName("Attributes.MaxShields"),
		TEXT("Maximum shields value"));

	GameplayTags.Attribute_Damage = TagManager.AddNativeGameplayTag(
		FName("Attributes.Damage"),
		TEXT("Meta damage attribute used by damage executions"));

	GameplayTags.Attribute_AttackPower = TagManager.AddNativeGameplayTag(
		FName("Attributes.AttackPower"),
		TEXT("General offensive power for physical and weapon-based damage"));

	GameplayTags.Attribute_AbilityPower = TagManager.AddNativeGameplayTag(
		FName("Attributes.AbilityPower"),
		TEXT("Offensive power for abilities and special techniques"));

	GameplayTags.Attribute_CritChance = TagManager.AddNativeGameplayTag(
		FName("Attributes.CritChance"),
		TEXT("Chance to critically hit, normalized from 0 to 1"));

	GameplayTags.Attribute_CritMultiplier = TagManager.AddNativeGameplayTag(
		FName("Attributes.CritMultiplier"),
		TEXT("Critical damage multiplier, e.g. 1.5 for 150 percent damage"));

	GameplayTags.Attribute_WeakpointBonus = TagManager.AddNativeGameplayTag(
		FName("Attributes.WeakpointBonus"),
		TEXT("Additional bonus damage applied on weakpoint hits"));

	GameplayTags.Attribute_Armor = TagManager.AddNativeGameplayTag(
		FName("Attributes.Armor"),
		TEXT("Flat damage mitigation value"));

	GameplayTags.Attribute_DamageReduction = TagManager.AddNativeGameplayTag(
		FName("Attributes.DamageReduction"),
		TEXT("Percent damage reduction, normalized from 0 to 1"));

	GameplayTags.Attribute_DodgeChance = TagManager.AddNativeGameplayTag(
		FName("Attributes.DodgeChance"),
		TEXT("Chance to fully avoid a hit, normalized from 0 to 1"));

	GameplayTags.Attribute_MoveSpeedMultiplier = TagManager.AddNativeGameplayTag(
		FName("Attributes.MoveSpeedMultiplier"),
		TEXT("Multiplier applied to character movement speed"));

	GameplayTags.Attribute_AttackSpeedMultiplier = TagManager.AddNativeGameplayTag(
		FName("Attributes.AttackSpeedMultiplier"),
		TEXT("Multiplier applied to attack speed or animation rate"));

	GameplayTags.Attribute_CooldownReduction = TagManager.AddNativeGameplayTag(
		FName("Attributes.CooldownReduction"),
		TEXT("Reduction to ability cooldowns, normalized from 0 to 1"));

	GameplayTags.Attribute_Guard = TagManager.AddNativeGameplayTag(
		FName("Attributes.Guard"),
		TEXT("Guard meter used when blocking to absorb damage"));

	GameplayTags.Attribute_MaxGuard = TagManager.AddNativeGameplayTag(
		FName("Attributes.MaxGuard"),
		TEXT("Maximum guard value"));

	GameplayTags.Attribute_Poise = TagManager.AddNativeGameplayTag(
		FName("Attributes.Poise"),
		TEXT("Poise / posture meter used for stagger / break"));

	GameplayTags.Attribute_PoiseDamage = TagManager.AddNativeGameplayTag(
		FName("Attributes.PoiseDamage"),
		TEXT("How much poise damage this character's attacks deal"));

	/** Damage / execution / regen data */
	GameplayTags.Data_BaseDamage = TagManager.AddNativeGameplayTag(
		FName("Data.BaseDamage"),
		TEXT("SetByCaller base damage for a damage effect"));

	GameplayTags.Data_AttackPowerScale = TagManager.AddNativeGameplayTag(
		FName("Data.AttackPowerScale"),
		TEXT("SetByCaller scaling coefficient for AttackPower"));

	GameplayTags.Data_AbilityPowerScale = TagManager.AddNativeGameplayTag(
		FName("Data.AbilityPowerScale"),
		TEXT("SetByCaller scaling coefficient for AbilityPower"));

	GameplayTags.Data_IsWeakpointHit = TagManager.AddNativeGameplayTag(
		FName("Data.IsWeakpointHit"),
		TEXT("SetByCaller flag indicating whether the hit struck a weakpoint"));

	GameplayTags.Data_BonusCritChance = TagManager.AddNativeGameplayTag(
		FName("Data.BonusCritChance"),
		TEXT("SetByCaller bonus critical hit chance"));

	GameplayTags.Data_BonusCritMultiplier = TagManager.AddNativeGameplayTag(
		FName("Data.BonusCritMultiplier"),
		TEXT("SetByCaller bonus critical damage multiplier"));

	GameplayTags.Data_IgnoreArmor = TagManager.AddNativeGameplayTag(
		FName("Data.IgnoreArmor"),
		TEXT("SetByCaller flag causing the hit to ignore armor"));

	GameplayTags.Data_IgnoreDamageReduction = TagManager.AddNativeGameplayTag(
		FName("Data.IgnoreDamageReduction"),
		TEXT("SetByCaller flag causing the hit to ignore percent damage reduction"));

	GameplayTags.Data_PoiseDamageScale = TagManager.AddNativeGameplayTag(
		FName("Data.PoiseDamageScale"),
		TEXT("SetByCaller scale applied to PoiseDamage for a hit"));

	GameplayTags.Data_IsBlockable = TagManager.AddNativeGameplayTag(
		FName("Data.IsBlockable"),
		TEXT("SetByCaller flag indicating whether this hit can be blocked"));

	GameplayTags.Data_IgnoreGuard = TagManager.AddNativeGameplayTag(
		FName("Data.IgnoreGuard"),
		TEXT("SetByCaller flag causing the hit to ignore guard"));

	GameplayTags.Data_BreakGuardBonus = TagManager.AddNativeGameplayTag(
		FName("Data.BreakGuardBonus"),
		TEXT("SetByCaller bonus poise damage against guard"));

	GameplayTags.Data_RegenAmount = TagManager.AddNativeGameplayTag(
		FName("Data.RegenAmount"),
		TEXT("SetByCaller regen magnitude used by simple regen effects"));

	/** Ability identity */
	GameplayTags.Ability_Attack_Light = TagManager.AddNativeGameplayTag(
		FName("Ability.Attack.Light"),
		TEXT("Light attack ability tag"));

	GameplayTags.Ability_Attack_Heavy = TagManager.AddNativeGameplayTag(
		FName("Ability.Attack.Heavy"),
		TEXT("Heavy attack ability tag"));

	GameplayTags.Ability_Movement_Dash = TagManager.AddNativeGameplayTag(
		FName("Ability.Movement.Dash"),
		TEXT("Dash ability tag"));

	GameplayTags.Ability_Movement_Sprint = TagManager.AddNativeGameplayTag(
		FName("Ability.Movement.Sprint"),
		TEXT("Sprint ability tag"));

	GameplayTags.Ability_Ranged_Projectile = TagManager.AddNativeGameplayTag(
		FName("Ability.Ranged.Projectile"),
		TEXT("Projectile-based ranged ability tag"));

	/** State */
	GameplayTags.State_Attacking = TagManager.AddNativeGameplayTag(
		FName("State.Attacking"),
		TEXT("Character is currently attacking"));

	GameplayTags.State_Movement_Sprinting = TagManager.AddNativeGameplayTag(
		FName("State.Movement.Sprinting"),
		TEXT("Character is currently sprinting"));

	GameplayTags.State_Blocking = TagManager.AddNativeGameplayTag(
		FName("State.Blocking"),
		TEXT("Character is currently blocking"));

	GameplayTags.State_Broken = TagManager.AddNativeGameplayTag(
		FName("State.Broken"),
		TEXT("Character is guard-broken or staggered"));

	/** Cinematic / hit feedback cues */
	GameplayTags.Cue_Hit_Impact = TagManager.AddNativeGameplayTag(
		FName("GameplayCue.Hit.Impact"),
		TEXT("Generic melee impact cue (sound, VFX, decals)"));

	GameplayTags.Cue_Hit_Critical = TagManager.AddNativeGameplayTag(
		FName("GameplayCue.Hit.Critical"),
		TEXT("Critical strike cue with amplified VFX and audio"));

	GameplayTags.Cue_Hit_Weakpoint = TagManager.AddNativeGameplayTag(
		FName("GameplayCue.Hit.Weakpoint"),
		TEXT("Weakpoint / armor-piercing hit cue"));

	GameplayTags.Cue_Hit_Block = TagManager.AddNativeGameplayTag(
		FName("GameplayCue.Hit.Block"),
		TEXT("Hit absorbed by a guard"));

	GameplayTags.Cue_Hit_Parry = TagManager.AddNativeGameplayTag(
		FName("GameplayCue.Hit.Parry"),
		TEXT("Perfect parry cinematic cue"));

	GameplayTags.Cue_Hit_GuardBreak = TagManager.AddNativeGameplayTag(
		FName("GameplayCue.Hit.GuardBreak"),
		TEXT("Guard-break impact cue"));

	GameplayTags.Cue_Hit_Finisher = TagManager.AddNativeGameplayTag(
		FName("GameplayCue.Hit.Finisher"),
		TEXT("Finisher / execution cinematic cue"));

	/** Hit reaction direction */
	GameplayTags.HitReact_Direction_Front = TagManager.AddNativeGameplayTag(
		FName("HitReact.Direction.Front"),
		TEXT("Hit was received from the front"));

	GameplayTags.HitReact_Direction_Back = TagManager.AddNativeGameplayTag(
		FName("HitReact.Direction.Back"),
		TEXT("Hit was received from behind"));

	GameplayTags.HitReact_Direction_Left = TagManager.AddNativeGameplayTag(
		FName("HitReact.Direction.Left"),
		TEXT("Hit was received from the left side"));

	GameplayTags.HitReact_Direction_Right = TagManager.AddNativeGameplayTag(
		FName("HitReact.Direction.Right"),
		TEXT("Hit was received from the right side"));

	/** Hit reaction severity */
	GameplayTags.HitReact_Severity_Light = TagManager.AddNativeGameplayTag(
		FName("HitReact.Severity.Light"),
		TEXT("Minor flinch / light hit reaction"));

	GameplayTags.HitReact_Severity_Heavy = TagManager.AddNativeGameplayTag(
		FName("HitReact.Severity.Heavy"),
		TEXT("Strong hit reaction with brief locomotion interrupt"));

	GameplayTags.HitReact_Severity_Stagger = TagManager.AddNativeGameplayTag(
		FName("HitReact.Severity.Stagger"),
		TEXT("Full stagger reaction; cancels actions"));

	GameplayTags.HitReact_Severity_Launch = TagManager.AddNativeGameplayTag(
		FName("HitReact.Severity.Launch"),
		TEXT("Launched / knocked back reaction"));

	/** Ranged weapon abilities */
	GameplayTags.Ability_Weapon_PrimaryFire = TagManager.AddNativeGameplayTag(
		FName("Ability.Weapon.PrimaryFire"),
		TEXT("Weapon primary fire (trigger pull)"));

	GameplayTags.Ability_Weapon_SecondaryFire = TagManager.AddNativeGameplayTag(
		FName("Ability.Weapon.SecondaryFire"),
		TEXT("Weapon secondary fire (alt fire / underbarrel)"));

	GameplayTags.Ability_Weapon_Reload = TagManager.AddNativeGameplayTag(
		FName("Ability.Weapon.Reload"),
		TEXT("Weapon reload ability"));

	GameplayTags.Ability_Weapon_ADS = TagManager.AddNativeGameplayTag(
		FName("Ability.Weapon.ADS"),
		TEXT("Aim down sights ability"));

	GameplayTags.Ability_Grenade_Throw = TagManager.AddNativeGameplayTag(
		FName("Ability.Grenade.Throw"),
		TEXT("Throw grenade ability"));

	/** Ranged weapon state */
	GameplayTags.State_Weapon_Firing = TagManager.AddNativeGameplayTag(
		FName("State.Weapon.Firing"),
		TEXT("Weapon is in active fire cycle"));

	GameplayTags.State_Weapon_Reloading = TagManager.AddNativeGameplayTag(
		FName("State.Weapon.Reloading"),
		TEXT("Weapon is currently reloading"));

	GameplayTags.State_Weapon_ADS = TagManager.AddNativeGameplayTag(
		FName("State.Weapon.ADS"),
		TEXT("Character is aiming down sights"));

	GameplayTags.State_Weapon_Empty = TagManager.AddNativeGameplayTag(
		FName("State.Weapon.Empty"),
		TEXT("Weapon magazine is empty"));

	GameplayTags.State_Weapon_Overheated = TagManager.AddNativeGameplayTag(
		FName("State.Weapon.Overheated"),
		TEXT("Energy / plasma weapon is overheated"));

	GameplayTags.State_Grenade_Cooking = TagManager.AddNativeGameplayTag(
		FName("State.Grenade.Cooking"),
		TEXT("Grenade fuse is burning before throw"));

	/** Weapon / grenade GameplayCues */
	GameplayTags.Cue_Weapon_MuzzleFlash = TagManager.AddNativeGameplayTag(
		FName("GameplayCue.Weapon.MuzzleFlash"),
		TEXT("Muzzle flash VFX + audio"));

	GameplayTags.Cue_Weapon_ShellEject = TagManager.AddNativeGameplayTag(
		FName("GameplayCue.Weapon.ShellEject"),
		TEXT("Brass / shell casing eject"));

	GameplayTags.Cue_Weapon_Reload = TagManager.AddNativeGameplayTag(
		FName("GameplayCue.Weapon.Reload"),
		TEXT("Reload audio / mag-swap VFX"));

	GameplayTags.Cue_Weapon_Empty = TagManager.AddNativeGameplayTag(
		FName("GameplayCue.Weapon.Empty"),
		TEXT("Dry-fire click"));

	GameplayTags.Cue_Weapon_Tracer = TagManager.AddNativeGameplayTag(
		FName("GameplayCue.Weapon.Tracer"),
		TEXT("Tracer / beam trail for hitscan or projectile"));

	GameplayTags.Cue_Grenade_Explode = TagManager.AddNativeGameplayTag(
		FName("GameplayCue.Grenade.Explode"),
		TEXT("Grenade explosion (VFX + audio + radial light)"));

	GameplayTags.Cue_Grenade_Bounce = TagManager.AddNativeGameplayTag(
		FName("GameplayCue.Grenade.Bounce"),
		TEXT("Grenade bounce on surface"));

	/** Ranged inputs */
	GameplayTags.Input_PrimaryFire = TagManager.AddNativeGameplayTag(
		FName("InputTag.PrimaryFire"),
		TEXT("Primary fire input (trigger / LMB)"));

	GameplayTags.Input_SecondaryFire = TagManager.AddNativeGameplayTag(
		FName("InputTag.SecondaryFire"),
		TEXT("Secondary fire input (alt fire / RMB)"));

	GameplayTags.Input_Reload = TagManager.AddNativeGameplayTag(
		FName("InputTag.Reload"),
		TEXT("Reload input"));

	GameplayTags.Input_Grenade = TagManager.AddNativeGameplayTag(
		FName("InputTag.Grenade"),
		TEXT("Grenade throw input"));

	/** Damage scaling SetByCallers (ranged) */
	GameplayTags.Data_FalloffMultiplier = TagManager.AddNativeGameplayTag(
		FName("Data.FalloffMultiplier"),
		TEXT("SetByCaller multiplier from distance falloff"));

	GameplayTags.Data_RangedDistance = TagManager.AddNativeGameplayTag(
		FName("Data.RangedDistance"),
		TEXT("SetByCaller cm distance from shooter to target at hit time"));

	/** Input */
	GameplayTags.Input_Ability_Sprint = TagManager.AddNativeGameplayTag(
		FName("InputTag.Ability.Sprint"),
		TEXT("Input tag for sprint ability"));

	GameplayTags.Input_Ability_1 = TagManager.AddNativeGameplayTag(
		FName("InputTag.Ability.1"),
		TEXT("Input tag for ability slot 1"));

	GameplayTags.Input_Ability_2 = TagManager.AddNativeGameplayTag(
		FName("InputTag.Ability.2"),
		TEXT("Input tag for ability slot 2"));

	GameplayTags.Input_Ability_3 = TagManager.AddNativeGameplayTag(
		FName("InputTag.Ability.3"),
		TEXT("Input tag for ability slot 3"));

	GameplayTags.Input_Ability_4 = TagManager.AddNativeGameplayTag(
		FName("InputTag.Ability.4"),
		TEXT("Input tag for ability slot 4"));

	GameplayTags.Input_Ability_5 = TagManager.AddNativeGameplayTag(
		FName("InputTag.Ability.5"),
		TEXT("Input tag for ability slot 5"));

	GameplayTags.Input_Ability_6 = TagManager.AddNativeGameplayTag(
		FName("InputTag.Ability.6"),
		TEXT("Input tag for ability slot 6"));

	GameplayTags.Input_Ability_7 = TagManager.AddNativeGameplayTag(
		FName("InputTag.Ability.7"),
		TEXT("Input tag for ability slot 7"));

	GameplayTags.Input_Ability_8 = TagManager.AddNativeGameplayTag(
		FName("InputTag.Ability.8"),
		TEXT("Input tag for ability slot 8"));

	GameplayTags.Input_Ability_9 = TagManager.AddNativeGameplayTag(
		FName("InputTag.Ability.9"),
		TEXT("Input tag for ability slot 9"));

	GameplayTags.Input_Ability_10 = TagManager.AddNativeGameplayTag(
		FName("InputTag.Ability.10"),
		TEXT("Input tag for ability slot 10"));

	GameplayTags.Input_Ability_Block = TagManager.AddNativeGameplayTag(
		FName("InputTag.Ability.Block"),
		TEXT("Input tag for block ability"));

	/** Equipment slots */
	GameplayTags.Equipment_Slot_Head = TagManager.AddNativeGameplayTag(
		FName("Equipment.Slot.Head"),
		TEXT("Head armor slot"));

	GameplayTags.Equipment_Slot_Chest = TagManager.AddNativeGameplayTag(
		FName("Equipment.Slot.Chest"),
		TEXT("Chest armor slot"));

	GameplayTags.Equipment_Slot_Arms = TagManager.AddNativeGameplayTag(
		FName("Equipment.Slot.Arms"),
		TEXT("Arms armor slot"));

	GameplayTags.Equipment_Slot_Legs = TagManager.AddNativeGameplayTag(
		FName("Equipment.Slot.Legs"),
		TEXT("Legs armor slot"));

	GameplayTags.Equipment_Slot_Boots = TagManager.AddNativeGameplayTag(
		FName("Equipment.Slot.Boots"),
		TEXT("Boots armor slot"));

	GameplayTags.Equipment_Slot_PrimaryWeapon = TagManager.AddNativeGameplayTag(
		FName("Equipment.Slot.PrimaryWeapon"),
		TEXT("Primary weapon slot"));

	GameplayTags.Equipment_Slot_SecondaryWeapon = TagManager.AddNativeGameplayTag(
		FName("Equipment.Slot.SecondaryWeapon"),
		TEXT("Secondary weapon slot"));

	GameplayTags.Equipment_Slot_HeavyWeapon = TagManager.AddNativeGameplayTag(
		FName("Equipment.Slot.HeavyWeapon"),
		TEXT("Heavy weapon slot"));
}