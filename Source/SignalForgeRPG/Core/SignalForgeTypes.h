#pragma once

#include "CoreMinimal.h"
#include "SignalForgeTypes.generated.h"

UENUM(BlueprintType)
enum class ESFCombatMode : uint8
{
	None	UMETA(DisplayName = "None"),
	Melee	UMETA(DisplayName = "Melee"),
	Ranged	UMETA(DisplayName = "Ranged"),
	Casting UMETA(DisplayName = "Casting")
};