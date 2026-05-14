#include "Inventory/SFWorldItemActor.h"

#include "Components/SFInventoryComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Inventory/SFItemDefinition.h"

#define LOCTEXT_NAMESPACE "SFWorldItem"

ASFWorldItemActor::ASFWorldItemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Pickup volume is the root so we move/scale as a single unit. Sphere
	// is QueryOnly so it doesn't block character movement -- you walk
	// straight through a dropped med pack and either auto-pick-up or get
	// an interact prompt.
	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->InitSphereRadius(PickupRadius);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PickupSphere->SetGenerateOverlapEvents(true);
	SetRootComponent(PickupSphere);

	// Visible mesh -- collision left off so projectiles / sweeps don't get
	// blocked by stray world drops. If a designer wants a physical pickup
	// (a barrel that you also kick) they can subclass and re-enable.
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	MeshComponent->SetupAttachment(PickupSphere);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetGenerateOverlapEvents(false);
}

ASFWorldItemActor* ASFWorldItemActor::SpawnWorldItem(
	const UObject* WorldContextObject,
	TSubclassOf<ASFWorldItemActor> ActorClass,
	USFItemDefinition* InItemDefinition,
	int32 InQuantity,
	const FTransform& SpawnTransform)
{
	if (!WorldContextObject || !InItemDefinition || InQuantity <= 0)
	{
		return nullptr;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// Fall back to the C++ base class if no BP override was supplied. This
	// lets the loot system "just work" with default art before designers
	// author a BP_WorldItemActor with custom outline / particles / sound.
	TSubclassOf<ASFWorldItemActor> ResolvedClass = ActorClass ? ActorClass : TSubclassOf<ASFWorldItemActor>(ASFWorldItemActor::StaticClass());

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ASFWorldItemActor* Spawned = World->SpawnActor<ASFWorldItemActor>(ResolvedClass, SpawnTransform, Params);
	if (Spawned)
	{
		// Set payload BEFORE BeginPlay fires so the mesh refresh inside
		// BeginPlay picks up the correct definition. SpawnActor on a
		// pre-BeginPlay actor defers BeginPlay until after construction
		// returns, so this assignment lands in time.
		Spawned->InitializePayload(InItemDefinition, InQuantity);
	}
	return Spawned;
}

void ASFWorldItemActor::InitializePayload(USFItemDefinition* InItemDefinition, int32 InQuantity)
{
	ItemDefinition = InItemDefinition;
	Quantity = FMath::Max(1, InQuantity);

	// If we're already in the world, refresh visuals immediately. If not,
	// BeginPlay will call RefreshMeshFromDefinition for us.
	if (HasActorBegunPlay())
	{
		RefreshMeshFromDefinition();
	}
}

void ASFWorldItemActor::BeginPlay()
{
	Super::BeginPlay();

	// Apply the configured radius now (CDO ctor radius may have been
	// stomped by a designer editing the property on a placed instance).
	if (PickupSphere)
	{
		PickupSphere->SetSphereRadius(PickupRadius);
		PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &ASFWorldItemActor::HandlePickupSphereBeginOverlap);
	}

	RefreshMeshFromDefinition();
}

void ASFWorldItemActor::RefreshMeshFromDefinition()
{
	if (!MeshComponent)
	{
		return;
	}

	if (!ItemDefinition)
	{
		MeshComponent->SetStaticMesh(nullptr);
		return;
	}

	// WorldStaticMesh is a TSoftObjectPtr -- LoadSynchronous is acceptable
	// here because pickups are short-lived and the mesh is small. If
	// you're spawning hundreds at once, swap to an async streaming handle
	// per asset and bind a callback to set the mesh when ready.
	UStaticMesh* Mesh = ItemDefinition->WorldStaticMesh.LoadSynchronous();
	MeshComponent->SetStaticMesh(Mesh);
}

bool ASFWorldItemActor::TryPickupBy(AActor* InteractingActor, FText& OutFailureReason)
{
	if (!InteractingActor || !ItemDefinition || Quantity <= 0)
	{
		OutFailureReason = LOCTEXT("InvalidPickup", "This item can't be picked up.");
		return false;
	}

	// We require the interactor to expose a USFInventoryComponent. Anything
	// else (an NPC walking through, an AI looter) won't pick the item up.
	// Use FindComponentByClass so subclassed inventories are also accepted.
	USFInventoryComponent* Inventory = InteractingActor->FindComponentByClass<USFInventoryComponent>();
	if (!Inventory)
	{
		OutFailureReason = LOCTEXT("NoInventory", "No inventory to receive this item.");
		return false;
	}

	const FSFInventoryAddResult AddResult = Inventory->AddItem(ItemDefinition, Quantity);
	if (AddResult.AddedQuantity <= 0)
	{
		// Inventory full / disallowed -- surface its error text to the UI.
		OutFailureReason = AddResult.ErrorText.IsEmpty()
			? LOCTEXT("CannotAccept", "Cannot pick up right now.")
			: AddResult.ErrorText;
		return false;
	}

	// Subtract what was accepted. AddItem can return partial success if
	// inventory has capacity for some but not all of the stack -- in that
	// case leave the actor in the world with the remainder so the player
	// can come back after dropping something.
	Quantity = FMath::Max(0, Quantity - AddResult.AddedQuantity);

	if (Quantity <= 0)
	{
		Destroy();
	}
	return true;
}

void ASFWorldItemActor::HandlePickupSphereBeginOverlap(
	UPrimitiveComponent* /*OverlappedComp*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	if (!bAutoPickupOnOverlap || !OtherActor)
	{
		return;
	}

	// Only player-style actors auto-vacuum -- having an inventory and a
	// PlayerController is a decent proxy for "this is the player". If a
	// design later wants AI looting, drop the controller check.
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}

	FText Unused;
	TryPickupBy(OtherActor, Unused);
}

// -----------------------------------------------------------------------------
// ISFInteractableInterface
// -----------------------------------------------------------------------------

ESFInteractionAvailability ASFWorldItemActor::GetInteractionAvailability_Implementation(
	const FSFInteractionContext& InteractionContext) const
{
	if (!ItemDefinition || Quantity <= 0)
	{
		return ESFInteractionAvailability::Invalid;
	}
	if (!InteractionContext.InteractingActor)
	{
		return ESFInteractionAvailability::Invalid;
	}
	if (InteractionContext.DistanceToTarget > InteractionRange)
	{
		return ESFInteractionAvailability::OutOfRange;
	}
	return ESFInteractionAvailability::Available;
}

FSFInteractionOption ASFWorldItemActor::GetPrimaryInteractionOption_Implementation(
	const FSFInteractionContext& InteractionContext) const
{
	FSFInteractionOption Option;
	Option.OptionId = TEXT("Pickup");
	Option.TriggerType = ESFInteractionTriggerType::Press;
	Option.Availability = GetInteractionAvailability_Implementation(InteractionContext);

	// Localized prompt. Includes quantity when > 1 so the player can see
	// they're about to grab a whole stack rather than a single item.
	if (ItemDefinition)
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("ItemName"), ItemDefinition->DisplayName);
		Args.Add(TEXT("Quantity"), FText::AsNumber(Quantity));
		Option.PromptText = (Quantity > 1)
			? FText::Format(LOCTEXT("PickupPromptQuantity", "Pick up {ItemName} x{Quantity}"), Args)
			: FText::Format(LOCTEXT("PickupPromptSingle", "Pick up {ItemName}"), Args);
	}
	else
	{
		Option.PromptText = LOCTEXT("PickupPromptGeneric", "Pick up");
	}
	return Option;
}

TArray<FSFInteractionOption> ASFWorldItemActor::GetInteractionOptions_Implementation(
	const FSFInteractionContext& InteractionContext) const
{
	TArray<FSFInteractionOption> Options;
	Options.Add(GetPrimaryInteractionOption_Implementation(InteractionContext));
	return Options;
}

FSFInteractionExecutionResult ASFWorldItemActor::Interact_Implementation(
	const FSFInteractionContext& InteractionContext)
{
	const ESFInteractionAvailability Availability =
		GetInteractionAvailability_Implementation(InteractionContext);
	if (Availability != ESFInteractionAvailability::Available)
	{
		return FSFInteractionExecutionResult::Failure(
			Availability,
			LOCTEXT("PickupUnavailable", "Cannot pick this up right now."),
			TEXT("Pickup"));
	}

	FText FailureReason;
	if (!TryPickupBy(InteractionContext.InteractingActor, FailureReason))
	{
		return FSFInteractionExecutionResult::Failure(
			ESFInteractionAvailability::Blocked,
			FailureReason,
			TEXT("Pickup"));
	}

	return FSFInteractionExecutionResult::Success(TEXT("Pickup"));
}

FSFInteractionExecutionResult ASFWorldItemActor::InteractWithOption_Implementation(
	const FSFInteractionContext& InteractionContext,
	FName OptionId)
{
	// Only one option (Pickup) -- anything else is a programming error.
	if (OptionId != NAME_None && OptionId != TEXT("Pickup"))
	{
		return FSFInteractionExecutionResult::Failure(
			ESFInteractionAvailability::Invalid,
			LOCTEXT("UnknownOption", "Unknown interaction option."),
			OptionId);
	}
	return Interact_Implementation(InteractionContext);
}

void ASFWorldItemActor::BeginInteractionFocus_Implementation(
	const FSFInteractionContext& /*InteractionContext*/)
{
	// Hook for outline / glow / hover sound. Left empty by default so
	// designers can override in BP without touching C++.
}

void ASFWorldItemActor::EndInteractionFocus_Implementation(
	const FSFInteractionContext& /*InteractionContext*/)
{
}

FVector ASFWorldItemActor::GetInteractionLocation_Implementation(
	const FSFInteractionContext& /*InteractionContext*/) const
{
	return GetActorLocation();
}

float ASFWorldItemActor::GetInteractionRange_Implementation(
	const FSFInteractionContext& /*InteractionContext*/) const
{
	return InteractionRange;
}

bool ASFWorldItemActor::CanInteract_Implementation(
	const FSFInteractionContext& InteractionContext) const
{
	return GetInteractionAvailability_Implementation(InteractionContext) ==
		ESFInteractionAvailability::Available;
}

FText ASFWorldItemActor::GetInteractionPromptText_Implementation(
	const FSFInteractionContext& InteractionContext) const
{
	return GetPrimaryInteractionOption_Implementation(InteractionContext).PromptText;
}

#undef LOCTEXT_NAMESPACE
