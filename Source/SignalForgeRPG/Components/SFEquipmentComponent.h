#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Animation/SFAnimationTypes.h"
#include "Inventory/SFSlotTypes.h"
#include "Combat/SFWeaponInstanceTypes.h"
#include "Inventory/SFInventoryTypes.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayAbilitySpecHandle.h"
#include "SFEquipmentComponent.generated.h"

class UGameplayAbility;

class USFItemDefinition;
class USFWeaponData;
class ASFWeaponActor;
class USkeletalMeshComponent;
class USFInventoryComponent;

UENUM(BlueprintType)
enum class ESFEquipmentOpResult : uint8
{
	Success,
	InvalidItem,
	InvalidSlot,
	UnsupportedItemType,
	MissingWeaponData,
	AlreadyEquipped,
	EquipFailed,
	InvalidInventoryEntry
};

USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFEquipmentSlotEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	ESFEquipmentSlot Slot = ESFEquipmentSlot::None;

	/** Static item definition this slot is showing (weapon, armor, etc.). */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<USFItemDefinition> ItemDefinition = nullptr;

	/** Optional weapon instance data for weapon slots. */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	FSFWeaponInstanceData WeaponInstance;

	/** Optional link back to the owning inventory entry (if equipped from inventory). */
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	FGuid InventoryEntryId;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	bool bHasItemEquipped = false;

	FSFEquipmentSlotEntry() {}

	FSFEquipmentSlotEntry(ESFEquipmentSlot InSlot, USFItemDefinition* InItemDefinition)
		: Slot(InSlot)
		, ItemDefinition(InItemDefinition)
		, WeaponInstance()
		, InventoryEntryId()
		, bHasItemEquipped(InItemDefinition != nullptr)
	{
	}

	bool IsWeaponSlotEntry() const
	{
		return ItemDefinition != nullptr && WeaponInstance.IsValid();
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnEquippedWeaponChangedSignature,
	USFWeaponData*, NewWeaponData,
	FSFWeaponInstanceData, NewWeaponInstance);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquipmentUpdatedSignature);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnEquipmentSlotChangedSignature,
	ESFEquipmentSlot, Slot,
	USFItemDefinition*, ItemDefinition,
	FSFWeaponInstanceData, WeaponInstance);

UCLASS(ClassGroup = (Custom), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class SIGNALFORGERPG_API USFEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFEquipmentComponent();

	UFUNCTION(BlueprintPure, Category = "Equipment")
	ESFEquipmentSlot GetPreferredSlotForItem(USFItemDefinition* ItemDefinition) const
	{
		return ResolvePreferredSlotForItem(ItemDefinition);
	}

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	ESFEquipmentSlot ResolvePreferredSlotForItem(USFItemDefinition* ItemDefinition) const;

	// Default base weapon if no instance is provided.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<USFWeaponData> DefaultWeaponData = nullptr;

	// Active weapon instance driving combat/animation.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Equipment")
	FSFWeaponInstanceData CurrentWeaponInstance;

	// Deprecated: kept only for compatibility, consider removing.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<ASFWeaponActor> EquippedWeaponActor = nullptr;

	// Visual weapon actors per equipment slot (sword on hip, gun on back, etc.).
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Equipment")
	TMap<ESFEquipmentSlot, TObjectPtr<ASFWeaponActor>> EquippedWeaponActors;

	// Active weapon slot driving CurrentWeaponInstance and combat.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Equipment")
	ESFEquipmentSlot ActiveWeaponSlot = ESFEquipmentSlot::None;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Equipment")
	TMap<ESFEquipmentSlot, FSFEquipmentSlotEntry> EquippedSlots;

	/**
	 * Tracks ability spec handles granted by a weapon currently occupying a slot.
	 * On unequip we clear these handles so the weapon's primary/secondary/reload/extra abilities
	 * no longer linger on the owner's ASC.
	 */
	TMap<ESFEquipmentSlot, TArray<FGameplayAbilitySpecHandle>> GrantedAbilityHandles;

public:
	/** Equip a raw weapon data asset as the active weapon (no instance perks/rolls). */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool EquipWeaponData(USFWeaponData* NewWeaponData, ESFEquipmentSlot InSlot = ESFEquipmentSlot::None);

	/** Equip a full weapon instance (definition + roll/perks) as active weapon. */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool EquipWeaponInstance(const FSFWeaponInstanceData& WeaponInstance, ESFEquipmentSlot InSlot = ESFEquipmentSlot::None);

	/** Equip an item definition using preferred slot resolution (non-instance path). */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	ESFEquipmentOpResult EquipItemDefinition(USFItemDefinition* ItemDefinition);

	/** Equip an item definition to a specific slot (non-instance path). */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	ESFEquipmentOpResult EquipItemToSlot(USFItemDefinition* ItemDefinition, ESFEquipmentSlot Slot);

	/** Equip an item + weapon instance to a specific slot (preferred for weapons). */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	ESFEquipmentOpResult EquipItemInstanceToSlot(USFItemDefinition* ItemDefinition, const FSFWeaponInstanceData& WeaponInstance, ESFEquipmentSlot Slot);

	/** Equip directly from an inventory entry (definition + instance) into a slot (or preferred slot). */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	ESFEquipmentOpResult EquipFromInventoryEntry(const FGuid& InventoryEntryId, ESFEquipmentSlot SlotOverride = ESFEquipmentSlot::None);

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool UnequipSlot(ESFEquipmentSlot Slot);

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void ClearEquippedWeapon();

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void ClearAllEquipment();

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void SetActiveWeaponSlot(ESFEquipmentSlot Slot);

	/** Convenience: get current weapon definition from the active instance. */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	USFWeaponData* GetCurrentWeaponData() const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	FSFWeaponInstanceData GetCurrentWeaponInstance() const
	{
		return CurrentWeaponInstance;
	}

	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool HasEquippedWeapon() const
	{
		return CurrentWeaponInstance.IsValid();
	}

	UFUNCTION(BlueprintPure, Category = "Equipment")
	ESFEquipmentSlot GetActiveWeaponSlot() const
	{
		return ActiveWeaponSlot;
	}

	/**
	 * Lightweight in-place update of the active weapon's instance data (e.g. ammo count
	 * changing after firing). Does not re-equip, re-grant abilities, or re-spawn the
	 * weapon actor. Broadcasts OnEquippedWeaponChanged so HUDs/widgets can refresh.
	 *
	 * The instance must match the currently-active weapon by InstanceId, otherwise the
	 * call is rejected to prevent accidental cross-weapon overwrites.
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool UpdateActiveWeaponInstance(const FSFWeaponInstanceData& UpdatedInstance);

	// For compatibility; returns the actor for the active weapon slot if any.
	UFUNCTION(BlueprintPure, Category = "Equipment")
	ASFWeaponActor* GetEquippedWeaponActor() const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	ASFWeaponActor* GetEquippedWeaponActorForSlot(ESFEquipmentSlot Slot) const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool HasItemEquippedInSlot(ESFEquipmentSlot Slot) const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	USFItemDefinition* GetEquippedItemInSlot(ESFEquipmentSlot Slot) const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	FSFWeaponInstanceData GetEquippedWeaponInstanceInSlot(ESFEquipmentSlot Slot) const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	FSFEquipmentSlotEntry GetEquipmentSlotEntry(ESFEquipmentSlot Slot) const;

	const TMap<ESFEquipmentSlot, FSFEquipmentSlotEntry>& GetEquippedSlots() const
	{
		return EquippedSlots;
	}

	const TMap<ESFEquipmentSlot, TObjectPtr<ASFWeaponActor>>& GetEquippedWeaponActors() const
	{
		return EquippedWeaponActors;
	}

	/** Total weight of all equipped items. */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	float GetTotalEquippedWeight() const;

	/** Get all equipped items of a given high-level item type (e.g. Weapon, Armor). */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	void GetEquippedItemsOfType(ESFItemType ItemType, TArray<FSFEquipmentSlotEntry>& OutEntries) const;

	/** Get all equipped weapons (any weapon slot). */
	UFUNCTION(BlueprintPure, Category = "Equipment")
	void GetEquippedWeapons(TArray<FSFEquipmentSlotEntry>& OutEntries) const;

	// Animation helpers

	UFUNCTION(BlueprintPure, Category = "Equipment|Animation")
	FSFWeaponAnimationProfile GetCurrentAnimationProfile() const;

	UFUNCTION(BlueprintPure, Category = "Equipment|Animation")
	ESFOverlayMode GetCurrentOverlayMode() const;

	UFUNCTION(BlueprintPure, Category = "Equipment|Animation")
	ESFCombatMode GetCurrentCombatMode() const;

	UFUNCTION(BlueprintPure, Category = "Equipment|Animation")
	UAnimMontage* GetCurrentLightAttackMontage() const;

	UFUNCTION(BlueprintPure, Category = "Equipment|Animation")
	UAnimMontage* GetCurrentHeavyAttackMontage() const;

	UFUNCTION(BlueprintPure, Category = "Equipment|Animation")
	UAnimMontage* GetCurrentAbilityMontage() const;

	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FOnEquippedWeaponChangedSignature OnEquippedWeaponChanged;

	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FOnEquipmentUpdatedSignature OnEquipmentUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FOnEquipmentSlotChangedSignature OnEquipmentSlotChanged;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool IsInventoryEntryEquipped(FGuid InventoryEntryId) const;

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool UnequipInventoryEntry(FGuid InventoryEntryId);

	/*UFUNCTION(BlueprintPure, Category = "Equipment")
	USFWeaponData* GetEquippedWeaponDataInSlot(ESFEquipmentSlot Slot) const;*/

protected:
	// Legacy: operates on EquippedWeaponActor + CurrentWeaponInstance's definition for the active weapon.
	void RefreshEquippedWeaponActor();
	void DestroyEquippedWeaponActor();

	// New: per-slot visual actors.
	void RefreshEquippedWeaponActorForSlot(ESFEquipmentSlot Slot, USFWeaponData* WeaponData);
	void DestroyEquippedWeaponActorForSlot(ESFEquipmentSlot Slot);

	void BroadcastEquipmentUpdated();

	/** Internal helper to set slot entry, including optional inventory entry link. */
	void SetSlotEntry(
		ESFEquipmentSlot Slot,
		USFItemDefinition* ItemDefinition,
		const FSFWeaponInstanceData* WeaponInstance = nullptr,
		const FGuid* InventoryEntryId = nullptr);

	bool IsWeaponSlot(ESFEquipmentSlot Slot) const;

	/** Grant weapon-defined abilities onto the owning character's ASC. Tracked per slot. */
	void GrantWeaponAbilitiesForSlot(ESFEquipmentSlot Slot, USFWeaponData* WeaponData);

	/** Remove any weapon-defined abilities granted for the given slot. */
	void RemoveWeaponAbilitiesForSlot(ESFEquipmentSlot Slot);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	bool bEquipDefaultWeaponOnBeginPlay = true;

	USkeletalMeshComponent* GetOwnerSkeletalMesh() const;
};