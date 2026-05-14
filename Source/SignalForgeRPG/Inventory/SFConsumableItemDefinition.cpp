#include "Inventory/SFConsumableItemDefinition.h"

#include "Inventory/SFItemTypes.h"

USFConsumableItemDefinition::USFConsumableItemDefinition()
{
	// Bake sensible defaults so a freshly-created asset is already a valid
	// consumable -- designers only need to assign ConsumeEffect.
	ItemType = ESFItemType::Consumable;
	bIsConsumable = true;
	bStackable = true;
	MaxStackSize = 10;
	bUniqueInstance = false;
}
