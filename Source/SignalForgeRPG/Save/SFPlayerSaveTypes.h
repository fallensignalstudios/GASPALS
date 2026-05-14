#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Combat/SFWeaponInstanceTypes.h"
#include "SFPlayerSaveTypes.generated.h"

class USFItemDefinition;
class USFAmmoType;

/**
 * Serializable mirror of FSFInventoryEntry.
 *
 * We do NOT save a raw TObjectPtr<USFItemDefinition> as the primary
 * reference -- hard pointers don't tolerate the item asset being renamed
 * or moved between save and load. We use a TSoftObjectPtr (asset path)
 * with FName ItemId as a fallback resolver. On load, we look up the item
 * by soft pointer first; if that returns null (asset moved), we fall back
 * to scanning the asset registry by ItemId.
 *
 * Per-entry runtime state (EntryId, LastUseTime, durability, item level,
 * favorite/equipped flags, instance tags, weapon roll data) is preserved
 * verbatim so consumable cooldowns survive a quicksave/quickload and so
 * an equipped weapon stays the SAME weapon (same EntryId) across loads.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFPlayerSaveInventoryEntry
{
	GENERATED_BODY()

	/** Soft pointer to the item definition asset. Primary resolution path. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Inventory")
	TSoftObjectPtr<USFItemDefinition> ItemDefinition;

	/**
	 * Stable item id (USFItemDefinition::ItemId). Fallback resolver if the
	 * soft pointer cannot be resolved (asset was moved without redirector).
	 * Saved alongside the soft pointer so load is robust to asset shuffling.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Inventory")
	FName ItemId = NAME_None;

	/** Preserves stable runtime identity across save/load (e.g. for equipped weapon). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Inventory")
	FGuid EntryId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Inventory")
	int32 Quantity = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Inventory")
	int32 CurrentDurability = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Inventory")
	int32 ItemLevel = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Inventory")
	bool bFavorite = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Inventory")
	bool bEquipped = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Inventory")
	FGameplayTagContainer InstanceTags;

	/**
	 * Per-entry runtime cooldown marker for consumables. World time when the
	 * item was last used. Stored as relative-to-zero so loading into a fresh
	 * world (TimeSeconds == 0) yields a sensible "long-ago" comparison.
	 *
	 * We persist the RELATIVE offset (LastUseTime - SaveWorldTime). On load,
	 * the service adds the LOAD world time back, so a 3-second-old cooldown
	 * remains 3 seconds old regardless of how long the player was away.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Inventory")
	float LastUseTimeOffset = -FLT_MAX;

	/** Weapon instance data (ammo-in-clip, rolled mods, etc.). Plain copy. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Inventory")
	FSFWeaponInstanceData WeaponInstanceData;
};

USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFPlayerSaveAmmoEntry
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Ammo")
	TSoftObjectPtr<USFAmmoType> AmmoType;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Ammo")
	int32 Count = 0;
};

/**
 * Player attribute snapshot. We save BOTH current and max values so a
 * load can detect "the max has gone up since save (designer rebalanced)"
 * cases and either clamp or refill rather than blindly restoring a value
 * larger than the current max.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFPlayerSaveAttributes
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Attributes")
	float Health = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Attributes")
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Attributes")
	float Echo = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Attributes")
	float MaxEcho = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Attributes")
	float Shields = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Attributes")
	float MaxShields = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Attributes")
	float Stamina = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Attributes")
	float MaxStamina = 100.0f;
};

/**
 * FSFPlayerSaveData
 *
 * Top-level payload stored inside USFPlayerSaveGame. Everything needed to
 * restore "where the player was, what they had, and how they were doing"
 * lives here.
 *
 * Deliberately NOT in here:
 *   - World narrative state (facts, factions, quests) -- that's the
 *     narrative save service's domain. The two saves are independent so
 *     each can iterate without breaking the other.
 *   - Enemy positions / dead-NPC list -- world-state save is a future
 *     feature. For now, loading respawns enemies (acceptable for now).
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFPlayerSaveData
{
	GENERATED_BODY()

	/**
	 * Bumped on incompatible schema changes. Load path refuses payloads
	 * whose SchemaVersion exceeds CurrentSchemaVersion (newer save in
	 * older build) and runs migrations for older versions.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save")
	int32 SchemaVersion = 1;

	/** Level name the save was taken in. Loaded as a hint for the load flow. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|World")
	FName LevelName = NAME_None;

	/** Player transform at save time. Used to re-place the pawn after travel. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|World")
	FTransform PlayerTransform;

	/** Camera/control rotation at save time (for first-person yaw recovery). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|World")
	FRotator ControlRotation = FRotator::ZeroRotator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Progression")
	int32 Level = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Progression")
	int32 XP = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Progression")
	FSFPlayerSaveAttributes Attributes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Inventory")
	TArray<FSFPlayerSaveInventoryEntry> InventoryEntries;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|Inventory")
	TArray<FSFPlayerSaveAmmoEntry> AmmoReserves;

	/**
	 * World time (UGameplayStatics::GetTimeSeconds) at save creation. Used
	 * to translate relative cooldown offsets into absolute LastUseTime
	 * values during apply.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save")
	float SaveWorldTimeSeconds = 0.0f;
};

/**
 * FSFPlayerSaveSlotInfo
 *
 * Display-friendly metadata bundle for a single save slot. Built by
 * USFPlayerSaveService::GetSlotInfo by loading the slot just far enough
 * to read its metadata header, NOT the full payload. UI widgets bind to
 * arrays of this struct to render slot pickers.
 *
 * If bIsValid is false, the slot is missing or corrupt -- only SlotName
 * is meaningful (caller may still want to render it as "<corrupt save>").
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFPlayerSaveSlotInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Save|Slot")
	FString SlotName;

	UPROPERTY(BlueprintReadOnly, Category = "Save|Slot")
	FString FriendlyName;

	UPROPERTY(BlueprintReadOnly, Category = "Save|Slot")
	FDateTime SaveTimestamp;

	UPROPERTY(BlueprintReadOnly, Category = "Save|Slot")
	float AccumulatedPlaytimeSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Save|Slot")
	FName LevelName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Save|Slot")
	int32 PlayerLevel = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Save|Slot")
	int32 PlayerXP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Save|Slot")
	int32 SchemaVersion = 0;

	/** False if the slot was missing, mistyped, or failed to load. */
	UPROPERTY(BlueprintReadOnly, Category = "Save|Slot")
	bool bIsValid = false;
};
