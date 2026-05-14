#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SFInteractableInterface.h"
#include "SFWorldItemActor.generated.h"

class USFItemDefinition;
class UStaticMeshComponent;
class USphereComponent;

/**
 * ASFWorldItemActor
 *
 * Visible, interactable representation of an item lying in the world.
 * Spawned by USFLootDropperComponent on enemy death, by chests, by
 * scripted "place a pickup here" level setup, or by anything else that
 * wants a player-grabbable item.
 *
 * Configuration is just an item definition + quantity. The actor reads
 * the definition's pickup mesh (or falls back to the world mesh) on
 * BeginPlay so a single C++ class supports every item type without
 * needing a Blueprint subclass per item.
 *
 * Interact path:
 *   Player's USFInteractionComponent calls Interact() with a context that
 *   includes the interacting actor. We pull the inventory component off
 *   that actor, call AddItem with our quantity, and reduce our internal
 *   quantity by however much was actually accepted. If anything is left
 *   (inventory full) we stay in the world; if everything fit we destroy.
 *
 * The actor does NOT auto-pickup on overlap by default -- player has to
 * press the interact key. Subclasses or designers can flip
 * bAutoPickupOnOverlap on for ammo / currency that should just vacuum.
 */
UCLASS(BlueprintType, Blueprintable)
class SIGNALFORGERPG_API ASFWorldItemActor : public AActor, public ISFInteractableInterface
{
	GENERATED_BODY()

public:
	ASFWorldItemActor();

	/**
	 * Static constructor used by the loot system. Spawns + configures a
	 * world item in one call so the dropper doesn't have to know about
	 * our component layout.
	 *
	 * Returns nullptr if World, ItemDefinition, or Quantity are invalid.
	 */
	UFUNCTION(BlueprintCallable, Category = "WorldItem", meta = (WorldContext = "WorldContextObject"))
	static ASFWorldItemActor* SpawnWorldItem(
		const UObject* WorldContextObject,
		TSubclassOf<ASFWorldItemActor> ActorClass,
		USFItemDefinition* ItemDefinition,
		int32 Quantity,
		const FTransform& SpawnTransform);

	/**
	 * Programmatic setter for the payload. Normally called immediately
	 * after spawn; safe to call at runtime but won't refresh visuals if
	 * called after BeginPlay (designers can call RefreshMeshFromDefinition
	 * if they need a live swap).
	 */
	UFUNCTION(BlueprintCallable, Category = "WorldItem")
	void InitializePayload(USFItemDefinition* InItemDefinition, int32 InQuantity);

	UFUNCTION(BlueprintPure, Category = "WorldItem")
	USFItemDefinition* GetItemDefinition() const { return ItemDefinition; }

	UFUNCTION(BlueprintPure, Category = "WorldItem")
	int32 GetQuantity() const { return Quantity; }

	/** Reads the mesh + scale from the assigned item definition. */
	UFUNCTION(BlueprintCallable, Category = "WorldItem")
	void RefreshMeshFromDefinition();

	// -------------------------------------------------------------------------
	// ISFInteractableInterface
	// -------------------------------------------------------------------------

	virtual ESFInteractionAvailability GetInteractionAvailability_Implementation(
		const FSFInteractionContext& InteractionContext) const override;

	virtual FSFInteractionOption GetPrimaryInteractionOption_Implementation(
		const FSFInteractionContext& InteractionContext) const override;

	virtual TArray<FSFInteractionOption> GetInteractionOptions_Implementation(
		const FSFInteractionContext& InteractionContext) const override;

	virtual FSFInteractionExecutionResult Interact_Implementation(
		const FSFInteractionContext& InteractionContext) override;

	virtual FSFInteractionExecutionResult InteractWithOption_Implementation(
		const FSFInteractionContext& InteractionContext,
		FName OptionId) override;

	virtual void BeginInteractionFocus_Implementation(
		const FSFInteractionContext& InteractionContext) override;

	virtual void EndInteractionFocus_Implementation(
		const FSFInteractionContext& InteractionContext) override;

	virtual FVector GetInteractionLocation_Implementation(
		const FSFInteractionContext& InteractionContext) const override;

	virtual float GetInteractionRange_Implementation(
		const FSFInteractionContext& InteractionContext) const override;

	virtual bool CanInteract_Implementation(
		const FSFInteractionContext& InteractionContext) const override;

	virtual FText GetInteractionPromptText_Implementation(
		const FSFInteractionContext& InteractionContext) const override;

protected:
	virtual void BeginPlay() override;

	/**
	 * Auto-vacuum mode: when an overlap with a SFCharacterBase fires, we
	 * call TryPickupBy directly without waiting for an interact key. Use
	 * for ammo / credits / generic resources. Default off so most items
	 * require an explicit interact.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldItem|Behavior")
	bool bAutoPickupOnOverlap = false;

	/**
	 * Radius of the overlap sphere used for auto-pickup and as the focus
	 * volume for the interaction system. Doesn't affect the visible mesh.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldItem|Behavior", meta = (ClampMin = "10.0"))
	float PickupRadius = 96.0f;

	/**
	 * Interaction range reported to ISFInteractableInterface consumers.
	 * Larger than PickupRadius so the prompt appears slightly before the
	 * player is on top of the item.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldItem|Behavior", meta = (ClampMin = "10.0"))
	float InteractionRange = 220.0f;

	/** Item this pickup represents. Required. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WorldItem")
	TObjectPtr<USFItemDefinition> ItemDefinition;

	/** Remaining quantity. Decreases on partial pickups; actor destroys at 0. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WorldItem", meta = (ClampMin = "1"))
	int32 Quantity = 1;

	/** Visible mesh. Filled from ItemDefinition on BeginPlay. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WorldItem|Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** Overlap volume for focus / auto-pickup. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "WorldItem|Components")
	TObjectPtr<USphereComponent> PickupSphere;

	/**
	 * Common pickup path used by both manual Interact and auto-vacuum.
	 * Returns true if anything was added. Decreases Quantity and destroys
	 * the actor if it hits zero.
	 */
	bool TryPickupBy(AActor* InteractingActor, FText& OutFailureReason);

	UFUNCTION()
	void HandlePickupSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);
};
