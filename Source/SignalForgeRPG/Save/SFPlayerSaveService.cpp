#include "Save/SFPlayerSaveService.h"

#include "Save/SFPlayerSaveGame.h"
#include "Save/SFPlayerSaveSlotManifest.h"
#include "Core/SignalForgeGameInstance.h"
#include "Characters/SFCharacterBase.h"
#include "Components/SFInventoryComponent.h"
#include "Components/SFAmmoReserveComponent.h"
#include "Components/SFProgressionComponent.h"
#include "Core/SFAttributeSetBase.h"
#include "Inventory/SFItemDefinition.h"
#include "Combat/SFAmmoType.h"
#include "Combat/SFWeaponInstanceTypes.h"
#include "AbilitySystemComponent.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectIterator.h"

// =============================================================================
// Logging
// =============================================================================

DEFINE_LOG_CATEGORY_STATIC(LogSFPlayerSave, Log, All);

// Reserved manifest slot. Reserves a slot name in the save dir; user code
// must not collide with it. The leading underscore is illegal in many UI
// flows so it stays out of accidental user-typed slot names.
const FString USFPlayerSaveService::ManifestSlotName = TEXT("_SFPlayerSaveSlotManifest");

// =============================================================================
// Local helpers
// =============================================================================

namespace SFPlayerSaveLocal
{
	/**
	 * Resolve a soft-pointer item reference, with FName-based fallback for
	 * the case where the asset was renamed/moved without a redirector.
	 *
	 * The fallback iterates loaded USFItemDefinition instances. This works
	 * because item defs are UDataAssets that get loaded en-masse by the
	 * primary-asset system (or by the gameplay code that referenced them
	 * before the save was taken). If the asset isn't loaded yet, the
	 * fallback returns null and the entry is dropped on load -- acceptable
	 * for a fallback. Future hardening: ask the asset manager to load all
	 * USFItemDefinition primary assets before applying the save.
	 */
	USFItemDefinition* ResolveItemDefinition(const TSoftObjectPtr<USFItemDefinition>& SoftRef, FName ItemId)
	{
		if (USFItemDefinition* SoftResolved = SoftRef.LoadSynchronous())
		{
			return SoftResolved;
		}

		if (ItemId.IsNone())
		{
			return nullptr;
		}

		for (TObjectIterator<USFItemDefinition> It; It; ++It)
		{
			USFItemDefinition* Candidate = *It;
			if (!Candidate || Candidate->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
			{
				continue;
			}
			if (Candidate->ItemId == ItemId)
			{
				return Candidate;
			}
		}

		return nullptr;
	}

	/** Mirror an FSFInventoryEntry into its serializable counterpart. */
	FSFPlayerSaveInventoryEntry MakeSaveEntry(const FSFInventoryEntry& Source, float SaveWorldTime)
	{
		FSFPlayerSaveInventoryEntry Out;
		Out.ItemDefinition = TSoftObjectPtr<USFItemDefinition>(Source.ItemDefinition.Get());
		Out.ItemId = Source.ItemDefinition ? Source.ItemDefinition->ItemId : NAME_None;
		Out.EntryId = Source.EntryId;
		Out.Quantity = Source.Quantity;
		Out.CurrentDurability = Source.CurrentDurability;
		Out.ItemLevel = Source.ItemLevel;
		Out.bFavorite = Source.bFavorite;
		Out.bEquipped = Source.bEquipped;
		Out.InstanceTags = Source.InstanceTags;
		Out.WeaponInstanceData = Source.WeaponInstanceData;

		// Translate absolute world-time-based LastUseTime into a relative
		// offset from save world time. The sentinel "never used" value
		// (-FLT_MAX) is preserved verbatim so it survives round-tripping.
		if (Source.LastUseTime <= -FLT_MAX * 0.5f)
		{
			Out.LastUseTimeOffset = -FLT_MAX;
		}
		else
		{
			Out.LastUseTimeOffset = Source.LastUseTime - SaveWorldTime;
		}

		return Out;
	}

	/** Inverse of MakeSaveEntry; resolves the soft pointer + fallback. */
	bool TryMakeRuntimeEntry(const FSFPlayerSaveInventoryEntry& Source, float LoadWorldTime, FSFInventoryEntry& OutEntry)
	{
		USFItemDefinition* ResolvedDef = ResolveItemDefinition(Source.ItemDefinition, Source.ItemId);
		if (!ResolvedDef)
		{
			UE_LOG(LogSFPlayerSave, Warning,
				TEXT("Save: could not resolve item definition (ItemId=%s) -- entry dropped."),
				*Source.ItemId.ToString());
			return false;
		}

		OutEntry = FSFInventoryEntry();
		OutEntry.EntryId = Source.EntryId.IsValid() ? Source.EntryId : FGuid::NewGuid();
		OutEntry.ItemDefinition = ResolvedDef;
		OutEntry.Quantity = FMath::Max(1, Source.Quantity);
		OutEntry.CurrentDurability = Source.CurrentDurability;
		OutEntry.ItemLevel = FMath::Max(1, Source.ItemLevel);
		OutEntry.bFavorite = Source.bFavorite;
		OutEntry.bEquipped = Source.bEquipped;
		OutEntry.InstanceTags = Source.InstanceTags;
		OutEntry.WeaponInstanceData = Source.WeaponInstanceData;

		if (Source.LastUseTimeOffset <= -FLT_MAX * 0.5f)
		{
			OutEntry.LastUseTime = -FLT_MAX;
		}
		else
		{
			OutEntry.LastUseTime = LoadWorldTime + Source.LastUseTimeOffset;
		}

		return true;
	}
}

// =============================================================================
// Slot operations
// =============================================================================

bool USFPlayerSaveService::SaveToSlot(const FString& SlotName, ASFCharacterBase* InPlayer, int32 UserIndex)
{
	OnBeforeSave.Broadcast(SlotName);

	if (SlotName.IsEmpty())
	{
		UE_LOG(LogSFPlayerSave, Warning, TEXT("SaveToSlot: empty slot name."));
		OnAfterSave.Broadcast(SlotName, false);
		return false;
	}

	ASFCharacterBase* Player = ResolvePlayer(InPlayer, UserIndex);
	if (!Player)
	{
		UE_LOG(LogSFPlayerSave, Warning, TEXT("SaveToSlot[%s]: no player to capture."), *SlotName);
		OnAfterSave.Broadcast(SlotName, false);
		return false;
	}

	USFPlayerSaveGame* SaveObject =
		Cast<USFPlayerSaveGame>(UGameplayStatics::CreateSaveGameObject(USFPlayerSaveGame::StaticClass()));
	if (!SaveObject)
	{
		UE_LOG(LogSFPlayerSave, Error, TEXT("SaveToSlot[%s]: failed to instantiate USFPlayerSaveGame."), *SlotName);
		OnAfterSave.Broadcast(SlotName, false);
		return false;
	}

	SaveObject->SlotName = SlotName;
	SaveObject->UserIndex = UserIndex;
	SaveObject->SaveTimestamp = FDateTime::UtcNow();
	SaveObject->FriendlyName = SlotName;
	SaveObject->PlayerData = BuildSaveData(Player);

	const bool bWrote = UGameplayStatics::SaveGameToSlot(SaveObject, SlotName, UserIndex);
	UE_LOG(LogSFPlayerSave, Log, TEXT("SaveToSlot[%s]: %s"), *SlotName, bWrote ? TEXT("OK") : TEXT("FAILED"));

	if (bWrote)
	{
		RegisterSlotInManifest(SlotName, UserIndex);
	}

	OnAfterSave.Broadcast(SlotName, bWrote);
	return bWrote;
}

bool USFPlayerSaveService::LoadFromSlot(const FString& SlotName, ASFCharacterBase* InPlayer, int32 UserIndex)
{
	OnBeforeLoad.Broadcast(SlotName);

	if (SlotName.IsEmpty() || !UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		UE_LOG(LogSFPlayerSave, Warning, TEXT("LoadFromSlot[%s]: slot missing."), *SlotName);
		OnAfterLoad.Broadcast(SlotName, false);
		return false;
	}

	USFPlayerSaveGame* SaveObject =
		Cast<USFPlayerSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
	if (!SaveObject)
	{
		UE_LOG(LogSFPlayerSave, Warning, TEXT("LoadFromSlot[%s]: load returned null or wrong type."), *SlotName);
		OnAfterLoad.Broadcast(SlotName, false);
		return false;
	}

	FSFPlayerSaveData Data = SaveObject->PlayerData;
	if (!MigrateSaveData(Data))
	{
		UE_LOG(LogSFPlayerSave, Warning,
			TEXT("LoadFromSlot[%s]: payload schema %d incompatible with current %d."),
			*SlotName, Data.SchemaVersion, CurrentSchemaVersion);
		OnAfterLoad.Broadcast(SlotName, false);
		return false;
	}

	ASFCharacterBase* Player = ResolvePlayer(InPlayer, UserIndex);
	if (!Player)
	{
		UE_LOG(LogSFPlayerSave, Warning, TEXT("LoadFromSlot[%s]: no player to apply to."), *SlotName);
		OnAfterLoad.Broadcast(SlotName, false);
		return false;
	}

	const bool bApplied = ApplySaveData(Data, Player);
	UE_LOG(LogSFPlayerSave, Log, TEXT("LoadFromSlot[%s]: %s"), *SlotName, bApplied ? TEXT("OK") : TEXT("FAILED"));

	OnAfterLoad.Broadcast(SlotName, bApplied);
	return bApplied;
}

bool USFPlayerSaveService::BeginLoadFromSlot(const FString& SlotName, int32 UserIndex)
{
	// Travel-then-apply path. Used from contexts where the player pawn does
	// not exist yet (main menu, level-select). Peek the slot to learn the
	// destination map, park the request on the GameInstance, and OpenLevel.
	// The destination map's GameMode/PC calls ConsumePendingLoadAndApply on
	// BeginPlay to finish the job.
	if (SlotName.IsEmpty() || !DoesSlotExist(SlotName, UserIndex))
	{
		UE_LOG(LogSFPlayerSave, Warning, TEXT("BeginLoadFromSlot[%s]: slot missing."), *SlotName);
		return false;
	}

	USFPlayerSaveGame* SaveObject = PeekSlot(SlotName, UserIndex);
	if (!SaveObject || SaveObject->PlayerData.LevelName.IsNone())
	{
		UE_LOG(LogSFPlayerSave, Warning,
			TEXT("BeginLoadFromSlot[%s]: missing or empty LevelName; refusing to travel."), *SlotName);
		return false;
	}

	USignalForgeGameInstance* GI = Cast<USignalForgeGameInstance>(GetGameInstance());
	if (!GI)
	{
		UE_LOG(LogSFPlayerSave, Warning,
			TEXT("BeginLoadFromSlot[%s]: owning GameInstance is not USignalForgeGameInstance."), *SlotName);
		return false;
	}

	const FName DestLevel = SaveObject->PlayerData.LevelName;
	GI->SetPendingLoad(SlotName, UserIndex);

	UE_LOG(LogSFPlayerSave, Log,
		TEXT("BeginLoadFromSlot[%s]: parking request and opening level %s."),
		*SlotName, *DestLevel.ToString());

	UGameplayStatics::OpenLevel(this, DestLevel);
	return true;
}

bool USFPlayerSaveService::ConsumePendingLoadAndApply(ASFCharacterBase* InPlayer)
{
	USignalForgeGameInstance* GI = Cast<USignalForgeGameInstance>(GetGameInstance());
	if (!GI || !GI->HasPendingLoad())
	{
		return false;
	}

	// Snapshot, then clear FIRST so a failure can't loop us back on the next
	// BeginPlay. The slot's level might be the same level we're already in.
	const FString SlotName = GI->PendingLoadSlotName;
	const int32 UserIndex = GI->PendingLoadUserIndex;
	GI->ClearPendingLoad();

	UE_LOG(LogSFPlayerSave, Log,
		TEXT("ConsumePendingLoadAndApply[%s]: applying parked load."), *SlotName);

	return LoadFromSlot(SlotName, InPlayer, UserIndex);
}

bool USFPlayerSaveService::DeleteSlot(const FString& SlotName, int32 UserIndex)
{
	if (SlotName.IsEmpty() || SlotName == ManifestSlotName)
	{
		return false;
	}

	const bool bDeleted = UGameplayStatics::DeleteGameInSlot(SlotName, UserIndex);
	if (bDeleted)
	{
		DeregisterSlotFromManifest(SlotName, UserIndex);
	}
	return bDeleted;
}

bool USFPlayerSaveService::DoesSlotExist(const FString& SlotName, int32 UserIndex) const
{
	if (SlotName.IsEmpty())
	{
		return false;
	}
	return UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex);
}

USFPlayerSaveGame* USFPlayerSaveService::PeekSlot(const FString& SlotName, int32 UserIndex) const
{
	if (!DoesSlotExist(SlotName, UserIndex))
	{
		return nullptr;
	}
	return Cast<USFPlayerSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
}

// =============================================================================
// Slot enumeration
// =============================================================================

TArray<FString> USFPlayerSaveService::GetAllSlotNames(int32 UserIndex) const
{
	TArray<FString> Out;

	USFPlayerSaveSlotManifest* Manifest = LoadOrCreateManifest(UserIndex);
	if (!Manifest)
	{
		return Out;
	}

	Out.Reserve(Manifest->KnownSlotNames.Num());
	for (const FString& Name : Manifest->KnownSlotNames)
	{
		// Filter out manifest-internal name and any slots that have been
		// deleted out-of-band (file is gone but manifest is stale).
		if (Name.IsEmpty() || Name == ManifestSlotName)
		{
			continue;
		}
		if (UGameplayStatics::DoesSaveGameExist(Name, UserIndex))
		{
			Out.Add(Name);
		}
	}

	return Out;
}

TArray<FSFPlayerSaveSlotInfo> USFPlayerSaveService::GetAllSlotInfos(int32 UserIndex) const
{
	TArray<FSFPlayerSaveSlotInfo> Out;
	const TArray<FString> Names = GetAllSlotNames(UserIndex);
	Out.Reserve(Names.Num());
	for (const FString& Name : Names)
	{
		Out.Add(GetSlotInfo(Name, UserIndex));
	}
	return Out;
}

FSFPlayerSaveSlotInfo USFPlayerSaveService::GetSlotInfo(const FString& SlotName, int32 UserIndex) const
{
	FSFPlayerSaveSlotInfo Info;
	Info.SlotName = SlotName;

	if (SlotName.IsEmpty() || !UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		return Info;
	}

	USFPlayerSaveGame* SaveObject =
		Cast<USFPlayerSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
	if (!SaveObject)
	{
		return Info;
	}

	Info.FriendlyName = SaveObject->FriendlyName.IsEmpty() ? SaveObject->SlotName : SaveObject->FriendlyName;
	Info.SaveTimestamp = SaveObject->SaveTimestamp;
	Info.AccumulatedPlaytimeSeconds = SaveObject->AccumulatedPlaytimeSeconds;
	Info.LevelName = SaveObject->PlayerData.LevelName;
	Info.PlayerLevel = SaveObject->PlayerData.Level;
	Info.PlayerXP = SaveObject->PlayerData.XP;
	Info.SchemaVersion = SaveObject->PlayerData.SchemaVersion;
	Info.bIsValid = true;
	return Info;
}

// =============================================================================
// Manifest helpers
// =============================================================================

USFPlayerSaveSlotManifest* USFPlayerSaveService::LoadOrCreateManifest(int32 UserIndex) const
{
	if (UGameplayStatics::DoesSaveGameExist(ManifestSlotName, UserIndex))
	{
		if (USFPlayerSaveSlotManifest* Existing =
			Cast<USFPlayerSaveSlotManifest>(UGameplayStatics::LoadGameFromSlot(ManifestSlotName, UserIndex)))
		{
			return Existing;
		}
		UE_LOG(LogSFPlayerSave, Warning,
			TEXT("Manifest slot exists but failed to deserialize; creating a fresh one."));
	}

	return Cast<USFPlayerSaveSlotManifest>(
		UGameplayStatics::CreateSaveGameObject(USFPlayerSaveSlotManifest::StaticClass()));
}

bool USFPlayerSaveService::WriteManifest(USFPlayerSaveSlotManifest* Manifest, int32 UserIndex) const
{
	if (!Manifest)
	{
		return false;
	}
	return UGameplayStatics::SaveGameToSlot(Manifest, ManifestSlotName, UserIndex);
}

void USFPlayerSaveService::RegisterSlotInManifest(const FString& SlotName, int32 UserIndex)
{
	if (SlotName.IsEmpty() || SlotName == ManifestSlotName)
	{
		return;
	}

	USFPlayerSaveSlotManifest* Manifest = LoadOrCreateManifest(UserIndex);
	if (!Manifest)
	{
		return;
	}

	if (!Manifest->KnownSlotNames.Contains(SlotName))
	{
		Manifest->KnownSlotNames.Add(SlotName);
		if (WriteManifest(Manifest, UserIndex))
		{
			OnSlotListChanged.Broadcast(SlotName);
		}
	}
}

void USFPlayerSaveService::DeregisterSlotFromManifest(const FString& SlotName, int32 UserIndex)
{
	if (SlotName.IsEmpty() || SlotName == ManifestSlotName)
	{
		return;
	}

	USFPlayerSaveSlotManifest* Manifest = LoadOrCreateManifest(UserIndex);
	if (!Manifest)
	{
		return;
	}

	const int32 RemovedCount = Manifest->KnownSlotNames.Remove(SlotName);
	if (RemovedCount > 0)
	{
		if (WriteManifest(Manifest, UserIndex))
		{
			OnSlotListChanged.Broadcast(SlotName);
		}
	}
}

// =============================================================================
// Build / apply
// =============================================================================

FSFPlayerSaveData USFPlayerSaveService::BuildSaveData(ASFCharacterBase* InPlayer) const
{
	FSFPlayerSaveData Data;
	Data.SchemaVersion = CurrentSchemaVersion;

	if (!InPlayer)
	{
		return Data;
	}

	// World context
	if (UWorld* World = InPlayer->GetWorld())
	{
		Data.LevelName = FName(*UWorld::RemovePIEPrefix(World->GetMapName()));
		Data.SaveWorldTimeSeconds = UGameplayStatics::GetTimeSeconds(World);
	}

	Data.PlayerTransform = InPlayer->GetActorTransform();

	if (APlayerController* PC = Cast<APlayerController>(InPlayer->GetController()))
	{
		Data.ControlRotation = PC->GetControlRotation();
	}
	else
	{
		Data.ControlRotation = InPlayer->GetActorRotation();
	}

	// Progression
	if (USFProgressionComponent* Progression = InPlayer->GetProgressionComponent())
	{
		Data.Level = Progression->GetCurrentLevel();
		Data.XP = Progression->GetCurrentXP();
	}

	// Attributes -- read via the attribute set rather than ASC->GetNumeric so
	// we don't pick up transient modifier deltas. Current values may include
	// active GE modifiers; for an RPG save this is what the player expects.
	if (UAbilitySystemComponent* ASC = InPlayer->GetAbilitySystemComponent())
	{
		Data.Attributes.Health = ASC->GetNumericAttribute(USFAttributeSetBase::GetHealthAttribute());
		Data.Attributes.MaxHealth = ASC->GetNumericAttribute(USFAttributeSetBase::GetMaxHealthAttribute());
		Data.Attributes.Echo = ASC->GetNumericAttribute(USFAttributeSetBase::GetEchoAttribute());
		Data.Attributes.MaxEcho = ASC->GetNumericAttribute(USFAttributeSetBase::GetMaxEchoAttribute());
		Data.Attributes.Shields = ASC->GetNumericAttribute(USFAttributeSetBase::GetShieldsAttribute());
		Data.Attributes.MaxShields = ASC->GetNumericAttribute(USFAttributeSetBase::GetMaxShieldsAttribute());
		Data.Attributes.Stamina = ASC->GetNumericAttribute(USFAttributeSetBase::GetStaminaAttribute());
		Data.Attributes.MaxStamina = ASC->GetNumericAttribute(USFAttributeSetBase::GetMaxStaminaAttribute());
	}

	// Inventory
	if (USFInventoryComponent* Inventory = InPlayer->GetInventoryComponent())
	{
		const TArray<FSFInventoryEntry>& LiveEntries = Inventory->GetInventoryEntries();
		Data.InventoryEntries.Reserve(LiveEntries.Num());
		for (const FSFInventoryEntry& Entry : LiveEntries)
		{
			if (!Entry.ItemDefinition)
			{
				continue;
			}
			Data.InventoryEntries.Add(SFPlayerSaveLocal::MakeSaveEntry(Entry, Data.SaveWorldTimeSeconds));
		}
	}

	// Ammo reserves
	if (USFAmmoReserveComponent* Ammo = InPlayer->GetAmmoReserveComponent())
	{
		const TMap<TObjectPtr<USFAmmoType>, int32>& Reserves = Ammo->GetReserves();
		Data.AmmoReserves.Reserve(Reserves.Num());
		for (const TPair<TObjectPtr<USFAmmoType>, int32>& Pair : Reserves)
		{
			if (!Pair.Key || Pair.Value <= 0)
			{
				continue;
			}
			FSFPlayerSaveAmmoEntry AmmoEntry;
			AmmoEntry.AmmoType = TSoftObjectPtr<USFAmmoType>(Pair.Key.Get());
			AmmoEntry.Count = Pair.Value;
			Data.AmmoReserves.Add(AmmoEntry);
		}
	}

	return Data;
}

bool USFPlayerSaveService::ApplySaveData(const FSFPlayerSaveData& Data, ASFCharacterBase* InPlayer)
{
	if (!InPlayer)
	{
		return false;
	}

	UWorld* World = InPlayer->GetWorld();
	const float LoadWorldTime = World ? UGameplayStatics::GetTimeSeconds(World) : 0.0f;

	// Place the pawn FIRST so any side effects of attribute changes (e.g.
	// movement-component-driven velocity updates) happen at the right spot.
	InPlayer->SetActorTransform(Data.PlayerTransform, /*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);

	if (APlayerController* PC = Cast<APlayerController>(InPlayer->GetController()))
	{
		PC->SetControlRotation(Data.ControlRotation);
	}

	// Progression FIRST -- this resets Max* attribute baselines based on the
	// level's stat curve. The subsequent attribute-apply step then writes
	// the precise saved current/max values on top, which is the source of
	// truth for save/load.
	if (USFProgressionComponent* Progression = InPlayer->GetProgressionComponent())
	{
		Progression->RestoreFromSave(Data.Level, Data.XP);
	}

	// Attributes -- write Max* before current values so any clamping inside
	// the attribute set sees the correct ceiling.
	if (UAbilitySystemComponent* ASC = InPlayer->GetAbilitySystemComponent())
	{
		ASC->SetNumericAttributeBase(USFAttributeSetBase::GetMaxHealthAttribute(),  Data.Attributes.MaxHealth);
		ASC->SetNumericAttributeBase(USFAttributeSetBase::GetMaxEchoAttribute(),    Data.Attributes.MaxEcho);
		ASC->SetNumericAttributeBase(USFAttributeSetBase::GetMaxShieldsAttribute(), Data.Attributes.MaxShields);
		ASC->SetNumericAttributeBase(USFAttributeSetBase::GetMaxStaminaAttribute(), Data.Attributes.MaxStamina);

		ASC->SetNumericAttributeBase(USFAttributeSetBase::GetHealthAttribute(),
			FMath::Clamp(Data.Attributes.Health, 0.0f, Data.Attributes.MaxHealth));
		ASC->SetNumericAttributeBase(USFAttributeSetBase::GetEchoAttribute(),
			FMath::Clamp(Data.Attributes.Echo, 0.0f, Data.Attributes.MaxEcho));
		ASC->SetNumericAttributeBase(USFAttributeSetBase::GetShieldsAttribute(),
			FMath::Clamp(Data.Attributes.Shields, 0.0f, Data.Attributes.MaxShields));
		ASC->SetNumericAttributeBase(USFAttributeSetBase::GetStaminaAttribute(),
			FMath::Clamp(Data.Attributes.Stamina, 0.0f, Data.Attributes.MaxStamina));
	}

	// Inventory -- resolve soft pointers + fallback ItemId, build runtime
	// entries, and replace the component's contents in one shot.
	if (USFInventoryComponent* Inventory = InPlayer->GetInventoryComponent())
	{
		TArray<FSFInventoryEntry> Runtime;
		Runtime.Reserve(Data.InventoryEntries.Num());
		for (const FSFPlayerSaveInventoryEntry& SavedEntry : Data.InventoryEntries)
		{
			FSFInventoryEntry NewEntry;
			if (SFPlayerSaveLocal::TryMakeRuntimeEntry(SavedEntry, LoadWorldTime, NewEntry))
			{
				Runtime.Add(MoveTemp(NewEntry));
			}
		}
		Inventory->SetInventoryEntriesFromSave(Runtime);
	}

	// Ammo reserves -- SetAmmoCount writes through the change delegate so UI
	// reflects the loaded counts. Soft-pointer resolution is straightforward
	// here; we don't carry a separate FName key for ammo types yet because
	// they're a much smaller set than items.
	if (USFAmmoReserveComponent* Ammo = InPlayer->GetAmmoReserveComponent())
	{
		// Zero everything first so a load can't leave behind stale ammo
		// rows that weren't in the save (e.g. ammo type the player no
		// longer carries any of). We do this by setting each currently-
		// known type to 0 *unless* it's in the save (handled below).
		TSet<USFAmmoType*> RestoredTypes;
		RestoredTypes.Reserve(Data.AmmoReserves.Num());

		for (const FSFPlayerSaveAmmoEntry& Entry : Data.AmmoReserves)
		{
			USFAmmoType* Resolved = Entry.AmmoType.LoadSynchronous();
			if (!Resolved)
			{
				UE_LOG(LogSFPlayerSave, Warning,
					TEXT("Save: could not resolve ammo type %s -- dropped."),
					*Entry.AmmoType.ToString());
				continue;
			}
			Ammo->SetAmmoCount(Resolved, FMath::Max(0, Entry.Count));
			RestoredTypes.Add(Resolved);
		}

		// Zero out any reserve row not present in the save.
		const TMap<TObjectPtr<USFAmmoType>, int32>& LiveReserves = Ammo->GetReserves();
		TArray<USFAmmoType*> ToZero;
		ToZero.Reserve(LiveReserves.Num());
		for (const TPair<TObjectPtr<USFAmmoType>, int32>& Pair : LiveReserves)
		{
			if (Pair.Key && Pair.Value > 0 && !RestoredTypes.Contains(Pair.Key.Get()))
			{
				ToZero.Add(Pair.Key.Get());
			}
		}
		for (USFAmmoType* Type : ToZero)
		{
			Ammo->SetAmmoCount(Type, 0);
		}
	}

	return true;
}

bool USFPlayerSaveService::MigrateSaveData(FSFPlayerSaveData& InOutData) const
{
	// Refuse payloads we don't know how to read.
	if (InOutData.SchemaVersion > CurrentSchemaVersion)
	{
		return false;
	}

	// SchemaVersion 1 is the initial release -- no migrations yet. When we
	// bump to 2, this is where we add the v1->v2 transform.
	if (InOutData.SchemaVersion < CurrentSchemaVersion)
	{
		UE_LOG(LogSFPlayerSave, Log,
			TEXT("MigrateSaveData: in-place upgrade from %d to %d (no transforms registered yet)."),
			InOutData.SchemaVersion, CurrentSchemaVersion);
		InOutData.SchemaVersion = CurrentSchemaVersion;
	}

	return true;
}

// =============================================================================
// Helpers
// =============================================================================

ASFCharacterBase* USFPlayerSaveService::ResolvePlayer(ASFCharacterBase* InPlayer, int32 UserIndex) const
{
	if (InPlayer)
	{
		return InPlayer;
	}
	return Cast<ASFCharacterBase>(UGameplayStatics::GetPlayerCharacter(this, UserIndex));
}
