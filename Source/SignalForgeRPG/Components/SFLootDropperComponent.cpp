#include "Components/SFLootDropperComponent.h"

#include "Characters/SFCharacterBase.h"
#include "Components/SFInventoryComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Inventory/SFLootTable.h"
#include "Inventory/SFWorldItemActor.h"
#include "Math/RandomStream.h"

USFLootDropperComponent::USFLootDropperComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USFLootDropperComponent::BeginPlay()
{
	Super::BeginPlay();

	// Auto-bind to the owner's death delegate. We only care if the owner
	// is an SFCharacterBase -- chests / destructibles call DropLoot()
	// manually from their own code paths instead of relying on the bind.
	OwnerCharacter = Cast<ASFCharacterBase>(GetOwner());
	if (OwnerCharacter)
	{
		OwnerCharacter->OnCharacterDied.AddDynamic(this, &USFLootDropperComponent::HandleOwnerDied);
	}
}

void USFLootDropperComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (OwnerCharacter)
	{
		OwnerCharacter->OnCharacterDied.RemoveDynamic(this, &USFLootDropperComponent::HandleOwnerDied);
		OwnerCharacter = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void USFLootDropperComponent::HandleOwnerDied(
	ASFCharacterBase* /*DeadCharacter*/,
	ASFCharacterBase* Killer)
{
	DropLoot(Killer);
}

void USFLootDropperComponent::DropLoot(AActor* Killer)
{
	// Net authority gate -- world spawns and inventory adds happen server-
	// side. Clients see the resulting actors / inventory changes through
	// existing replication paths on those systems.
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	if (bDropOnlyOnce && bHasDropped)
	{
		return;
	}
	if (!LootTable)
	{
		return;
	}

	const TArray<FSFLootRoll> Rolls = LootTable->RollDrops(DeterministicSeed);
	if (Rolls.Num() == 0)
	{
		bHasDropped = true;
		return;
	}

	// For scatter we share one stream so the offsets read as deterministic
	// jitter when DeterministicSeed is set. When seed==0 we re-seed per
	// drop so two enemies dying in the same frame don't pile up on the
	// same scatter direction.
	FRandomStream Scatter;
	if (DeterministicSeed != 0)
	{
		Scatter.Initialize(DeterministicSeed + 7919); // arbitrary co-prime
	}
	else
	{
		Scatter.GenerateNewSeed();
	}

	// Try inventory pathway. If the killer has no inventory we'll fall
	// back to world spawns regardless of mode. Cache the inventory once
	// here instead of per roll to avoid repeated component lookups.
	USFInventoryComponent* KillerInventory = nullptr;
	if (Killer && (DeliveryMode == ESFLootDeliveryMode::DirectToKiller ||
		DeliveryMode == ESFLootDeliveryMode::DirectThenWorld))
	{
		KillerInventory = Killer->FindComponentByClass<USFInventoryComponent>();
	}

	const FVector BasePos = Owner->GetActorLocation() + FVector(0.0f, 0.0f, SpawnHeightOffset);

	for (const FSFLootRoll& Roll : Rolls)
	{
		if (!Roll.ItemDefinition || Roll.Quantity <= 0)
		{
			continue;
		}

		int32 RemainingQty = Roll.Quantity;

		// Phase A -- direct deposit into the killer's inventory if mode
		// allows it. Captures the partial-accept case so any overflow
		// can still spawn in the world below.
		if (KillerInventory && DeliveryMode != ESFLootDeliveryMode::SpawnInWorld)
		{
			const FSFInventoryAddResult AddResult = KillerInventory->AddItem(Roll.ItemDefinition, RemainingQty);
			RemainingQty = FMath::Max(0, RemainingQty - AddResult.AddedQuantity);

			if (RemainingQty == 0)
			{
				continue;
			}

			// DirectToKiller drops anything that didn't fit -- mirrors the
			// "your bag is full" pattern. DirectThenWorld lets the rest
			// fall through to the world spawn below.
			if (DeliveryMode == ESFLootDeliveryMode::DirectToKiller)
			{
				continue;
			}
		}

		// Phase B -- spawn world pickup actors for whatever is left.
		// One actor per roll keeps the visual count accurate; stack-merge
		// happens automatically when the player picks them up.
		const float Angle = Scatter.FRand() * 2.0f * PI;
		const float Dist = Scatter.FRand() * SpawnScatterRadius;
		const FVector ScatterOffset(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.0f);
		const FTransform SpawnXform(FRotator(0.0f, Scatter.FRand() * 360.0f, 0.0f), BasePos + ScatterOffset);

		ASFWorldItemActor::SpawnWorldItem(
			this,
			WorldItemActorClass,
			Roll.ItemDefinition,
			RemainingQty,
			SpawnXform);
	}

	bHasDropped = true;
}
