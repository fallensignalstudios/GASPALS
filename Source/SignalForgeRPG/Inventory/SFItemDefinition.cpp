#include "Inventory/SFItemDefinition.h"
#include "Combat/SFWeaponData.h"


USFWeaponData* USFItemDefinition::GetWeaponData() const
{
	return WeaponData.IsNull() ? nullptr : WeaponData.LoadSynchronous();
}