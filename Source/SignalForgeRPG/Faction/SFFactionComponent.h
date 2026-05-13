#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "GenericTeamAgentInterface.h"
#include "Faction/SFFactionTypes.h"
#include "SFFactionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FSFOnFactionTagChanged,
	FGameplayTag, OldFaction,
	FGameplayTag, NewFaction);

/**
 * USFFactionComponent
 *
 * Universal faction carrier for every ASFCharacterBase. Replaces the old
 * "hostility is implied by class identity" pattern. The component:
 *
 *   - Stores a replicated FactionTag (under the "Faction." root).
 *   - Mirrors FactionTag from a USFNPCNarrativeIdentityComponent if one is
 *     present on the owner (avoids double-authoring on NPCs).
 *   - Exposes Get/SetFactionTag with an OnRep + delegate fanout.
 *
 * Hostility resolution lives in USFFactionStatics, not here. The component
 * is intentionally small.
 *
 * The actual IGenericTeamAgentInterface implementation lives on the owning
 * ASFCharacterBase (Unreal requires the interface on the AActor, not on a
 * component), and that implementation delegates back into this component
 * via GetTeamId() / GetAttitudeTowardOther().
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SIGNALFORGERPG_API USFFactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFFactionComponent();

	// -------------------------------------------------------------------------
	// Lifecycle
	// -------------------------------------------------------------------------
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	// -------------------------------------------------------------------------
	// Faction tag
	// -------------------------------------------------------------------------

	/** Authoritative read of the current faction tag. */
	UFUNCTION(BlueprintPure, Category = "Faction")
	FGameplayTag GetFactionTag() const { return FactionTag; }

	/** Server-side setter. Use during spawn or on faction-change quests. */
	UFUNCTION(BlueprintCallable, Category = "Faction")
	void SetFactionTag(FGameplayTag NewFaction);

	/** Broadcasts whenever the faction tag changes (server immediate; client via OnRep). */
	UPROPERTY(BlueprintAssignable, Category = "Faction")
	FSFOnFactionTagChanged OnFactionTagChanged;

	// -------------------------------------------------------------------------
	// Team id mapping for IGenericTeamAgentInterface
	// -------------------------------------------------------------------------

	/** Deterministic mapping from the faction tag name to a stable FGenericTeamId. */
	FGenericTeamId GetTeamId() const;

	/** Cooked attitude toward another actor's faction (calls USFFactionStatics). */
	ETeamAttitude::Type GetAttitudeTowardOther(const AActor& Other) const;

protected:
	/** Replicated faction tag. */
	UPROPERTY(ReplicatedUsing = OnRep_FactionTag, EditAnywhere, BlueprintReadOnly,
		Category = "Faction", meta = (Categories = "Faction"))
	FGameplayTag FactionTag;

	UFUNCTION()
	void OnRep_FactionTag(FGameplayTag OldFaction);

	/**
	 * If true and the owner has a USFNPCNarrativeIdentityComponent, that
	 * component's FactionTag will be mirrored into this component on
	 * BeginPlay so designers only author the tag in one place.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Faction|Mirroring")
	bool bMirrorFromNPCIdentity = true;
};
