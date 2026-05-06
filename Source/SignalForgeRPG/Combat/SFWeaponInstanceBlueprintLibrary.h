// Combat/SFWeaponInstanceBlueprintLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Combat/SFWeaponInstanceTypes.h"
#include "SFWeaponInstanceBlueprintLibrary.generated.h"

UCLASS()
class SIGNALFORGERPG_API USFWeaponInstanceBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weapon")
	static FSFWeaponStatBlock ResolveEffectiveStats(const FSFWeaponInstanceData& InstanceData);
};