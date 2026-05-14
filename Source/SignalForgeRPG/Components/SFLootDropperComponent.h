#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SFLootDropperComponent.generated.h"

class ASFCharacterBase;
class ASFWorldItemActor;
class USFLootTable;

UENUM(BlueprintType)
enum class ESFLootDeliveryMode : uint8
{
	/**
	 * Spawn one world pickup actor per loot roll at the dying character's
	 * location, in a tight ring around them. Player walks over / interacts
	 * to collect. The "Dark Souls / Diablo" feel.
	 */
	SpawnInWorld,

	/**
	 * Push the rolls directly into the killer's inventory, bypassing any
	 * world pickup actors. Good for streamlined loot ("you killed an
	 * enemy and gained 50 credits") or for AI-killed enemies whose loot
	 * shouldn't litter the level.
	 */
	DirectToKiller,

	/**
	 * Try DirectToKiller first; if the killer has no inventory component
	 * or doesn't accept the item (full bag), fall back to SpawnInWorld
	 * so loot is never silently lost.
	 */
	DirectThenWorld
};

/**
 * USFLootDropperComponent
 *
 * Attach to any actor (typically ASFEnemyCharacter or ASFCharacterBase
 * subclasses) that should drop loot when it dies. Auto-binds to the
 * owning character's OnCharacterDied delegate and rolls the assigned
 * USFLootTable on broadcast.
 *
 * Configuration is data-only: pick a loot table, choose a delivery mode,
 * optionally override the world pickup actor class. No subclassing or
 * BP wiring required on the enemy itself.
 *
 * Net-authority: drops only execute on the server (HasAuthority on the
 * owning actor). Replication of the resulting pickup actors / inventory
 * adds is handled by their own systems.
 */
UCLASS(ClassGroup = (Custom), BlueprintType, meta = (BlueprintSpawnableComponent))
class SIGNALFORGERPG_API USFLootDropperComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFLootDropperComponent();

	/**
	 * Manually trigger a drop. Normally called automatically via the
	 * owner's OnCharacterDied delegate; exposed publicly so chests and
	 * destructibles can re-use the same dropper component without being
	 * SFCharacterBase subclasses.
	 *
	 * Killer may be nullptr -- DirectToKiller / DirectThenWorld will then
	 * behave like SpawnInWorld since there's no recipient.
	 */
	UFUNCTION(BlueprintCallable, Category = "Loot")
	void DropLoot(AActor* Killer);

	UFUNCTION(BlueprintPure, Category = "Loot")
	USFLootTable* GetLootTable() const { return LootTable; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * The loot table this dropper rolls on trigger. nullptr is legal
	 * (component does nothing) so designers can attach the component as
	 * part of an enemy archetype prefab and fill the table later.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<USFLootTable> LootTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	ESFLootDeliveryMode DeliveryMode = ESFLootDeliveryMode::DirectThenWorld;

	/**
	 * World pickup actor class spawned in SpawnInWorld / fallback paths.
	 * Falls back to the C++ ASFWorldItemActor if unset, so loot drops
	 * still work before designers create BP_WorldItemActor.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|World")
	TSubclassOf<ASFWorldItemActor> WorldItemActorClass;

	/**
	 * Vertical offset added to the spawn transform so pickups don't clip
	 * into the floor mesh. ~40 units is a good default for player-scale
	 * enemies; bump up for bosses.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|World")
	float SpawnHeightOffset = 40.0f;

	/**
	 * Horizontal radius pickups are scattered in around the death point.
	 * 0 = stacked on top of each other; ~80 spreads them in a small ring
	 * so a 3-item drop reads as three distinct world items.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|World", meta = (ClampMin = "0.0"))
	float SpawnScatterRadius = 80.0f;

	/**
	 * If set, RollDrops is given this seed for deterministic drops -- useful
	 * for boss encounters that should always drop the same item or for
	 * test maps. 0 = use a fresh random seed each death (the default).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|Advanced")
	int32 DeterministicSeed = 0;

	/**
	 * If true, the dropper unsubscribes from OnCharacterDied after firing
	 * once. Default true -- characters generally don't die twice, and the
	 * extra guard prevents respawn / revival flows from re-dropping loot.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot|Advanced")
	bool bDropOnlyOnce = true;

	/** Cached owner cast for the death-binding path. May be nullptr if owner isn't a character. */
	UPROPERTY()
	TObjectPtr<ASFCharacterBase> OwnerCharacter;

	bool bHasDropped = false;

	UFUNCTION()
	void HandleOwnerDied(ASFCharacterBase* DeadCharacter, ASFCharacterBase* Killer);
};
