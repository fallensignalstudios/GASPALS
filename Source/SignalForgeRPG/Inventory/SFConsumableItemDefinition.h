#pragma once

#include "CoreMinimal.h"
#include "Inventory/SFItemDefinition.h"
#include "SFConsumableItemDefinition.generated.h"

class UGameplayEffect;
class USoundBase;
class UAnimMontage;
class UNiagaraSystem;

/**
 * USFConsumableItemDefinition
 *
 * Data-asset for items that the player USES from the inventory (health packs,
 * echo stims, buffs, food, throwables that fire off a GE on consume, etc.).
 *
 * Lives alongside USFItemDefinition rather than replacing it -- weapons,
 * gear, quest items still use the base class. Only items the player
 * actively "consumes" need this richer schema.
 *
 * Why a subclass instead of stuffing this on the base?
 *   - USFItemDefinition is already large; weapons / quest items don't need
 *     ConsumeEffect or cooldown bookkeeping.
 *   - bIsConsumable on the base is a flag-only signal for filters/UI; this
 *     subclass is what actually answers "what does using it do?".
 *   - Designers can search the picker for "ConsumableItem" specifically
 *     when authoring potions, stims, food, etc.
 *
 * Runtime contract:
 *   USFInventoryComponent::UseItem(EntryId) looks up the definition,
 *   checks for this subclass, validates cooldown, applies ConsumeEffect to
 *   the owning ASC, decrements quantity by AmountConsumedPerUse, and fires
 *   the OnItemUsed event. Designers don't write code -- they author a
 *   Data Asset + a GameplayEffect blueprint.
 */
UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFConsumableItemDefinition : public USFItemDefinition
{
	GENERATED_BODY()

public:
	USFConsumableItemDefinition();

	/**
	 * The GameplayEffect that fires on the user when they consume the item.
	 * Typically an Instant effect modifying Health, Echo, or another attribute
	 * by an additive amount (e.g. +50 Health). Can also be a Duration effect
	 * for buffs / regen-over-time. Required for the item to do anything.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable|Effect")
	TSubclassOf<UGameplayEffect> ConsumeEffect;

	/**
	 * GE level passed to MakeOutgoingSpec when applying ConsumeEffect. Lets
	 * designers reuse one effect class for tiered potions (Level 1 small pack,
	 * Level 2 medium, Level 3 large) by scaling the magnitude curve inside
	 * the effect.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable|Effect", meta = (ClampMin = "1.0"))
	float ConsumeEffectLevel = 1.0f;

	/**
	 * How many stack units are removed per successful use. Almost always 1.
	 * Set higher only for items that "burn through" multiple stack units on
	 * a single activation (e.g. spend 3 ammo on a single fire).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable|Use", meta = (ClampMin = "1"))
	int32 AmountConsumedPerUse = 1;

	/**
	 * Cooldown in seconds before the SAME item entry can be used again.
	 * Enforced per-entry against FSFInventoryEntry::LastUseTime. 0 = no
	 * cooldown. Note this is a per-entry cooldown, not a global item
	 * cooldown -- two different health packs are independent.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable|Use", meta = (ClampMin = "0.0"))
	float UseCooldownSeconds = 0.0f;

	/**
	 * If true, the use is refused while the target's current attribute is
	 * already at its max for the primary resource we care about. Useful so
	 * the player doesn't waste a Health Pack at full HP. Designers select
	 * which attribute via PrimaryRestoredAttributeTag; if no tag is set the
	 * check is skipped.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable|Use")
	bool bRefuseWhenAtFull = true;

	/**
	 * Identifies which attribute this consumable primarily restores so the
	 * "refuse at full" check knows what to look at. Match this against your
	 * attribute tags (e.g. Attribute.Health, Attribute.Echo). Optional --
	 * the inventory's UseItem path falls back to allowing the use if the
	 * tag isn't recognized.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable|Use")
	FGameplayTag PrimaryRestoredAttributeTag;

	// -------------------------------------------------------------------------
	// Cosmetic / feedback (all optional)
	// -------------------------------------------------------------------------

	/** Played 2D on consume. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable|Feedback")
	TSoftObjectPtr<USoundBase> UseSound;

	/** Played on the using character if their AnimInstance accepts it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable|Feedback")
	TSoftObjectPtr<UAnimMontage> UseMontage;

	/** Spawned at the character's location on use. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable|Feedback")
	TSoftObjectPtr<UNiagaraSystem> UseVfx;
};
