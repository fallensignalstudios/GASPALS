#include "Classes/SFClassDefinition.h"

#include "Misc/DataValidation.h"
#include "AbilitySystem/SFGameplayAbility.h"

#if WITH_EDITOR

EDataValidationResult USFClassDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	// Identity
	if (ClassName.IsNone())
	{
		Context.AddError(FText::FromString(
			FString::Printf(TEXT("%s: ClassName is not set."), *GetName())));
		Result = EDataValidationResult::Invalid;
	}

	if (ClassDisplayName.IsEmpty())
	{
		Context.AddError(FText::FromString(
			FString::Printf(TEXT("%s: ClassDisplayName is not set."), *GetName())));
		Result = EDataValidationResult::Invalid;
	}

	if (ClassDescription.IsEmpty())
	{
		Context.AddWarning(FText::FromString(
			FString::Printf(TEXT("%s: ClassDescription is empty."), *GetName())));
	}

	if (ClassIcon == nullptr)
	{
		Context.AddWarning(FText::FromString(
			FString::Printf(TEXT("%s: ClassIcon is not set."), *GetName())));
	}

	// Disciplines
	if (AvailableDisciplines.IsEmpty())
	{
		Context.AddError(FText::FromString(
			FString::Printf(TEXT("%s: No AvailableDisciplines assigned."), *GetName())));
		Result = EDataValidationResult::Invalid;
	}

	if (DefaultDiscipline == nullptr)
	{
		Context.AddError(FText::FromString(
			FString::Printf(TEXT("%s: DefaultDiscipline is not set."), *GetName())));
		Result = EDataValidationResult::Invalid;
	}
	else if (!AvailableDisciplines.Contains(DefaultDiscipline))
	{
		Context.AddError(FText::FromString(
			FString::Printf(TEXT("%s: DefaultDiscipline is not in AvailableDisciplines."), *GetName())));
		Result = EDataValidationResult::Invalid;
	}

	// Abilities
	if (ActiveAbilities.IsEmpty())
	{
		Context.AddWarning(FText::FromString(
			FString::Printf(TEXT("%s: No ActiveAbilities assigned."), *GetName())));
	}

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : ActiveAbilities)
	{
		if (!AbilityClass)
		{
			Context.AddError(FText::FromString(
				FString::Printf(TEXT("%s: Null entry in ActiveAbilities."), *GetName())));
			Result = EDataValidationResult::Invalid;
		}
	}

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : PassiveAbilities)
	{
		if (!AbilityClass)
		{
			Context.AddError(FText::FromString(
				FString::Printf(TEXT("%s: Null entry in PassiveAbilities."), *GetName())));
			Result = EDataValidationResult::Invalid;
		}
	}

	// Stats
	if (!ClassAttributeModifierEffect)
	{
		Context.AddError(FText::FromString(
			FString::Printf(TEXT("%s: ClassAttributeModifierEffect is not set."), *GetName())));
		Result = EDataValidationResult::Invalid;
	}

	// Weapons
	if (AllowedWeaponTypes == 0)
	{
		Context.AddError(FText::FromString(
			FString::Printf(TEXT("%s: AllowedWeaponTypes is empty — class cannot equip any weapon."), *GetName())));
		Result = EDataValidationResult::Invalid;
	}

	// Animation
	if (!LocomotionAnimBlueprint)
	{
		Context.AddWarning(FText::FromString(
			FString::Printf(TEXT("%s: LocomotionAnimBlueprint is not set."), *GetName())));
	}

	// Starting Equipment
	for (const FSFStartingEquipmentEntry& Entry : StartingEquipment)
	{
		if (!Entry.ItemDefinition)
		{
			Context.AddError(FText::FromString(
				FString::Printf(TEXT("%s: Null ItemDefinition in StartingEquipment."), *GetName())));
			Result = EDataValidationResult::Invalid;
		}

		if (Entry.Quantity <= 0)
		{
			Context.AddError(FText::FromString(
				FString::Printf(TEXT("%s: StartingEquipment entry has Quantity <= 0."), *GetName())));
			Result = EDataValidationResult::Invalid;
		}
	}

	return Result;
}

#endif