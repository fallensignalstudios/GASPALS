#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GameplayEffect.h"
#include "Abilities/GameplayAbility.h"
#include "SFClassDefinition.generated.h"

class UAnimBlueprint;
class USFItemDefinition;
class USFDisciplineDefinition;
class UTexture2D;

// -----------------------------------------------------------------------------
// Weapon type enum — lives here as the class definition is the authority on
// what weapon types a class can use.
// -----------------------------------------------------------------------------

UENUM(BlueprintType, Meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ESFWeaponType : uint8
{
	None = 0			UMETA(Hidden),
	Sword = 1 << 0	UMETA(DisplayName = "Sword"),
	Greatsword = 1 << 1	UMETA(DisplayName = "Greatsword"),
	DualSword = 1 << 2	UMETA(DisplayName = "Dual Sword"),
	Twinblade = 1 << 3	UMETA(DisplayName = "Twinblade"),
	Rifle = 1 << 4	UMETA(DisplayName = "Rifle"),
	Shotgun = 1 << 5	UMETA(DisplayName = "Shotgun"),
	Minigun = 1 << 6	UMETA(DisplayName = "Minigun"),
	Pistol = 1 << 7	UMETA(DisplayName = "Pistol"),
};
ENUM_CLASS_FLAGS(ESFWeaponType);

// -----------------------------------------------------------------------------
// Starting equipment entry — item definition + quantity.
// Only used on fresh save. Player inventory takes over after first play.
// -----------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FSFStartingEquipmentEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<USFItemDefinition> ItemDefinition = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	int32 Quantity = 1;
};

// -----------------------------------------------------------------------------
// USFClassDefinition
// One data asset instance per class. Defines everything that makes a class
// distinct: abilities, stats, weapon permissions, visuals, and starting gear.
// -----------------------------------------------------------------------------

UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFClassDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// -------------------------------------------------------------------------
	// Identity
	// -------------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FName ClassName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FText ClassDisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FText ClassDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	TObjectPtr<UTexture2D> ClassIcon = nullptr;

	// -------------------------------------------------------------------------
	// Abilities
	//
	// Active abilities are granted to the player and bound to input slots.
	// Passive abilities are granted but run automatically with no input binding.
	// -------------------------------------------------------------------------

	/** Active abilities granted to the player on class assignment. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> ActiveAbilities;

	/**
	 * Passive abilities granted on class assignment.
	 * These have no input tag and run automatically or via internal triggers.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> PassiveAbilities;

	/**
 * Disciplines available to this class.
 * Unlocked at level 10. Player picks one to specialize in.
 * Each class should have 2-3 disciplines.
 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Disciplines")
	TArray<TObjectPtr<USFDisciplineDefinition>> AvailableDisciplines;

	/**
	 * The discipline automatically assigned at class selection (before level 10).
	 * Must be one of the entries in AvailableDisciplines.
	 * Players can switch at level 10 when not in combat.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Disciplines")
	TObjectPtr<USFDisciplineDefinition> DefaultDiscipline = nullptr;

	/** Level at which the player can switch disciplines. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Disciplines")
	int32 DisciplineUnlockLevel = 10;

	// -------------------------------------------------------------------------
	// Stats
	//
	// A single Gameplay Effect applied to the ASC on class assignment.
	// Use Scalable Float / Set By Caller modifiers inside the GE to define
	// per-class stat bonuses. Removed cleanly when the class changes.
	// -------------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	TSubclassOf<UGameplayEffect> ClassAttributeModifierEffect;

	// -------------------------------------------------------------------------
	// Weapons
	// -------------------------------------------------------------------------

	/**
	 * Bitmask of weapon types this class is allowed to equip.
	 * ESFWeaponType is defined in this file as the class is the authority
	 * on weapon permissions.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapons",
		Meta = (Bitmask, BitmaskEnum = "/Script/SignalForgeRPG.ESFWeaponType"))
	int32 AllowedWeaponTypes = 0;

	UFUNCTION(BlueprintPure, Category = "Weapons")
	bool IsWeaponTypeAllowed(ESFWeaponType WeaponType) const
	{
		return (AllowedWeaponTypes & static_cast<int32>(WeaponType)) != 0;
	}

	// -------------------------------------------------------------------------
	// Animation
	//
	// Base locomotion animation blueprint for this class.
	// Weapon-specific animation layers are applied separately by the
	// equipment system and should not be set here.
	// -------------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimBlueprint> LocomotionAnimBlueprint = nullptr;

	// -------------------------------------------------------------------------
	// Starting Equipment
	//
	// Only applied on a fresh save when the player has no existing inventory.
	// The player's inventory takes over after first play.
	// -------------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Starting Equipment")
	TArray<FSFStartingEquipmentEntry> StartingEquipment;

	// -------------------------------------------------------------------------
	// Data Validation
	// -------------------------------------------------------------------------

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	// -------------------------------------------------------------------------
	// UPrimaryDataAsset
	// -------------------------------------------------------------------------

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("SFClassDefinition", GetFName());
	}
};