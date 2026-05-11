#include "Components/SFEquipmentComponent.h"

#include "Combat/SFAmmoType.h"
#include "Combat/SFWeaponActor.h"
#include "Combat/SFWeaponData.h"
#include "Components/SFAmmoReserveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "Inventory/SFItemDefinition.h"
#include "Inventory/SFItemTypes.h"
#include "Components/SFInventoryComponent.h"
#include "Characters/SFCharacterBase.h"
#include "Core/SignalForgeGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "TimerManager.h"

USFEquipmentComponent::USFEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USFEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bEquipDefaultWeaponOnBeginPlay && DefaultWeaponData)
	{
		FSFWeaponInstanceData DefaultInstance;
		DefaultInstance.WeaponDefinition = DefaultWeaponData;
		EquipWeaponInstance(DefaultInstance, ESFEquipmentSlot::PrimaryWeapon);
	}
}

void USFEquipmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearAllEquipment();
	Super::EndPlay(EndPlayReason);
}

USFWeaponData* USFEquipmentComponent::GetCurrentWeaponData() const
{
	return CurrentWeaponInstance.WeaponDefinition.IsNull()
		? nullptr
		: CurrentWeaponInstance.WeaponDefinition.LoadSynchronous();
}

bool USFEquipmentComponent::EquipWeaponData(USFWeaponData* NewWeaponData, ESFEquipmentSlot InSlot)
{
	if (!NewWeaponData)
	{
		return false;
	}

	FSFWeaponInstanceData Instance;
	Instance.WeaponDefinition = NewWeaponData;
	return EquipWeaponInstance(Instance, InSlot);
}

bool USFEquipmentComponent::EquipWeaponInstance(const FSFWeaponInstanceData& WeaponInstance, ESFEquipmentSlot InSlot)
{
	if (!WeaponInstance.IsValid())
	{
		return false;
	}

	if (InSlot == ESFEquipmentSlot::None)
	{
		InSlot = ActiveWeaponSlot != ESFEquipmentSlot::None
			? ActiveWeaponSlot
			: ESFEquipmentSlot::PrimaryWeapon;
	}

	USFWeaponData* NewWeaponData = WeaponInstance.WeaponDefinition.LoadSynchronous();
	if (!NewWeaponData)
	{
		return false;
	}

	if (CurrentWeaponInstance.InstanceId == WeaponInstance.InstanceId &&
		ActiveWeaponSlot == InSlot)
	{
		return true;
	}

	// Strict ability model: when we change the active slot, the previously-active slot's
	// weapon abilities must no longer be usable. The new slot's abilities are granted below.
	// (Same-slot re-equip is allowed — we still re-grant in case the weapon data changed.)
	if (ActiveWeaponSlot != ESFEquipmentSlot::None && ActiveWeaponSlot != InSlot)
	{
		RemoveWeaponAbilitiesForSlot(ActiveWeaponSlot);
		// Snap the now-inactive weapon visual to its holster socket.
		UpdateWeaponActorAttachmentForSlot(ActiveWeaponSlot, /*bIsActive=*/false);
	}

	ActiveWeaponSlot = InSlot;
	CurrentWeaponInstance = WeaponInstance;

	// Auto-load the clip on first equip so a fresh weapon doesn't dry-fire on its first trigger
	// pull. If the weapon has an AmmoType, draw from the carrier's reserve; otherwise (energy /
	// reserve-less weapons) just top to ClipSize so the weapon is usable. Instances with prior
	// non-zero ammo (e.g. a saved half-empty clip) are left alone.
	if (NewWeaponData->AmmoConfig.ClipSize > 0 && CurrentWeaponInstance.AmmoInClip <= 0)
	{
		const int32 ClipSize = NewWeaponData->AmmoConfig.ClipSize;
		USFAmmoType* AmmoType = NewWeaponData->AmmoConfig.AmmoType;

		if (AmmoType)
		{
			if (ASFCharacterBase* OwningCharacter = Cast<ASFCharacterBase>(GetOwner()))
			{
				if (USFAmmoReserveComponent* Reserve = OwningCharacter->GetAmmoReserveComponent())
				{
					// Seed the reserve once if it's empty, so freshly-spawned characters with no
					// loadout still have something to load. Designers can override by setting the
					// reserve explicitly before equip, or by setting DefaultStartingAmount to 0.
					Reserve->EnsureStartingAmmo(AmmoType);
					const int32 Drawn = Reserve->ConsumeAmmo(AmmoType, ClipSize);
					CurrentWeaponInstance.AmmoInClip = Drawn;
				}
				else
				{
					CurrentWeaponInstance.AmmoInClip = ClipSize;
				}
			}
			else
			{
				CurrentWeaponInstance.AmmoInClip = ClipSize;
			}
		}
		else
		{
			// Weapon doesn't draw from a pooled reserve — free top-up.
			CurrentWeaponInstance.AmmoInClip = ClipSize;
		}
	}

	RefreshEquippedWeaponActor();

	// New slot is now active — make sure its visual is on the hand socket, not the holster.
	UpdateWeaponActorAttachmentForSlot(InSlot, /*bIsActive=*/true);

	// Replace any prior weapon-granted abilities for this slot, then grant new ones.
	RemoveWeaponAbilitiesForSlot(InSlot);
	GrantWeaponAbilitiesForSlot(InSlot, NewWeaponData);

	if (ASFCharacterBase* Character = Cast<ASFCharacterBase>(GetOwner()))
	{
		Character->OnWeaponEquipped(NewWeaponData);
	}

	OnEquippedWeaponChanged.Broadcast(NewWeaponData, CurrentWeaponInstance);
	BroadcastEquipmentUpdated();
	return true;
}

ESFEquipmentOpResult USFEquipmentComponent::EquipItemDefinition(USFItemDefinition* ItemDefinition)
{
	if (!ItemDefinition)
	{
		return ESFEquipmentOpResult::InvalidItem;
	}

	const ESFEquipmentSlot PreferredSlot = ResolvePreferredSlotForItem(ItemDefinition);
	if (PreferredSlot == ESFEquipmentSlot::None)
	{
		return ESFEquipmentOpResult::UnsupportedItemType;
	}

	return EquipItemToSlot(ItemDefinition, PreferredSlot);
}

ESFEquipmentOpResult USFEquipmentComponent::EquipItemToSlot(
	USFItemDefinition* ItemDefinition,
	ESFEquipmentSlot Slot)
{
	if (!ItemDefinition)
	{
		return ESFEquipmentOpResult::InvalidItem;
	}

	if (Slot == ESFEquipmentSlot::None)
	{
		return ESFEquipmentOpResult::InvalidSlot;
	}

	if (ItemDefinition->IsWeapon())
	{
		if (!IsWeaponSlot(Slot))
		{
			return ESFEquipmentOpResult::InvalidSlot;
		}

		if (!ItemDefinition->HasWeaponData())
		{
			return ESFEquipmentOpResult::MissingWeaponData;
		}

		FSFWeaponInstanceData Instance;
		Instance.WeaponDefinition = ItemDefinition->WeaponData;

		return EquipItemInstanceToSlot(ItemDefinition, Instance, Slot);
	}

	if (IsWeaponSlot(Slot))
	{
		return ESFEquipmentOpResult::InvalidSlot;
	}

	if (GetEquippedItemInSlot(Slot) == ItemDefinition)
	{
		return ESFEquipmentOpResult::AlreadyEquipped;
	}

	SetSlotEntry(Slot, ItemDefinition, nullptr, nullptr);
	OnEquipmentSlotChanged.Broadcast(Slot, ItemDefinition, FSFWeaponInstanceData());
	BroadcastEquipmentUpdated();
	return ESFEquipmentOpResult::Success;
}

ESFEquipmentOpResult USFEquipmentComponent::EquipItemInstanceToSlot(
	USFItemDefinition* ItemDefinition,
	const FSFWeaponInstanceData& WeaponInstance,
	ESFEquipmentSlot Slot)
{
	if (!ItemDefinition)
	{
		return ESFEquipmentOpResult::InvalidItem;
	}

	if (Slot == ESFEquipmentSlot::None)
	{
		return ESFEquipmentOpResult::InvalidSlot;
	}

	if (!ItemDefinition->IsWeapon())
	{
		return ESFEquipmentOpResult::UnsupportedItemType;
	}

	if (!IsWeaponSlot(Slot))
	{
		return ESFEquipmentOpResult::InvalidSlot;
	}

	if (!WeaponInstance.IsValid())
	{
		return ESFEquipmentOpResult::MissingWeaponData;
	}

	USFWeaponData* WeaponData = WeaponInstance.WeaponDefinition.LoadSynchronous();
	if (!WeaponData)
	{
		return ESFEquipmentOpResult::MissingWeaponData;
	}

	const FSFEquipmentSlotEntry ExistingEntry = GetEquipmentSlotEntry(Slot);
	if (ExistingEntry.bHasItemEquipped &&
		ExistingEntry.ItemDefinition == ItemDefinition &&
		ExistingEntry.WeaponInstance.InstanceId == WeaponInstance.InstanceId &&
		ActiveWeaponSlot == Slot)
	{
		return ESFEquipmentOpResult::AlreadyEquipped;
	}

	SetSlotEntry(Slot, ItemDefinition, &WeaponInstance, &ExistingEntry.InventoryEntryId);

	RefreshEquippedWeaponActorForSlot(Slot, WeaponData);

	if (ActiveWeaponSlot == ESFEquipmentSlot::None ||
		Slot == ESFEquipmentSlot::PrimaryWeapon)
	{
		if (!EquipWeaponInstance(WeaponInstance, Slot))
		{
			return ESFEquipmentOpResult::EquipFailed;
		}
	}

	OnEquipmentSlotChanged.Broadcast(Slot, ItemDefinition, WeaponInstance);
	BroadcastEquipmentUpdated();
	return ESFEquipmentOpResult::Success;
}

ESFEquipmentOpResult USFEquipmentComponent::EquipFromInventoryEntry(
	const FGuid& InventoryEntryId,
	ESFEquipmentSlot SlotOverride /*= ESFEquipmentSlot::None*/)
{
	if (!InventoryEntryId.IsValid())
	{
		return ESFEquipmentOpResult::InvalidInventoryEntry;
	}

	// Find the owner's inventory component
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return ESFEquipmentOpResult::InvalidInventoryEntry;
	}

	USFInventoryComponent* Inventory = OwnerActor->FindComponentByClass<USFInventoryComponent>();
	if (!Inventory)
	{
		return ESFEquipmentOpResult::InvalidInventoryEntry;
	}

	FSFInventoryEntry InventoryEntry;
	if (!Inventory->GetEntryById(InventoryEntryId, InventoryEntry) || !InventoryEntry.IsValid())
	{
		return ESFEquipmentOpResult::InvalidInventoryEntry;
	}

	USFItemDefinition* ItemDef = InventoryEntry.ItemDefinition;
	if (!ItemDef)
	{
		return ESFEquipmentOpResult::InvalidItem;
	}

	ESFEquipmentSlot Slot = SlotOverride;
	if (Slot == ESFEquipmentSlot::None)
	{
		Slot = ResolvePreferredSlotForItem(ItemDef);
	}

	if (Slot == ESFEquipmentSlot::None)
	{
		return ESFEquipmentOpResult::UnsupportedItemType;
	}

	// Weapon path
	if (ItemDef->IsWeapon())
	{
		if (!IsWeaponSlot(Slot))
		{
			return ESFEquipmentOpResult::InvalidSlot;
		}

		const FSFWeaponInstanceData& Instance = InventoryEntry.WeaponInstanceData;
		if (!Instance.IsValid())
		{
			// Fallback: build instance from definition if needed.
			if (!ItemDef->HasWeaponData())
			{
				return ESFEquipmentOpResult::MissingWeaponData;
			}

			FSFWeaponInstanceData FallbackInstance;
			FallbackInstance.WeaponDefinition = ItemDef->WeaponData;
			return EquipItemInstanceToSlot(ItemDef, FallbackInstance, Slot);
		}

		USFWeaponData* WeaponData = Instance.WeaponDefinition.LoadSynchronous();
		if (!WeaponData)
		{
			return ESFEquipmentOpResult::MissingWeaponData;
		}

		const FSFEquipmentSlotEntry ExistingEntry = GetEquipmentSlotEntry(Slot);
		if (ExistingEntry.bHasItemEquipped &&
			ExistingEntry.ItemDefinition == ItemDef &&
			ExistingEntry.WeaponInstance.InstanceId == Instance.InstanceId)
		{
			return ESFEquipmentOpResult::AlreadyEquipped;
		}

		// Equip and record InventoryEntryId
		SetSlotEntry(Slot, ItemDef, &Instance, &InventoryEntry.EntryId);
		RefreshEquippedWeaponActorForSlot(Slot, WeaponData);

		if (ActiveWeaponSlot == ESFEquipmentSlot::None ||
			Slot == ESFEquipmentSlot::PrimaryWeapon)
		{
			if (!EquipWeaponInstance(Instance, Slot))
			{
				return ESFEquipmentOpResult::EquipFailed;
			}
		}

		OnEquipmentSlotChanged.Broadcast(Slot, ItemDef, Instance);
		BroadcastEquipmentUpdated();
		return ESFEquipmentOpResult::Success;
	}

	// Non-weapon items
	if (IsWeaponSlot(Slot))
	{
		return ESFEquipmentOpResult::InvalidSlot;
	}

	if (GetEquippedItemInSlot(Slot) == ItemDef)
	{
		return ESFEquipmentOpResult::AlreadyEquipped;
	}

	SetSlotEntry(Slot, ItemDef, nullptr, &InventoryEntry.EntryId);
	OnEquipmentSlotChanged.Broadcast(Slot, ItemDef, FSFWeaponInstanceData());
	BroadcastEquipmentUpdated();
	return ESFEquipmentOpResult::Success;
}

bool USFEquipmentComponent::UnequipSlot(ESFEquipmentSlot Slot)
{
	if (Slot == ESFEquipmentSlot::None)
	{
		return false;
	}

	FSFEquipmentSlotEntry* ExistingEntry = EquippedSlots.Find(Slot);
	if (!ExistingEntry || !ExistingEntry->bHasItemEquipped)
	{
		return false;
	}

	const FSFWeaponInstanceData OldInstance = ExistingEntry->WeaponInstance;

	EquippedSlots.Remove(Slot);
	OnEquipmentSlotChanged.Broadcast(Slot, nullptr, FSFWeaponInstanceData());

	if (IsWeaponSlot(Slot))
	{
		DestroyEquippedWeaponActorForSlot(Slot);
		RemoveWeaponAbilitiesForSlot(Slot);

		if (ActiveWeaponSlot == Slot)
		{
			ActiveWeaponSlot = ESFEquipmentSlot::None;
			CurrentWeaponInstance = FSFWeaponInstanceData();
			OnEquippedWeaponChanged.Broadcast(nullptr, FSFWeaponInstanceData());
		}
	}

	BroadcastEquipmentUpdated();
	return true;
}

void USFEquipmentComponent::ClearEquippedWeapon()
{
	if (!CurrentWeaponInstance.IsValid() && ActiveWeaponSlot == ESFEquipmentSlot::None)
	{
		return;
	}

	DestroyEquippedWeaponActorForSlot(ActiveWeaponSlot);
	RemoveWeaponAbilitiesForSlot(ActiveWeaponSlot);

	CurrentWeaponInstance = FSFWeaponInstanceData();
	ActiveWeaponSlot = ESFEquipmentSlot::None;

	if (ASFCharacterBase* Character = Cast<ASFCharacterBase>(GetOwner()))
	{
		Character->OnWeaponEquipped(nullptr);
	}

	DestroyEquippedWeaponActor();

	OnEquippedWeaponChanged.Broadcast(nullptr, FSFWeaponInstanceData());
	BroadcastEquipmentUpdated();
}

void USFEquipmentComponent::ClearAllEquipment()
{
	const bool bHadAnyEquipment =
		!EquippedSlots.IsEmpty() ||
		CurrentWeaponInstance.IsValid() ||
		!EquippedWeaponActors.IsEmpty() ||
		EquippedWeaponActor != nullptr;

	if (!bHadAnyEquipment)
	{
		return;
	}

	for (const TPair<ESFEquipmentSlot, TObjectPtr<ASFWeaponActor>>& Pair : EquippedWeaponActors)
	{
		if (Pair.Value)
		{
			Pair.Value->Destroy();
		}
	}
	EquippedWeaponActors.Empty();

	// Clear all weapon-granted ability handles before we forget which slot owned them.
	TArray<ESFEquipmentSlot> SlotsWithAbilities;
	GrantedAbilityHandles.GenerateKeyArray(SlotsWithAbilities);
	for (const ESFEquipmentSlot Slot : SlotsWithAbilities)
	{
		RemoveWeaponAbilitiesForSlot(Slot);
	}
	GrantedAbilityHandles.Empty();

	EquippedSlots.Reset();
	CurrentWeaponInstance = FSFWeaponInstanceData();
	ActiveWeaponSlot = ESFEquipmentSlot::None;

	DestroyEquippedWeaponActor();

	OnEquippedWeaponChanged.Broadcast(nullptr, FSFWeaponInstanceData());
	BroadcastEquipmentUpdated();
}

void USFEquipmentComponent::SetActiveWeaponSlot(ESFEquipmentSlot Slot)
{
	if (!IsWeaponSlot(Slot))
	{
		return;
	}

	if (ActiveWeaponSlot == Slot)
	{
		return;
	}

	const FSFEquipmentSlotEntry Entry = GetEquipmentSlotEntry(Slot);
	if (!Entry.bHasItemEquipped || !Entry.WeaponInstance.IsValid())
	{
		return;
	}

	EquipWeaponInstance(Entry.WeaponInstance, Slot);
}

ASFWeaponActor* USFEquipmentComponent::GetEquippedWeaponActor() const
{
	return ActiveWeaponSlot != ESFEquipmentSlot::None
		? GetEquippedWeaponActorForSlot(ActiveWeaponSlot)
		: nullptr;
}

ASFWeaponActor* USFEquipmentComponent::GetEquippedWeaponActorForSlot(ESFEquipmentSlot Slot) const
{
	if (const TObjectPtr<ASFWeaponActor>* Found = EquippedWeaponActors.Find(Slot))
	{
		return Found->Get();
	}
	return nullptr;
}

bool USFEquipmentComponent::HasItemEquippedInSlot(ESFEquipmentSlot Slot) const
{
	const FSFEquipmentSlotEntry* Entry = EquippedSlots.Find(Slot);
	return Entry && Entry->bHasItemEquipped && Entry->ItemDefinition != nullptr;
}

USFItemDefinition* USFEquipmentComponent::GetEquippedItemInSlot(ESFEquipmentSlot Slot) const
{
	const FSFEquipmentSlotEntry* Entry = EquippedSlots.Find(Slot);
	return Entry ? Entry->ItemDefinition.Get() : nullptr;
}

FSFWeaponInstanceData USFEquipmentComponent::GetEquippedWeaponInstanceInSlot(ESFEquipmentSlot Slot) const
{
	const FSFEquipmentSlotEntry* Entry = EquippedSlots.Find(Slot);
	return Entry ? Entry->WeaponInstance : FSFWeaponInstanceData();
}

FSFEquipmentSlotEntry USFEquipmentComponent::GetEquipmentSlotEntry(ESFEquipmentSlot Slot) const
{
	if (const FSFEquipmentSlotEntry* Entry = EquippedSlots.Find(Slot))
	{
		return *Entry;
	}

	return FSFEquipmentSlotEntry(Slot, nullptr);
}

float USFEquipmentComponent::GetTotalEquippedWeight() const
{
	float Total = 0.0f;

	for (const TPair<ESFEquipmentSlot, FSFEquipmentSlotEntry>& Pair : EquippedSlots)
	{
		const FSFEquipmentSlotEntry& Entry = Pair.Value;
		if (Entry.bHasItemEquipped && Entry.ItemDefinition)
		{
			Total += Entry.ItemDefinition->Weight;
		}
	}

	return Total;
}

void USFEquipmentComponent::GetEquippedItemsOfType(
	ESFItemType ItemType,
	TArray<FSFEquipmentSlotEntry>& OutEntries) const
{
	OutEntries.Reset();

	for (const TPair<ESFEquipmentSlot, FSFEquipmentSlotEntry>& Pair : EquippedSlots)
	{
		const FSFEquipmentSlotEntry& Entry = Pair.Value;
		if (Entry.bHasItemEquipped &&
			Entry.ItemDefinition &&
			Entry.ItemDefinition->ItemType == ItemType)
		{
			OutEntries.Add(Entry);
		}
	}
}

void USFEquipmentComponent::GetEquippedWeapons(TArray<FSFEquipmentSlotEntry>& OutEntries) const
{
	GetEquippedItemsOfType(ESFItemType::Weapon, OutEntries);
}

FSFWeaponAnimationProfile USFEquipmentComponent::GetCurrentAnimationProfile() const
{
	if (USFWeaponData* WeaponData = GetCurrentWeaponData())
	{
		return WeaponData->GetAnimationProfile();
	}
	return FSFWeaponAnimationProfile();
}

ESFOverlayMode USFEquipmentComponent::GetCurrentOverlayMode() const
{
	const FSFWeaponAnimationProfile Profile = GetCurrentAnimationProfile();
	return Profile.OverlayMode;
}

ESFCombatMode USFEquipmentComponent::GetCurrentCombatMode() const
{
	const FSFWeaponAnimationProfile Profile = GetCurrentAnimationProfile();
	return Profile.CombatMode;
}

UAnimMontage* USFEquipmentComponent::GetCurrentLightAttackMontage() const
{
	if (USFWeaponData* WeaponData = GetCurrentWeaponData())
	{
		return WeaponData->GetLightAttackMontage();
	}
	return nullptr;
}

UAnimMontage* USFEquipmentComponent::GetCurrentHeavyAttackMontage() const
{
	if (USFWeaponData* WeaponData = GetCurrentWeaponData())
	{
		return WeaponData->GetHeavyAttackMontage();
	}
	return nullptr;
}

UAnimMontage* USFEquipmentComponent::GetCurrentAbilityMontage() const
{
	if (USFWeaponData* WeaponData = GetCurrentWeaponData())
	{
		return WeaponData->GetAbilityMontage();
	}
	return nullptr;
}

// Legacy single-actor path (for active weapon only).
void USFEquipmentComponent::RefreshEquippedWeaponActor()
{
	DestroyEquippedWeaponActor();

	USFWeaponData* WeaponData = GetCurrentWeaponData();
	if (!WeaponData || !WeaponData->WeaponActorClass)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (!OwnerActor || !World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerActor;
	SpawnParams.Instigator = Cast<APawn>(OwnerActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	EquippedWeaponActor = World->SpawnActor<ASFWeaponActor>(
		WeaponData->WeaponActorClass,
		FTransform::Identity,
		SpawnParams);

	if (!EquippedWeaponActor)
	{
		return;
	}

	EquippedWeaponActor->InitializeFromWeaponData(WeaponData);

	if (USkeletalMeshComponent* OwnerMesh = GetOwnerSkeletalMesh())
	{
		const FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, true);
		EquippedWeaponActor->AttachToComponent(OwnerMesh, AttachRules, WeaponData->AttachSocketName);
		EquippedWeaponActor->SetActorRelativeTransform(WeaponData->RelativeAttachTransform);
	}
	else
	{
		EquippedWeaponActor->Destroy();
		EquippedWeaponActor = nullptr;
	}
}

void USFEquipmentComponent::DestroyEquippedWeaponActor()
{
	if (EquippedWeaponActor)
	{
		EquippedWeaponActor->Destroy();
		EquippedWeaponActor = nullptr;
	}
}

// New: per-slot visual actor management.
void USFEquipmentComponent::RefreshEquippedWeaponActorForSlot(
	ESFEquipmentSlot Slot,
	USFWeaponData* WeaponData)
{
	DestroyEquippedWeaponActorForSlot(Slot);

	if (!WeaponData)
	{
		return;
	}

	// Misconfiguration guards: warn loudly so designers immediately see why a slot
	// has no visible mesh or always attaches to the hand socket instead of holster.
	if (!WeaponData->WeaponActorClass)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("SFEquipment: WeaponActorClass is NULL on '%s' (slot %d) -- no mesh will be spawned. ")
			TEXT("Assign a BP_Weapon_* class on the weapon data asset."),
			*WeaponData->GetName(),
			static_cast<int32>(Slot));
		return;
	}

	if (WeaponData->AttachSocketName == NAME_None)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("SFEquipment: AttachSocketName is NAME_None on '%s' (slot %d) -- weapon will attach to mesh root. ")
			TEXT("Set the hand socket (e.g. 'GripPoint' / 'hand_r_socket') on the weapon data asset."),
			*WeaponData->GetName(),
			static_cast<int32>(Slot));
	}

	if (WeaponData->HolsteredAttachSocketName == NAME_None)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("SFEquipment: HolsteredAttachSocketName is NAME_None on '%s' (slot %d) -- holstered weapons will use the hand socket as a fallback. ")
			TEXT("Set a holster socket (e.g. 'spine_03_holster_primary') on the weapon data asset to avoid weapons stacking in the hand."),
			*WeaponData->GetName(),
			static_cast<int32>(Slot));
	}

	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (!OwnerActor || !World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerActor;
	SpawnParams.Instigator = Cast<APawn>(OwnerActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASFWeaponActor* NewActor =
		World->SpawnActor<ASFWeaponActor>(WeaponData->WeaponActorClass, FTransform::Identity, SpawnParams);

	if (!NewActor)
	{
		return;
	}

	NewActor->InitializeFromWeaponData(WeaponData);

	if (USkeletalMeshComponent* OwnerMesh = GetOwnerSkeletalMesh())
	{
		const FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, true);
		// Attach to the appropriate socket based on whether this slot is currently active.
		// Non-active slots use the holster socket when one is provided; if no holster socket is
		// configured we fall back to the hand socket (loud-and-visible default beats silent fail).
		const bool bIsActiveSlot = (ActiveWeaponSlot == Slot);
		const bool bHasHolsterSocket = WeaponData->HolsteredAttachSocketName != NAME_None;
		const FName SocketName = (!bIsActiveSlot && bHasHolsterSocket)
			? WeaponData->HolsteredAttachSocketName
			: WeaponData->AttachSocketName;
		const FTransform& RelativeXform = (!bIsActiveSlot && bHasHolsterSocket)
			? WeaponData->HolsteredRelativeAttachTransform
			: WeaponData->RelativeAttachTransform;

		NewActor->AttachToComponent(OwnerMesh, AttachRules, SocketName);
		NewActor->SetActorRelativeTransform(RelativeXform);
	}
	else
	{
		NewActor->Destroy();
		return;
	}

	EquippedWeaponActors.Add(Slot, NewActor);
}

void USFEquipmentComponent::DestroyEquippedWeaponActorForSlot(ESFEquipmentSlot Slot)
{
	if (TObjectPtr<ASFWeaponActor>* Found = EquippedWeaponActors.Find(Slot))
	{
		if (Found->Get())
		{
			Found->Get()->Destroy();
		}

		EquippedWeaponActors.Remove(Slot);
	}
}

void USFEquipmentComponent::BroadcastEquipmentUpdated()
{
	OnEquipmentUpdated.Broadcast();
}

void USFEquipmentComponent::SetSlotEntry(
	ESFEquipmentSlot Slot,
	USFItemDefinition* ItemDefinition,
	const FSFWeaponInstanceData* WeaponInstance,
	const FGuid* InventoryEntryId)
{
	if (Slot == ESFEquipmentSlot::None)
	{
		return;
	}

	FSFEquipmentSlotEntry& Entry = EquippedSlots.FindOrAdd(Slot);
	Entry.Slot = Slot;
	Entry.ItemDefinition = ItemDefinition;
	Entry.bHasItemEquipped = (ItemDefinition != nullptr);

	if (WeaponInstance)
	{
		Entry.WeaponInstance = *WeaponInstance;
	}
	else
	{
		Entry.WeaponInstance = FSFWeaponInstanceData();
	}

	if (InventoryEntryId && InventoryEntryId->IsValid())
	{
		Entry.InventoryEntryId = *InventoryEntryId;
	}
	else if (!Entry.bHasItemEquipped)
	{
		Entry.InventoryEntryId.Invalidate();
	}
}

ESFEquipmentSlot USFEquipmentComponent::ResolvePreferredSlotForItem(USFItemDefinition* ItemDefinition) const
{
	if (!ItemDefinition)
	{
		return ESFEquipmentSlot::None;
	}

	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
	const FGameplayTag SlotTag = ItemDefinition->ItemCategoryTag;

	if (SlotTag.IsValid())
	{
		if (SlotTag.MatchesTagExact(Tags.Equipment_Slot_Head))
		{
			return ESFEquipmentSlot::Head;
		}

		if (SlotTag.MatchesTagExact(Tags.Equipment_Slot_Chest))
		{
			return ESFEquipmentSlot::Chest;
		}

		if (SlotTag.MatchesTagExact(Tags.Equipment_Slot_Arms))
		{
			return ESFEquipmentSlot::Arms;
		}

		if (SlotTag.MatchesTagExact(Tags.Equipment_Slot_Legs))
		{
			return ESFEquipmentSlot::Legs;
		}

		if (SlotTag.MatchesTagExact(Tags.Equipment_Slot_Boots))
		{
			return ESFEquipmentSlot::Boots;
		}

		if (SlotTag.MatchesTagExact(Tags.Equipment_Slot_PrimaryWeapon))
		{
			return ESFEquipmentSlot::PrimaryWeapon;
		}

		if (SlotTag.MatchesTagExact(Tags.Equipment_Slot_SecondaryWeapon))
		{
			return ESFEquipmentSlot::SecondaryWeapon;
		}

		if (SlotTag.MatchesTagExact(Tags.Equipment_Slot_HeavyWeapon))
		{
			return ESFEquipmentSlot::HeavyWeapon;
		}
	}

	// Fallbacks if no slot tag is assigned yet.
	switch (ItemDefinition->ItemType)
	{
	case ESFItemType::Weapon:
		return ESFEquipmentSlot::PrimaryWeapon;

	case ESFItemType::Armor:
		return ESFEquipmentSlot::Chest;

	default:
		return ESFEquipmentSlot::None;
	}
}

bool USFEquipmentComponent::IsWeaponSlot(ESFEquipmentSlot Slot) const
{
	return Slot == ESFEquipmentSlot::PrimaryWeapon
		|| Slot == ESFEquipmentSlot::SecondaryWeapon
		|| Slot == ESFEquipmentSlot::HeavyWeapon;
}

USkeletalMeshComponent* USFEquipmentComponent::GetOwnerSkeletalMesh() const
{
	if (const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		return OwnerCharacter->GetMesh();
	}

	return GetOwner()
		? GetOwner()->FindComponentByClass<USkeletalMeshComponent>()
		: nullptr;
}

bool USFEquipmentComponent::IsInventoryEntryEquipped(FGuid InventoryEntryId) const
{
	if (!InventoryEntryId.IsValid())
	{
		return false;
	}

	for (const TPair<ESFEquipmentSlot, FSFEquipmentSlotEntry>& Pair : EquippedSlots)
	{
		const FSFEquipmentSlotEntry& Entry = Pair.Value;
		if (Entry.bHasItemEquipped && Entry.InventoryEntryId == InventoryEntryId)
		{
			return true;
		}
	}

	return false;
}

bool USFEquipmentComponent::UnequipInventoryEntry(FGuid InventoryEntryId)
{
	if (!InventoryEntryId.IsValid())
	{
		return false;
	}

	for (const TPair<ESFEquipmentSlot, FSFEquipmentSlotEntry>& Pair : EquippedSlots)
	{
		const FSFEquipmentSlotEntry& Entry = Pair.Value;
		if (Entry.bHasItemEquipped && Entry.InventoryEntryId == InventoryEntryId)
		{
			return UnequipSlot(Pair.Key);
		}
	}

	return false;
}
void USFEquipmentComponent::GrantWeaponAbilitiesForSlot(ESFEquipmentSlot Slot, USFWeaponData* WeaponData)
{
	UE_LOG(LogTemp, Warning,
		TEXT("GrantWeaponAbilitiesForSlot: Slot=%d WeaponData=%s"),
		(int32)Slot,
		WeaponData ? *WeaponData->GetName() : TEXT("<NULL>"));

	if (!WeaponData)
	{
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("  PrimaryFireAbility=%s  SecondaryFireAbility=%s  ReloadAbility=%s  ExtraWeaponAbilities=%d"),
		WeaponData->PrimaryFireAbility ? *WeaponData->PrimaryFireAbility->GetName() : TEXT("<NULL>"),
		WeaponData->SecondaryFireAbility ? *WeaponData->SecondaryFireAbility->GetName() : TEXT("<NULL>"),
		WeaponData->ReloadAbility ? *WeaponData->ReloadAbility->GetName() : TEXT("<NULL>"),
		WeaponData->ExtraWeaponAbilities.Num());

	ASFCharacterBase* Character = Cast<ASFCharacterBase>(GetOwner());
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("  -> aborting: owner is not ASFCharacterBase"));
		return;
	}

	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	if (!ASC || !ASC->IsOwnerActorAuthoritative())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("  -> aborting: ASC=%p IsAuthoritative=%d (clients receive specs via replication)"),
			ASC,
			ASC ? (ASC->IsOwnerActorAuthoritative() ? 1 : 0) : 0);
		return;
	}

	TArray<FGameplayAbilitySpecHandle>& Handles = GrantedAbilityHandles.FindOrAdd(Slot);

	auto GrantOne = [&](TSubclassOf<UGameplayAbility> AbilityClass)
	{
		if (!AbilityClass)
		{
			return;
		}
		const FGameplayAbilitySpecHandle NewHandle = Character->GrantCharacterAbility(AbilityClass, 1);
		if (NewHandle.IsValid())
		{
			Handles.Add(NewHandle);
		}
	};

	GrantOne(WeaponData->PrimaryFireAbility);
	GrantOne(WeaponData->SecondaryFireAbility);
	GrantOne(WeaponData->ReloadAbility);
	for (const TSubclassOf<UGameplayAbility>& Extra : WeaponData->ExtraWeaponAbilities)
	{
		GrantOne(Extra);
	}
}

void USFEquipmentComponent::RemoveWeaponAbilitiesForSlot(ESFEquipmentSlot Slot)
{
	TArray<FGameplayAbilitySpecHandle>* Handles = GrantedAbilityHandles.Find(Slot);
	if (!Handles || Handles->Num() == 0)
	{
		GrantedAbilityHandles.Remove(Slot);
		return;
	}

	if (ASFCharacterBase* Character = Cast<ASFCharacterBase>(GetOwner()))
	{
		if (UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent())
		{
			if (ASC->IsOwnerActorAuthoritative())
			{
				for (const FGameplayAbilitySpecHandle& Handle : *Handles)
				{
					if (Handle.IsValid())
					{
						ASC->ClearAbility(Handle);
					}
				}
			}
		}
	}

	GrantedAbilityHandles.Remove(Slot);
}

bool USFEquipmentComponent::UpdateActiveWeaponInstance(const FSFWeaponInstanceData& UpdatedInstance)
{
	if (!CurrentWeaponInstance.IsValid())
	{
		return false;
	}

	if (UpdatedInstance.InstanceId != CurrentWeaponInstance.InstanceId)
	{
		// Different instance — reject. Callers should use EquipWeaponInstance to swap weapons.
		return false;
	}

	CurrentWeaponInstance = UpdatedInstance;

	USFWeaponData* WeaponData = CurrentWeaponInstance.WeaponDefinition.IsValid()
		? CurrentWeaponInstance.WeaponDefinition.Get()
		: CurrentWeaponInstance.WeaponDefinition.LoadSynchronous();

	OnEquippedWeaponChanged.Broadcast(WeaponData, CurrentWeaponInstance);
	BroadcastEquipmentUpdated();
	return true;
}

// ============================================================================
// Weapon switching + holstering
// ============================================================================

void USFEquipmentComponent::UpdateWeaponActorAttachmentForSlot(ESFEquipmentSlot Slot, bool bIsActive)
{
	if (Slot == ESFEquipmentSlot::None)
	{
		return;
	}

	const TObjectPtr<ASFWeaponActor>* Found = EquippedWeaponActors.Find(Slot);
	ASFWeaponActor* Actor = Found ? Found->Get() : nullptr;
	if (!Actor)
	{
		return;
	}

	USkeletalMeshComponent* OwnerMesh = GetOwnerSkeletalMesh();
	if (!OwnerMesh)
	{
		return;
	}

	const FSFEquipmentSlotEntry* Entry = EquippedSlots.Find(Slot);
	if (!Entry || !Entry->WeaponInstance.IsValid())
	{
		return;
	}

	USFWeaponData* WeaponData = Entry->WeaponInstance.WeaponDefinition.IsValid()
		? Entry->WeaponInstance.WeaponDefinition.Get()
		: Entry->WeaponInstance.WeaponDefinition.LoadSynchronous();
	if (!WeaponData)
	{
		return;
	}

	const bool bHasHolsterSocket = WeaponData->HolsteredAttachSocketName != NAME_None;
	const FName SocketName = (!bIsActive && bHasHolsterSocket)
		? WeaponData->HolsteredAttachSocketName
		: WeaponData->AttachSocketName;
	const FTransform& RelativeXform = (!bIsActive && bHasHolsterSocket)
		? WeaponData->HolsteredRelativeAttachTransform
		: WeaponData->RelativeAttachTransform;

	const FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, true);
	Actor->AttachToComponent(OwnerMesh, AttachRules, SocketName);
	Actor->SetActorRelativeTransform(RelativeXform);
}

bool USFEquipmentComponent::SwitchToWeaponSlot(ESFEquipmentSlot Slot)
{
	if (!IsWeaponSlot(Slot))
	{
		return false;
	}

	// Block re-entrant switches: if a swap is already in flight, ignore (could be debounced
	// later with a queue if designers want chainable cycle).
	if (bIsSwitchingWeapons)
	{
		return false;
	}

	// Same slot already active? No-op. (Pressing 1 while Primary is active = nothing.)
	if (ActiveWeaponSlot == Slot)
	{
		return false;
	}

	const FSFEquipmentSlotEntry Entry = GetEquipmentSlotEntry(Slot);
	if (!Entry.bHasItemEquipped || !Entry.WeaponInstance.IsValid())
	{
		// Empty slot — nothing to draw.
		return false;
	}

	USFWeaponData* TargetWeaponData = Entry.WeaponInstance.WeaponDefinition.IsValid()
		? Entry.WeaponInstance.WeaponDefinition.Get()
		: Entry.WeaponInstance.WeaponDefinition.LoadSynchronous();
	if (!TargetWeaponData)
	{
		return false;
	}

	ASFCharacterBase* Character = Cast<ASFCharacterBase>(GetOwner());
	UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;

	// While the swap is in flight, block fire / reload / ADS via State.Weapon.Switching.
	// Also pre-emptively yank the OUTGOING weapon's abilities so trigger-held full-auto stops.
	if (ActiveWeaponSlot != ESFEquipmentSlot::None)
	{
		RemoveWeaponAbilitiesForSlot(ActiveWeaponSlot);
	}

	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
	if (ASC && Tags.State_Weapon_Switching.IsValid())
	{
		ASC->AddLooseGameplayTag(Tags.State_Weapon_Switching, 1);
	}

	// Optional holster/draw montages — animation only, doesn't gate the timer.
	if (Character)
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			if (UAnimInstance* Anim = Mesh->GetAnimInstance())
			{
				// Holster outgoing weapon (if there was one).
				if (ActiveWeaponSlot != ESFEquipmentSlot::None)
				{
					if (USFWeaponData* OldData = GetCurrentWeaponData())
					{
						if (OldData->HolsterMontage)
						{
							Anim->Montage_Play(OldData->HolsterMontage, 1.0f);
						}
					}
				}
				// Draw incoming weapon.
				if (TargetWeaponData->DrawMontage)
				{
					Anim->Montage_Play(TargetWeaponData->DrawMontage, 1.0f);
				}
			}
		}
	}

	bIsSwitchingWeapons = true;
	bPendingSwitchIsHolster = false;
	PendingSwitchSlot = Slot;

	OnWeaponSwitchStarted.Broadcast(Slot, Entry.ItemDefinition, Entry.WeaponInstance);

	const float SwapTime = FMath::Max(0.0f, TargetWeaponData->SwapTimeSeconds);
	UWorld* World = GetWorld();
	if (SwapTime <= KINDA_SMALL_NUMBER || !World)
	{
		// Zero-delay path: fire the completion immediately.
		FinishWeaponSwitch();
		return true;
	}

	World->GetTimerManager().SetTimer(
		SwitchTimerHandle,
		FTimerDelegate::CreateUObject(this, &USFEquipmentComponent::FinishWeaponSwitch),
		SwapTime,
		false);
	return true;
}

bool USFEquipmentComponent::HolsterActiveWeapon()
{
	if (bIsSwitchingWeapons)
	{
		return false;
	}

	if (ActiveWeaponSlot == ESFEquipmentSlot::None)
	{
		return false;
	}

	const ESFEquipmentSlot SlotToHolster = ActiveWeaponSlot;
	const FSFEquipmentSlotEntry Entry = GetEquipmentSlotEntry(SlotToHolster);

	USFWeaponData* OldData = GetCurrentWeaponData();
	const float SwapTime = OldData ? FMath::Max(0.0f, OldData->SwapTimeSeconds) : 0.0f;

	ASFCharacterBase* Character = Cast<ASFCharacterBase>(GetOwner());
	UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;

	// Immediately revoke abilities so the soon-to-be-holstered weapon can't fire any more shots.
	RemoveWeaponAbilitiesForSlot(SlotToHolster);

	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();
	if (ASC && Tags.State_Weapon_Switching.IsValid())
	{
		ASC->AddLooseGameplayTag(Tags.State_Weapon_Switching, 1);
	}

	if (Character && OldData && OldData->HolsterMontage)
	{
		if (USkeletalMeshComponent* Mesh = Character->GetMesh())
		{
			if (UAnimInstance* Anim = Mesh->GetAnimInstance())
			{
				Anim->Montage_Play(OldData->HolsterMontage, 1.0f);
			}
		}
	}

	bIsSwitchingWeapons = true;
	bPendingSwitchIsHolster = true;
	PendingSwitchSlot = SlotToHolster;

	OnWeaponSwitchStarted.Broadcast(SlotToHolster, Entry.ItemDefinition, Entry.WeaponInstance);

	UWorld* World = GetWorld();
	if (SwapTime <= KINDA_SMALL_NUMBER || !World)
	{
		FinishWeaponSwitch();
		return true;
	}

	World->GetTimerManager().SetTimer(
		SwitchTimerHandle,
		FTimerDelegate::CreateUObject(this, &USFEquipmentComponent::FinishWeaponSwitch),
		SwapTime,
		false);
	return true;
}

void USFEquipmentComponent::FinishWeaponSwitch()
{
	const FSignalForgeGameplayTags& Tags = FSignalForgeGameplayTags::Get();

	ASFCharacterBase* Character = Cast<ASFCharacterBase>(GetOwner());
	UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;

	if (ASC && Tags.State_Weapon_Switching.IsValid())
	{
		ASC->RemoveLooseGameplayTag(Tags.State_Weapon_Switching, 1);
	}

	const ESFEquipmentSlot TargetSlot = PendingSwitchSlot;
	const bool bWasHolster = bPendingSwitchIsHolster;

	// Reset state up front so any reentrancy from broadcast callbacks works.
	bIsSwitchingWeapons = false;
	bPendingSwitchIsHolster = false;
	PendingSwitchSlot = ESFEquipmentSlot::None;
	SwitchTimerHandle.Invalidate();

	if (bWasHolster)
	{
		// Holster path: visually move the weapon to its holster socket and clear active.
		UpdateWeaponActorAttachmentForSlot(TargetSlot, /*bIsActive=*/false);

		ActiveWeaponSlot = ESFEquipmentSlot::None;
		CurrentWeaponInstance = FSFWeaponInstanceData();
		OnEquippedWeaponChanged.Broadcast(nullptr, FSFWeaponInstanceData());
		BroadcastEquipmentUpdated();

		const FSFEquipmentSlotEntry Entry = GetEquipmentSlotEntry(TargetSlot);
		OnWeaponSwitchCompleted.Broadcast(TargetSlot, Entry.ItemDefinition, Entry.WeaponInstance);
		return;
	}

	// Swap path: promote target slot to active. EquipWeaponInstance handles ability grant,
	// visual reattachment to hand socket, and broadcasts.
	const FSFEquipmentSlotEntry Entry = GetEquipmentSlotEntry(TargetSlot);
	if (Entry.bHasItemEquipped && Entry.WeaponInstance.IsValid())
	{
		EquipWeaponInstance(Entry.WeaponInstance, TargetSlot);
	}

	const FSFEquipmentSlotEntry FinalEntry = GetEquipmentSlotEntry(TargetSlot);
	OnWeaponSwitchCompleted.Broadcast(TargetSlot, FinalEntry.ItemDefinition, FinalEntry.WeaponInstance);
}
