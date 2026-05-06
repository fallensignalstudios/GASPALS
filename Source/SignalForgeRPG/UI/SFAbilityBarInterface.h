// SFAbilityBarInterface.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SFAbilityBarInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class USFAbilityBarInterface : public UInterface
{
	GENERATED_BODY()
};

class SIGNALFORGERPG_API ISFAbilityBarInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "UI")
	void RefreshAbilityBar();
};