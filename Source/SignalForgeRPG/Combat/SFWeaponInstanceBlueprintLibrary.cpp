// Combat/SFWeaponInstanceBlueprintLibrary.cpp
#include "Combat/SFWeaponInstanceBlueprintLibrary.h"
#include "Combat/SFWeaponData.h"
#include "Combat/SFWeaponPerkDefinition.h"

FSFWeaponStatBlock USFWeaponInstanceBlueprintLibrary::ResolveEffectiveStats(const FSFWeaponInstanceData& InstanceData)
{
	FSFWeaponStatBlock Result;

	const USFWeaponData* WeaponDef = InstanceData.WeaponDefinition.LoadSynchronous();
	if (!WeaponDef)
	{
		return Result;
	}

	Result = WeaponDef->BaseStats;

	auto AddPerkStats = [&Result](const TSoftObjectPtr<USFWeaponPerkDefinition>& PerkPtr)
		{
			if (const USFWeaponPerkDefinition* Perk = PerkPtr.LoadSynchronous())
			{
				Result += Perk->StatModifiers;
			}
		};

	AddPerkStats(InstanceData.BarrelPerk);
	AddPerkStats(InstanceData.MagazinePerk);
	AddPerkStats(InstanceData.TraitColumn1Perk);
	AddPerkStats(InstanceData.TraitColumn2Perk);
	AddPerkStats(InstanceData.OriginTraitPerk);
	AddPerkStats(InstanceData.MasterworkPerk);

	Result += InstanceData.TuningData.ToStatBlock();
	Result.ClampTo(WeaponDef->MinStatCaps, WeaponDef->MaxStatCaps);

	return Result;
}