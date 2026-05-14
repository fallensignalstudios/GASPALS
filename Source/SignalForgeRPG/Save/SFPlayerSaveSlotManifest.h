#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SFPlayerSaveSlotManifest.generated.h"

/**
 * USFPlayerSaveSlotManifest
 *
 * Tiny USaveGame whose only job is to remember which slot names this
 * user has created. UE's UGameplayStatics does NOT expose disk-level
 * slot enumeration -- DoesSaveGameExist requires you to already know
 * the name. The manifest is the canonical workaround: every successful
 * SaveToSlot registers its name here; every DeleteSlot deregisters.
 *
 * Stored under a fixed reserved slot name (see kManifestSlotName). The
 * UI never touches the manifest directly; it goes through
 * USFPlayerSaveService::GetAllSlotNames / RefreshSlotInfos.
 */
UCLASS()
class SIGNALFORGERPG_API USFPlayerSaveSlotManifest : public USaveGame
{
	GENERATED_BODY()

public:
	/** Slot names that have been written at least once and not since deleted. */
	UPROPERTY()
	TArray<FString> KnownSlotNames;
};
