#include "Combat/SFWeaponPerkDefinition.h"
#include "Combat/SFWeaponData.h"

bool USFWeaponPerkDefinition::CanApplyToWeapon(const USFWeaponData* WeaponData) const
{
	if (!WeaponData)
	{
		return false;
	}

	if (!RequiredWeaponTags.IsEmpty() && !WeaponData->WeaponTags.HasAll(RequiredWeaponTags))
	{
		return false;
	}

	if (!BlockedWeaponTags.IsEmpty() && WeaponData->WeaponTags.HasAny(BlockedWeaponTags))
	{
		return false;
	}

	return true;
}