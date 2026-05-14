// SignalForgeGameInstance.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SignalForgeGameInstance.generated.h"

UCLASS()
class SIGNALFORGERPG_API USignalForgeGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	// -------------------------------------------------------------------------
	// Pending save-load (survives map travel)
	//
	// When the main menu / load-game UI picks a save, the player needs to
	// travel to the saved level FIRST, then have the slot applied AFTER the
	// new map finishes loading. We can't keep state on a widget across
	// OpenLevel, so the request is parked here on the game instance and the
	// destination map's GameMode/PlayerController consumes it on BeginPlay
	// via USFPlayerSaveService::ConsumePendingLoadAndApply.
	// -------------------------------------------------------------------------

	/** Slot name that should be applied to the player after the next map load. */
	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FString PendingLoadSlotName;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	int32 PendingLoadUserIndex = 0;

	UFUNCTION(BlueprintCallable, Category = "Save")
	void SetPendingLoad(const FString& InSlotName, int32 InUserIndex)
	{
		PendingLoadSlotName = InSlotName;
		PendingLoadUserIndex = InUserIndex;
	}

	UFUNCTION(BlueprintCallable, Category = "Save")
	void ClearPendingLoad()
	{
		PendingLoadSlotName.Reset();
		PendingLoadUserIndex = 0;
	}

	UFUNCTION(BlueprintPure, Category = "Save")
	bool HasPendingLoad() const { return !PendingLoadSlotName.IsEmpty(); }
};