#pragma once

#include "CoreMinimal.h"
#include "SFSlotTypes.generated.h"

UENUM(BlueprintType)
enum class ESFEquipmentSlot : uint8
{
	None,
	Head,
	Chest,
	Arms,
	Legs,
	Boots,
	PrimaryWeapon,
	SecondaryWeapon,
	HeavyWeapon
};