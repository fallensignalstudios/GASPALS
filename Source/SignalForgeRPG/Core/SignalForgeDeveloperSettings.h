// SignalForgeDeveloperSettings.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SignalForgeDeveloperSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Signal Forge"))
class SIGNALFORGERPG_API USignalForgeDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	USignalForgeDeveloperSettings();

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<class UInputMappingContext> DefaultInputMappingContext;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Framework")
	int32 DefaultPlayerLevel = 1;
};