#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "SFAttributeSetBase.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class SIGNALFORGERPG_API USFAttributeSetBase : public UAttributeSet
{
	GENERATED_BODY()

public:
	USFAttributeSetBase();

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(USFAttributeSetBase, Health)

		UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(USFAttributeSetBase, MaxHealth)

		UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Echo;
	ATTRIBUTE_ACCESSORS(USFAttributeSetBase, Echo)

		UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData MaxEcho;
	ATTRIBUTE_ACCESSORS(USFAttributeSetBase, MaxEcho)

		UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Shields;
	ATTRIBUTE_ACCESSORS(USFAttributeSetBase, Shields)

		UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData MaxShields;
	ATTRIBUTE_ACCESSORS(USFAttributeSetBase, MaxShields)

		UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(USFAttributeSetBase, Stamina)

		UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(USFAttributeSetBase, MaxStamina)

		UPROPERTY(BlueprintReadOnly, Category = "Meta Attributes")
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS(USFAttributeSetBase, Damage)

		UPROPERTY(BlueprintReadOnly, Category = "Offense")
	FGameplayAttributeData AttackPower;
	ATTRIBUTE_ACCESSORS(USFAttributeSetBase, AttackPower)

		UPROPERTY(BlueprintReadOnly, Category = "Offense")
	FGameplayAttributeData AbilityPower;
	ATTRIBUTE_ACCESSORS(USFAttributeSetBase, AbilityPower)

		UPROPERTY(BlueprintReadOnly, Category = "Offense")
	FGameplayAttributeData CritChance;
	ATTRIBUTE_ACCESSORS(USFAttributeSetBase, CritChance)

		UPROPERTY(BlueprintReadOnly, Category = "Offense")
	FGameplayAttributeData CritMultiplier;
	ATTRIBUTE_ACCESSORS(USFAttributeSetBase, CritMultiplier)

		UPROPERTY(BlueprintReadOnly, Category = "Offense")
	FGameplayAttributeData WeakpointBonus;
	ATTRIBUTE_ACCESSORS(USFAttributeSetBase, WeakpointBonus)

		UPROPERTY(BlueprintReadOnly, Category = "Defense")
	FGameplayAttributeData Armor;
	ATTRIBUTE_ACCESSORS(USFAttributeSetBase, Armor)

		UPROPERTY(BlueprintReadOnly, Category = "Defense")
	FGameplayAttributeData DamageReduction;
	ATTRIBUTE_ACCESSORS(USFAttributeSetBase, DamageReduction)

		UPROPERTY(BlueprintReadOnly, Category = "Defense")
	FGameplayAttributeData DodgeChance;
	ATTRIBUTE_ACCESSORS(USFAttributeSetBase, DodgeChance)

		/** Guard meter used when blocking to absorb damage before health. */
		UPROPERTY(BlueprintReadOnly, Category = "Defense")
	FGameplayAttributeData Guard;
	ATTRIBUTE_ACCESSORS(USFAttributeSetBase, Guard)

		UPROPERTY(BlueprintReadOnly, Category = "Defense")
	FGameplayAttributeData MaxGuard;
	ATTRIBUTE_ACCESSORS(USFAttributeSetBase, MaxGuard)

		/** How resistant the character is to being staggered / broken. */
		UPROPERTY(BlueprintReadOnly, Category = "Defense")
	FGameplayAttributeData Poise;
	ATTRIBUTE_ACCESSORS(USFAttributeSetBase, Poise)

		/** How much poise damage this character's attacks deal. */
		UPROPERTY(BlueprintReadOnly, Category = "Offense")
	FGameplayAttributeData PoiseDamage;
	ATTRIBUTE_ACCESSORS(USFAttributeSetBase, PoiseDamage)

		UPROPERTY(BlueprintReadOnly, Category = "Utility")
	FGameplayAttributeData MoveSpeedMultiplier;
	ATTRIBUTE_ACCESSORS(USFAttributeSetBase, MoveSpeedMultiplier)

		UPROPERTY(BlueprintReadOnly, Category = "Utility")
	FGameplayAttributeData AttackSpeedMultiplier;
	ATTRIBUTE_ACCESSORS(USFAttributeSetBase, AttackSpeedMultiplier)

		UPROPERTY(BlueprintReadOnly, Category = "Utility")
	FGameplayAttributeData CooldownReduction;
	ATTRIBUTE_ACCESSORS(USFAttributeSetBase, CooldownReduction)
};