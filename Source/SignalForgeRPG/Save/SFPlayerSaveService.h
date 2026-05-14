#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Save/SFPlayerSaveTypes.h"
#include "SFPlayerSaveService.generated.h"

class ASFPlayerCharacter;
class ASFCharacterBase;
class USFPlayerSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSFOnPlayerSaveSlotEvent, const FString&, SlotName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSFOnPlayerSaveSlotResultEvent, const FString&, SlotName, bool, bSuccess);

/**
 * USFPlayerSaveService
 *
 * GameInstanceSubsystem responsible for persisting the player's level,
 * XP, attributes, transform, inventory, and ammo reserves to a named
 * slot, and reapplying that state on load.
 *
 * Why a GameInstanceSubsystem (rather than a UObject like the narrative
 * service):
 *   - Stable lifetime across PIE / packaged builds; auto-instantiated.
 *   - Discoverable from anywhere: GetGameInstance()->GetSubsystem<>().
 *   - Survives map travel, which is critical because LoadFromSlot will
 *     typically be called BEFORE the destination map is loaded.
 *
 * Why this is separate from USFNarrativeSaveService:
 *   - Narrative state (facts, factions, quests, dialogue) is its own
 *     well-isolated subsystem and already has a save service. Stuffing
 *     player state into the narrative payload would couple two systems
 *     that should be able to ship and iterate independently.
 *   - A future USFGameSaveService façade can call both in sequence; the
 *     two slots use parallel filenames (e.g. "Slot01_Player" and
 *     "Slot01_Narrative") so they round-trip together.
 *
 * Save/load is synchronous on the main thread. UE's SaveGameToSlot does
 * the I/O on a worker internally but blocks the caller until done; for
 * a single-player RPG saving < 1 MB this is fine. Switch to the async
 * variants if save sizes balloon.
 */
UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFPlayerSaveService : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// -------------------------------------------------------------------------
	// Slot operations
	// -------------------------------------------------------------------------

	/**
	 * Capture the current player state and write it to disk under SlotName.
	 *
	 * Resolves the player character via:
	 *   1. InPlayer if non-null (caller picks)
	 *   2. UGameplayStatics::GetPlayerCharacter(this, UserIndex) cast to
	 *      ASFCharacterBase
	 *
	 * Returns false if no usable player can be resolved or if disk I/O
	 * fails. Broadcasts OnBeforeSave (always) and OnAfterSave (always,
	 * with success flag) so UI can show / hide a saving indicator.
	 */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool SaveToSlot(const FString& SlotName, ASFCharacterBase* InPlayer = nullptr, int32 UserIndex = 0);

	/**
	 * Read SlotName off disk and apply it to the current player. The pawn
	 * must already exist -- this is "in-place restore", not "travel and
	 * spawn". Typical flow:
	 *   1. UI calls OpenLevel(SaveData.LevelName) for the destination map
	 *   2. After PIE / packaged map load completes, call LoadFromSlot
	 *   3. Service reads the slot, re-places the pawn, and pushes state
	 *
	 * Returns false on missing slot, schema mismatch, or apply failure.
	 */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool LoadFromSlot(const FString& SlotName, ASFCharacterBase* InPlayer = nullptr, int32 UserIndex = 0);

	UFUNCTION(BlueprintCallable, Category = "Save")
	bool DeleteSlot(const FString& SlotName, int32 UserIndex = 0);

	UFUNCTION(BlueprintPure, Category = "Save")
	bool DoesSlotExist(const FString& SlotName, int32 UserIndex = 0) const;

	/**
	 * Peek at metadata WITHOUT applying the save. Useful for slot
	 * pickers / continue screens. Returns nullptr if the slot is missing
	 * or corrupt. Caller owns nothing -- the returned USFPlayerSaveGame
	 * is a fresh transient UObject.
	 */
	UFUNCTION(BlueprintCallable, Category = "Save")
	USFPlayerSaveGame* PeekSlot(const FString& SlotName, int32 UserIndex = 0) const;

	// -------------------------------------------------------------------------
	// Slot enumeration (backed by USFPlayerSaveSlotManifest)
	// -------------------------------------------------------------------------

	/**
	 * Names of every slot this user has written at least once and not since
	 * deleted. Backed by a tiny manifest USaveGame (UE has no built-in disk
	 * enumeration). Order is creation-order; sort in the UI if you want
	 * recency.
	 */
	UFUNCTION(BlueprintCallable, Category = "Save")
	TArray<FString> GetAllSlotNames(int32 UserIndex = 0) const;

	/**
	 * Load header metadata for every known slot. Each entry's bIsValid is
	 * true if the slot loaded cleanly, false if missing/corrupt (in which
	 * case only SlotName is meaningful). Use this to populate slot-browser
	 * UI without forcing a full payload load per slot.
	 */
	UFUNCTION(BlueprintCallable, Category = "Save")
	TArray<FSFPlayerSaveSlotInfo> GetAllSlotInfos(int32 UserIndex = 0) const;

	UFUNCTION(BlueprintCallable, Category = "Save")
	FSFPlayerSaveSlotInfo GetSlotInfo(const FString& SlotName, int32 UserIndex = 0) const;

	/** Broadcast whenever the known-slot list changes (save / delete). */
	UPROPERTY(BlueprintAssignable, Category = "Save")
	FSFOnPlayerSaveSlotEvent OnSlotListChanged;

	// -------------------------------------------------------------------------
	// Build / apply (exposed for testing + composition with future
	// world-state save services that want to manage their own slot file)
	// -------------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Save")
	FSFPlayerSaveData BuildSaveData(ASFCharacterBase* InPlayer) const;

	UFUNCTION(BlueprintCallable, Category = "Save")
	bool ApplySaveData(const FSFPlayerSaveData& Data, ASFCharacterBase* InPlayer);

	UFUNCTION(BlueprintPure, Category = "Save")
	int32 GetCurrentSchemaVersion() const { return CurrentSchemaVersion; }

	/**
	 * Bring an older payload up to CurrentSchemaVersion. Returns false if
	 * the payload is fundamentally incompatible (e.g. from a newer build
	 * that we can't read). No-op when SchemaVersion already matches.
	 */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool MigrateSaveData(UPARAM(ref) FSFPlayerSaveData& InOutData) const;

	// -------------------------------------------------------------------------
	// Delegates -- UI binds here for "Saving..." / "Loading..." indicators
	// -------------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Save")
	FSFOnPlayerSaveSlotEvent OnBeforeSave;

	UPROPERTY(BlueprintAssignable, Category = "Save")
	FSFOnPlayerSaveSlotResultEvent OnAfterSave;

	UPROPERTY(BlueprintAssignable, Category = "Save")
	FSFOnPlayerSaveSlotEvent OnBeforeLoad;

	UPROPERTY(BlueprintAssignable, Category = "Save")
	FSFOnPlayerSaveSlotResultEvent OnAfterLoad;

protected:
	/** Resolve the player character, falling back to UGameplayStatics if null. */
	ASFCharacterBase* ResolvePlayer(ASFCharacterBase* InPlayer, int32 UserIndex) const;

	/** Reserved slot name used by the slot manifest. Never written by user code. */
	static const FString ManifestSlotName;

	/** Read the manifest off disk (or return a fresh empty one). */
	class USFPlayerSaveSlotManifest* LoadOrCreateManifest(int32 UserIndex) const;

	/** Write the manifest. Returns false on disk failure. */
	bool WriteManifest(class USFPlayerSaveSlotManifest* Manifest, int32 UserIndex) const;

	/** Add SlotName to the manifest if absent and write it back. */
	void RegisterSlotInManifest(const FString& SlotName, int32 UserIndex);

	/** Remove SlotName from the manifest if present and write it back. */
	void DeregisterSlotFromManifest(const FString& SlotName, int32 UserIndex);

	UPROPERTY(EditDefaultsOnly, Category = "Save")
	int32 CurrentSchemaVersion = 1;
};
