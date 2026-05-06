#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "Inventory/SFItemDefinition.h"
#include "SFItemInstance.generated.h"

class USFItemDefinition;
class USFWeaponData;
class USFInventoryComponent; // your inventory comp, assumed

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class SIGNALFORGERPG_API USFItemInstance : public UObject
{
	GENERATED_BODY()

public:
	USFItemInstance();

	/** Static definition backing this instance. Immutable shared data. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<USFItemDefinition> ItemDef;

	/** Owning inventory component (not replicated, owned by it). */
	UPROPERTY()
	TObjectPtr<USFInventoryComponent> OwningInventory;

	/** Unique runtime ID (per inventory) if you need stable references. */
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	int32 RuntimeId = INDEX_NONE;

	/** Current stack size for this entry. */
	UPROPERTY(BlueprintReadOnly, Category = "Item|State")
	int32 Quantity = 1;

	/** Current durability (0..MaxDurability from definition). */
	UPROPERTY(BlueprintReadOnly, Category = "Item|State")
	int32 CurrentDurability = 0;

	/** Player/item level for scaling (if you want level-based rolls). */
	UPROPERTY(BlueprintReadOnly, Category = "Item|State")
	int32 ItemLevel = 1;

	/** Whether this particular instance is favorited/pinned in UI. */
	UPROPERTY(BlueprintReadOnly, Category = "Item|State")
	bool bFavorite = false;

	/** Whether this item is currently equipped/active (weapon, armor, etc.). */
	UPROPERTY(BlueprintReadOnly, Category = "Item|State")
	bool bEquipped = false;

	/** Time this item was last used (game time seconds). */
	UPROPERTY(BlueprintReadOnly, Category = "Item|State")
	float LastUseTime = -FLT_MAX;

	/** Arbitrary tags you want at instance level (e.g. rolled traits). */
	UPROPERTY(BlueprintReadOnly, Category = "Item|State")
	FGameplayTagContainer InstanceTags;

	/** Optional: encoded roll/perk data for this copy (weapon affixes, etc.). */
	// UPROPERTY(BlueprintReadOnly, Category = "Item|State")
	// FSFItemRollData RollData;

	// ------------------ Initialization ------------------

	UFUNCTION(BlueprintCallable, Category = "Item")
	void Initialize(USFItemDefinition* InDefinition, int32 InQuantity = 1, int32 InItemLevel = 1);

	// ------------------ Definition passthroughs ------------------

	UFUNCTION(BlueprintPure, Category = "Item")
	bool IsValidInstance() const;

	UFUNCTION(BlueprintPure, Category = "Item")
	FName GetItemId() const;

	UFUNCTION(BlueprintPure, Category = "Item")
	FText GetDisplayName() const;

	UFUNCTION(BlueprintPure, Category = "Item")
	FText GetDescription() const;

	UFUNCTION(BlueprintPure, Category = "Item")
	ESFItemType GetItemType() const;

	UFUNCTION(BlueprintPure, Category = "Item")
	ESFItemRarity GetRarity() const;

	UFUNCTION(BlueprintPure, Category = "Item")
	const FGameplayTagContainer& GetDefinitionTags() const;

	UFUNCTION(BlueprintPure, Category = "Item")
	bool HasItemTag(FGameplayTag TagToMatch, bool bExactMatch = false) const;

	UFUNCTION(BlueprintPure, Category = "Item")
	int32 GetEffectiveMaxStackSize() const;

	UFUNCTION(BlueprintPure, Category = "Item")
	bool CanStackWith(const USFItemInstance* Other) const;

	UFUNCTION(BlueprintPure, Category = "Item")
	float GetSingleItemWeight() const;

	UFUNCTION(BlueprintPure, Category = "Item")
	float GetStackWeight() const;

	UFUNCTION(BlueprintPure, Category = "Item")
	bool IsWeapon() const;

	UFUNCTION(BlueprintPure, Category = "Item")
	USFWeaponData* GetWeaponData() const;

	// ------------------ State operations ------------------

	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetQuantity(int32 NewQuantity);

	UFUNCTION(BlueprintCallable, Category = "Item")
	bool TryAddToStack(int32 Amount, int32& OutActuallyAdded);

	UFUNCTION(BlueprintCallable, Category = "Item")
	bool TryRemoveFromStack(int32 Amount, int32& OutActuallyRemoved);

	UFUNCTION(BlueprintPure, Category = "Item")
	bool IsStackFull() const;

	UFUNCTION(BlueprintPure, Category = "Item")
	int32 GetStackSpace() const;

	UFUNCTION(BlueprintPure, Category = "Item")
	bool HasDurability() const;

	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetCurrentDurability(int32 NewDurability);

	UFUNCTION(BlueprintPure, Category = "Item")
	bool IsBroken() const;

	UFUNCTION(BlueprintPure, Category = "Item")
	bool CanUse() const;

	UFUNCTION(BlueprintPure, Category = "Item")
	bool CanDrop() const;

	UFUNCTION(BlueprintPure, Category = "Item")
	bool CanSell() const;

	UFUNCTION(BlueprintPure, Category = "Item")
	bool CanDestroy() const;

	/** For consumables/weapons to check cooldowns, etc. */
	UFUNCTION(BlueprintPure, Category = "Item")
	bool IsOnCooldown(float CurrentTime, float CooldownDuration) const;

	// ------------------ Usage hooks (for BP) ------------------

	/** Called when this instance is used (e.g. consumed, equipped, etc.). */
	UFUNCTION(BlueprintNativeEvent, Category = "Item|Usage")
	void OnUsed(AActor* User);
	virtual void OnUsed_Implementation(AActor* User);

	/** Called when equipped/unequipped. */
	UFUNCTION(BlueprintNativeEvent, Category = "Item|Usage")
	void OnEquipped(AActor* User);
	virtual void OnEquipped_Implementation(AActor* User);

	UFUNCTION(BlueprintNativeEvent, Category = "Item|Usage")
	void OnUnequipped(AActor* User);
	virtual void OnUnequipped_Implementation(AActor* User);
};