#pragma once

#include "CoreMinimal.h"
#include "Characters/SFCharacterBase.h"
#include "Components/SFInteractableInterface.h"
#include "Dialogue/Data/SFConversationSourceInterface.h"
#include "GameplayTagContainer.h"
#include "SFNPCBase.generated.h"

class USFConversationDataAsset;
class USFNPCNarrativeIdentityComponent;
class USFNarrativeComponent;

/**
 * ASFNPCBase
 *
 * Canonical non-combat / dialogue-capable character. Inherits the full
 * SFCharacterBase stack (GAS, attributes, equipment, inventory, hit
 * resolution) so any NPC can be wounded, healed, killed, or scripted
 * exactly the same way the player and enemies are.
 *
 * Layered on top:
 *   - USFNPCNarrativeIdentityComponent: faction, narrative tags,
 *     disposition, essential flag, stable per-NPC ContextId.
 *   - Quest/fact-aware interaction gating: optional RequiredFactTags /
 *     BlockedFactTags container that is checked against the asking
 *     player's narrative component before the "Talk" prompt appears.
 *   - Auto-fires `Fact.NPC.<ContextId>.Met` on the first conversation,
 *     so dialogue authors can branch on it without scripting.
 *   - Death broadcasts an FSFNarrativeDelta (ApplyOutcome) so kills
 *     can drive faction shifts and quest progress automatically.
 *
 * Default AI controller is ASFNPCAIController.
 */
UCLASS()
class SIGNALFORGERPG_API ASFNPCBase
	: public ASFCharacterBase
	, public ISFInteractableInterface
	, public ISFConversationSourceInterface
{
	GENERATED_BODY()

public:
	ASFNPCBase();

	//~ Begin AActor / ACharacter
	virtual void BeginPlay() override;
	//~ End AActor / ACharacter

	//~ ASFCharacterBase
	virtual void HandleDeath() override;
	//~ End ASFCharacterBase

	UFUNCTION(BlueprintPure, Category = "NPC")
	USFNPCNarrativeIdentityComponent* GetNarrativeIdentity() const { return NarrativeIdentity; }

	UFUNCTION(BlueprintPure, Category = "NPC")
	FText GetNPCName() const { return NPCName; }

protected:
	// -------------------------------------------------------------------------
	// Identity / display
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC")
	FText NPCName;

	/** Narrative identity (faction, context id, disposition, narrative tags). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC")
	TObjectPtr<USFNPCNarrativeIdentityComponent> NarrativeIdentity;

	// -------------------------------------------------------------------------
	// Interaction config
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FText InteractionPromptText = FText::FromString(TEXT("Talk"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FText InteractionSubPromptText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool bCanInteract = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	float InteractionRange = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool bShowPromptWhenUnavailable = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	ESFInteractionTriggerType InteractionTriggerType = ESFInteractionTriggerType::Press;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0"))
	float InteractionHoldDuration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	FName PrimaryInteractionOptionId = TEXT("Talk");

	// -------------------------------------------------------------------------
	// Narrative gating
	// -------------------------------------------------------------------------

	/** All of these tags must be present in the asking player's faction/world tag context for the interaction to be available. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Gating", meta = (Categories = "Fact"))
	FGameplayTagContainer RequiredFactTags;

	/** Any of these tags being present in the asking player's faction/world tag context blocks the interaction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Gating", meta = (Categories = "Fact"))
	FGameplayTagContainer BlockedFactTags;

	/** If set, NPC refuses to talk while disposition matches one of these. Hostile NPCs default to refusing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Gating")
	bool bRefuseDialogueWhenHostile = true;

	// -------------------------------------------------------------------------
	// Dialogue
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName DialogueSpeakerId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<USFConversationDataAsset> ConversationAsset = nullptr;

	/**
	 * When the player triggers their first conversation with this NPC,
	 * a bool world fact `Fact.NPC.<ContextId>.Met = true` is set on the
	 * player's narrative component. Designers can then branch on it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	bool bAutoSetMetFactOnFirstConversation = true;

	// Death automatically writes `Fact.NPC.Killed` (bool) keyed by this NPC's
	// ContextId on every connected player's narrative component, so quests
	// and faction reactions can branch on it. For richer reactions
	// (asset-based outcomes, faction deltas, etc.) override HandleDeath in
	// a subclass or react to OnDispositionChanged via the identity component.

public:
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

	// -------------------------------------------------------------------------
	// ISFConversationSourceInterface
	// -------------------------------------------------------------------------

	virtual USFConversationDataAsset* GetConversationAsset_Implementation() const override;
	virtual FName GetDialogueSpeakerId_Implementation() const override;

protected:
	/** Locates the asking player's narrative component. May return nullptr (interactor isn't a player). */
	USFNarrativeComponent* ResolveInstigatorNarrative(const FSFInteractionContext& InteractionContext) const;

	/** Returns true if the player's narrative tag context satisfies RequiredFactTags / BlockedFactTags. */
	bool PassesFactGate(USFNarrativeComponent* InstigatorNarrative) const;
};
