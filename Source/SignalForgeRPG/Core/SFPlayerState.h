#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "SFPlayerState.generated.h"

class USFNarrativeComponent;
class USFCompanionComponent;
class UAbilitySystemComponent;

/**
 * ASFPlayerState
 *
 * Persistent per-player container for state that must survive pawn death,
 * possession changes, and seamless travel. Owns the player's narrative
 * component (quests, dialogue, world facts, factions, identity, endings)
 * and is the canonical replication target for narrative state on the
 * owning client.
 *
 * Note: AbilitySystemComponent ownership is also commonly hosted here
 * via the IAbilitySystemInterface. The interface is implemented as a
 * forward hook so the project can wire its existing GAS owner without
 * forcing a duplicate component on this class. If your GAS lives on the
 * pawn, leave GetAbilitySystemComponent() returning nullptr here.
 */
UCLASS()
class SIGNALFORGERPG_API ASFPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ASFPlayerState();

	//~ Begin AActor interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	//~ End AActor interface

	//~ Begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End IAbilitySystemInterface

	/** Accessor for the owning narrative component. Always valid after construction. */
	UFUNCTION(BlueprintPure, Category = "Narrative")
	USFNarrativeComponent* GetNarrativeComponent() const { return NarrativeComponent; }

	/** Accessor for the player's companion component (roster, active companion, orders, approval). */
	UFUNCTION(BlueprintPure, Category = "Companion")
	USFCompanionComponent* GetCompanionComponent() const { return CompanionComponent; }

protected:
	/**
	 * Player's narrative component. Hosts quests, dialogue, world facts,
	 * faction standings, identity axes, ending scores, and the save/load
	 * surface. Replicated to the owning client.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narrative")
	TObjectPtr<USFNarrativeComponent> NarrativeComponent;

	/**
	 * Player's companion component. Owns the recruited roster, the active
	 * companion, the per-companion approval map, and the radial-wheel
	 * command surface (stance, orders, slow-mo).
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Companion")
	TObjectPtr<USFCompanionComponent> CompanionComponent;
};
