// SignalForgeDeveloperSettings.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SignalForgeDeveloperSettings.generated.h"

class USFFactionRelationshipData;

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

	/**
	 * Global faction relationship matrix used by USFFactionStatics. Optional.
	 * If unset, USFFactionStatics falls back to: same-tag = Friendly, different = Neutral.
	 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Faction")
	TSoftObjectPtr<USFFactionRelationshipData> DefaultFactionRelationships;

	/** Synchronous resolver for editor / runtime convenience. May return nullptr. */
	const USFFactionRelationshipData* GetDefaultFactionRelationships() const;
};