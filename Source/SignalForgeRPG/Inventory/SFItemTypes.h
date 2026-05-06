// Copyright Fallen Signal Studios LLC 2026. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "SFItemTypes.generated.h"

UENUM(BlueprintType)
enum class ESFItemType : uint8
{
	None UMETA(DisplayName = "None"),
	Weapon UMETA(DisplayName = "Weapon"),
	Armor UMETA(DisplayName = "Armor"),
	Consumable UMETA(DisplayName = "Consumable"),
	Quest UMETA(DisplayName = "Quest Item"),
	Resource UMETA(DisplayName = "Resource"),
	Misc UMETA(DisplayName = "Miscellaneous")
};