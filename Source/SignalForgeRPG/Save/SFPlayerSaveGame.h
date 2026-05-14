#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Save/SFPlayerSaveTypes.h"
#include "SFPlayerSaveGame.generated.h"

/**
 * USFPlayerSaveGame
 *
 * USaveGame container for a single player slot. Mirrors the
 * USFNarrativeSaveGame pattern so the two systems are symmetric and a
 * future USFGameSaveService can compose both under one façade.
 *
 * Metadata (FriendlyName, SlotName, Timestamp, PlaytimeSeconds) lives
 * here rather than in FSFPlayerSaveData because the metadata is for
 * slot-browser UIs and shouldn't have a Schema migration story attached.
 * The actual gameplay payload (FSFPlayerSaveData) carries its own
 * SchemaVersion.
 */
UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFPlayerSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	USFPlayerSaveGame();

	/** Human-readable label shown in slot pickers ("Chapter 2 - Forge Outpost"). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Meta")
	FString FriendlyName;

	/** Slot identifier this save was last written to. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Meta")
	FString SlotName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Meta")
	int32 UserIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Meta")
	FDateTime SaveTimestamp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Meta")
	float AccumulatedPlaytimeSeconds = 0.0f;

	/** The actual player-state payload. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save")
	FSFPlayerSaveData PlayerData;
};
