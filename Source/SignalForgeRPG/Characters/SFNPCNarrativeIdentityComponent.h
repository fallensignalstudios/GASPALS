#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "SFNPCNarrativeIdentityComponent.generated.h"

/**
 * Coarse social/combat disposition for an NPC. Drives interaction prompts,
 * AI reactions, and faction-band defaults. Replicated.
 */
UENUM(BlueprintType)
enum class ESFNPCDisposition : uint8
{
	Friendly UMETA(DisplayName = "Friendly"),
	Wary     UMETA(DisplayName = "Wary"),
	Neutral  UMETA(DisplayName = "Neutral"),
	Hostile  UMETA(DisplayName = "Hostile"),
	Dead     UMETA(DisplayName = "Dead"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FSFNPCDispositionChanged,
	ESFNPCDisposition, OldDisposition,
	ESFNPCDisposition, NewDisposition);

/**
 * USFNPCNarrativeIdentityComponent
 *
 * Lightweight, replicated component bolted onto any narrative-relevant
 * actor (NPCs, key world objects). Provides:
 *
 *   - A stable ContextId used as FSFWorldFactKey::ContextId so per-NPC
 *     world facts can be authored/queried without colliding.
 *   - A FactionTag used to look up shared faction standings on the
 *     GameState's narrative replicator.
 *   - A NarrativeTags container for broad classification ("NPC.Role.QuestGiver",
 *     "NPC.Status.Wounded") that designers can match against in dialogue
 *     conditions and quest predicates.
 *   - An ESFNPCDisposition enum that the AI controller, animation system,
 *     and interaction gating all read from a single source of truth.
 *
 * Server-authoritative. Clients receive disposition changes via OnRep
 * and rebroadcast through OnDispositionChanged.
 */
UCLASS(ClassGroup = (Narrative), meta = (BlueprintSpawnableComponent))
class SIGNALFORGERPG_API USFNPCNarrativeIdentityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFNPCNarrativeIdentityComponent();

	//~ Begin UActorComponent
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void InitializeComponent() override;
	//~ End UActorComponent

	/** Stable per-NPC identifier used as ContextId for world facts. Auto-derived from owner name if blank. */
	UFUNCTION(BlueprintPure, Category = "Narrative|Identity")
	FName GetNPCContextId() const { return NPCContextId; }

	UFUNCTION(BlueprintCallable, Category = "Narrative|Identity")
	void SetNPCContextId(FName NewId) { NPCContextId = NewId; }

	UFUNCTION(BlueprintPure, Category = "Narrative|Identity")
	FGameplayTag GetFactionTag() const { return FactionTag; }

	UFUNCTION(BlueprintCallable, Category = "Narrative|Identity")
	void SetFactionTag(FGameplayTag NewFaction) { FactionTag = NewFaction; }

	UFUNCTION(BlueprintPure, Category = "Narrative|Identity")
	const FGameplayTagContainer& GetNarrativeTags() const { return NarrativeTags; }

	UFUNCTION(BlueprintPure, Category = "Narrative|Identity")
	bool HasNarrativeTag(FGameplayTag Tag) const { return NarrativeTags.HasTag(Tag); }

	UFUNCTION(BlueprintPure, Category = "Narrative|Identity")
	bool IsEssential() const { return bIsEssential; }

	UFUNCTION(BlueprintPure, Category = "Narrative|Identity")
	bool IsHostileByDefault() const { return bIsHostileByDefault; }

	UFUNCTION(BlueprintPure, Category = "Narrative|Identity")
	ESFNPCDisposition GetDisposition() const { return Disposition; }

	/** Authority-only setter that fans out OnDispositionChanged via OnRep on clients. */
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Narrative|Identity")
	void SetDisposition(ESFNPCDisposition NewDisposition);

	/** Broadcast on server immediately after SetDisposition; on clients after OnRep_Disposition. */
	UPROPERTY(BlueprintAssignable, Category = "Narrative|Identity")
	FSFNPCDispositionChanged OnDispositionChanged;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Identity")
	FName NPCContextId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Identity", meta = (Categories = "Faction"))
	FGameplayTag FactionTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Identity", meta = (Categories = "NPC"))
	FGameplayTagContainer NarrativeTags;

	/** Essential NPCs cannot be killed; lethal damage caps them at 1 HP and the systems treat them as alive. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Identity")
	bool bIsEssential = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Identity")
	bool bIsHostileByDefault = false;

	UPROPERTY(ReplicatedUsing = OnRep_Disposition, EditAnywhere, BlueprintReadOnly, Category = "Narrative|Identity")
	ESFNPCDisposition Disposition = ESFNPCDisposition::Neutral;

	UFUNCTION()
	void OnRep_Disposition(ESFNPCDisposition OldDisposition);

	/** Cached previous value used for server-side delegate parity. */
	ESFNPCDisposition PreviousDisposition = ESFNPCDisposition::Neutral;
};
