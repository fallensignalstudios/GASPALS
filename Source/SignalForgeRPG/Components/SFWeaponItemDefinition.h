//#pragma once
//
//#include "CoreMinimal.h"
//#include "Inventory/SFItemDefinition.h"
//#include "Inventory/SFSlotTypes.h"
//#include "Misc/DataValidation.h"
//#include "SFWeaponItemDefinition.generated.h"
//
//UCLASS(BlueprintType)
//class SIGNALFORGERPG_API USFWeaponItemDefinition : public USFItemDefinition
//{
//	GENERATED_BODY()
//
//public:
//	USFWeaponItemDefinition()
//	{
//		ItemType = ESFItemType::Weapon;
//		bStackable = false;
//		bUniqueInstance = true;
//		MaxStackSize = 1;
//	}
//
//	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
//	ESFEquipmentSlot PreferredWeaponSlot = ESFEquipmentSlot::PrimaryWeapon;
//
//	UFUNCTION(BlueprintPure, Category = "Weapon")
//	bool IsWeaponDefinitionValid() const
//	{
//		return IsWeapon()
//			&& HasWeaponData()
//			&& !CanStack()
//			&& GetEffectiveMaxStackSize() == 1
//			&& (PreferredWeaponSlot == ESFEquipmentSlot::PrimaryWeapon
//				|| PreferredWeaponSlot == ESFEquipmentSlot::SecondaryWeapon
//				|| PreferredWeaponSlot == ESFEquipmentSlot::HeavyWeapon);
//	}
//
//#if WITH_EDITOR
//	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override
//	{
//		const EDataValidationResult SuperResult = Super::IsDataValid(Context);
//		bool bHasErrors = (SuperResult == EDataValidationResult::Invalid);
//		bool bHasWarnings = false;
//
//		if (!HasWeaponData())
//		{
//			Context.AddError(FText::FromString(TEXT("WeaponData must be assigned.")));
//			bHasErrors = true;
//		}
//
//		if (!IsWeapon())
//		{
//			Context.AddError(FText::FromString(TEXT("Weapon item definitions must use ItemType = Weapon.")));
//			bHasErrors = true;
//		}
//
//		if (bStackable)
//		{
//			Context.AddError(FText::FromString(TEXT("Weapon items must not be stackable.")));
//			bHasErrors = true;
//		}
//
//		if (!bUniqueInstance)
//		{
//			Context.AddWarning(FText::FromString(TEXT("Weapon items usually should be unique instances.")));
//			bHasWarnings = true;
//		}
//
//		if (MaxStackSize != 1)
//		{
//			Context.AddWarning(FText::FromString(TEXT("Weapon items should use MaxStackSize = 1.")));
//			bHasWarnings = true;
//		}
//
//		if (PreferredWeaponSlot != ESFEquipmentSlot::PrimaryWeapon
//			&& PreferredWeaponSlot != ESFEquipmentSlot::SecondaryWeapon
//			&& PreferredWeaponSlot != ESFEquipmentSlot::HeavyWeapon)
//		{
//			Context.AddError(FText::FromString(TEXT("PreferredWeaponSlot must be a weapon slot.")));
//			bHasErrors = true;
//		}
//
//		if (bHasErrors)
//		{
//			return EDataValidationResult::Invalid;
//		}
//
//		return bHasWarnings ? EDataValidationResult::Valid : SuperResult;
//	}
//#endif
//};