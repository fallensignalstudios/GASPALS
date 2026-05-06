#include "Inventory/SFItemInstance.h"
#include "Inventory/SFItemDefinition.h"
#include "Combat/SFWeaponData.h"
// #include "Inventory/SFInventoryComponent.h"

USFItemInstance::USFItemInstance()
{
	Quantity = 1;
	ItemLevel = 1;
	CurrentDurability = 0;
	LastUseTime = -FLT_MAX;
}

void USFItemInstance::Initialize(USFItemDefinition* InDefinition, int32 InQuantity, int32 InItemLevel)
{
	ItemDef = InDefinition;
	ItemLevel = FMath::Max(1, InItemLevel);

	if (ItemDef)
	{
		const int32 MaxStack = ItemDef->GetEffectiveMaxStackSize();
		Quantity = FMath::Clamp(InQuantity, 1, MaxStack);
		CurrentDurability = ItemDef->MaxDurability;
	}
	else
	{
		Quantity = 0;
		CurrentDurability = 0;
	}
}

bool USFItemInstance::IsValidInstance() const
{
	return ItemDef != nullptr && ItemDef->IsDefinitionValid();
}

FName USFItemInstance::GetItemId() const
{
	return ItemDef ? ItemDef->ItemId : NAME_None;
}

FText USFItemInstance::GetDisplayName() const
{
	return ItemDef ? ItemDef->DisplayName : FText::GetEmpty();
}

FText USFItemInstance::GetDescription() const
{
	return ItemDef ? ItemDef->Description : FText::GetEmpty();
}

ESFItemType USFItemInstance::GetItemType() const
{
	return ItemDef ? ItemDef->ItemType : ESFItemType::None;
}

ESFItemRarity USFItemInstance::GetRarity() const
{
	return ItemDef ? ItemDef->Rarity : ESFItemRarity::Common;
}

const FGameplayTagContainer& USFItemInstance::GetDefinitionTags() const
{
	static FGameplayTagContainer EmptyTags;
	return ItemDef ? ItemDef->ItemTags : EmptyTags;
}

bool USFItemInstance::HasItemTag(FGameplayTag TagToMatch, bool bExactMatch) const
{
	return ItemDef ? ItemDef->HasItemTag(TagToMatch, bExactMatch) : false;
}

int32 USFItemInstance::GetEffectiveMaxStackSize() const
{
	return ItemDef ? ItemDef->GetEffectiveMaxStackSize() : 1;
}

bool USFItemInstance::CanStackWith(const USFItemInstance* Other) const
{
	if (!Other || !ItemDef || Other->ItemDef != ItemDef)
	{
		return false;
	}

	// For weapons/unique instances, GetEffectiveMaxStackSize will be 1 anyway.
	if (!ItemDef->CanStack())
	{
		return false;
	}

	// Optional: require same ItemLevel or some tag parity for stacking.
	return true;
}

float USFItemInstance::GetSingleItemWeight() const
{
	return ItemDef ? ItemDef->Weight : 0.0f;
}

float USFItemInstance::GetStackWeight() const
{
	return GetSingleItemWeight() * Quantity;
}

bool USFItemInstance::IsWeapon() const
{
	return ItemDef && ItemDef->IsWeapon();
}

USFWeaponData* USFItemInstance::GetWeaponData() const
{
	return ItemDef ? ItemDef->GetWeaponData() : nullptr;
}

// ------------------ State operations ------------------

void USFItemInstance::SetQuantity(int32 NewQuantity)
{
	if (!ItemDef)
	{
		Quantity = 0;
		return;
	}

	const int32 MaxStack = GetEffectiveMaxStackSize();
	Quantity = FMath::Clamp(NewQuantity, 0, MaxStack);
}

bool USFItemInstance::TryAddToStack(int32 Amount, int32& OutActuallyAdded)
{
	OutActuallyAdded = 0;
	if (!ItemDef || Amount <= 0)
	{
		return false;
	}

	const int32 MaxStack = GetEffectiveMaxStackSize();
	const int32 Space = MaxStack - Quantity;
	if (Space <= 0)
	{
		return false;
	}

	const int32 Delta = FMath::Min(Amount, Space);
	Quantity += Delta;
	OutActuallyAdded = Delta;
	return Delta > 0;
}

bool USFItemInstance::TryRemoveFromStack(int32 Amount, int32& OutActuallyRemoved)
{
	OutActuallyRemoved = 0;
	if (Amount <= 0 || Quantity <= 0)
	{
		return false;
	}

	const int32 Delta = FMath::Min(Amount, Quantity);
	Quantity -= Delta;
	OutActuallyRemoved = Delta;
	return Delta > 0;
}

bool USFItemInstance::IsStackFull() const
{
	return Quantity >= GetEffectiveMaxStackSize();
}

int32 USFItemInstance::GetStackSpace() const
{
	return GetEffectiveMaxStackSize() - Quantity;
}

bool USFItemInstance::HasDurability() const
{
	return ItemDef && ItemDef->MaxDurability > 0;
}

void USFItemInstance::SetCurrentDurability(int32 NewDurability)
{
	if (!ItemDef || !HasDurability())
	{
		CurrentDurability = 0;
		return;
	}

	CurrentDurability = FMath::Clamp(NewDurability, 0, ItemDef->MaxDurability);
}

bool USFItemInstance::IsBroken() const
{
	return HasDurability() && CurrentDurability <= 0;
}

bool USFItemInstance::CanUse() const
{
	if (!IsValidInstance())
	{
		return false;
	}

	if (Quantity <= 0)
	{
		return false;
	}

	if (IsBroken())
	{
		return false;
	}

	if (ItemDef->RequiredLevel > 0)
	{
		// You can compare against owning character's level outside this class,
		// or inject it into ItemLevel and check here.
	}

	return true;
}

bool USFItemInstance::CanDrop() const
{
	return ItemDef ? ItemDef->bCanDrop : false;
}

bool USFItemInstance::CanSell() const
{
	return ItemDef ? ItemDef->bCanSell : false;
}

bool USFItemInstance::CanDestroy() const
{
	return ItemDef ? ItemDef->bCanDestroy : false;
}

bool USFItemInstance::IsOnCooldown(float CurrentTime, float CooldownDuration) const
{
	if (CooldownDuration <= 0.f)
	{
		return false;
	}

	return (CurrentTime - LastUseTime) < CooldownDuration;
}

// ------------------ Usage hooks ------------------

void USFItemInstance::OnUsed_Implementation(AActor* User)
{
	// Intentionally empty by default, override in BP or subclasses.
}

void USFItemInstance::OnEquipped_Implementation(AActor* User)
{
}

void USFItemInstance::OnUnequipped_Implementation(AActor* User)
{
}