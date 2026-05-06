#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GameplayTagContainer.h"
#include "SFGameStateBase.generated.h"

class USFNarrativeReplicatorComponent;

/**
 * ASFGameStateBase
 *
 * Server-authoritative, replicated world container. One instance exists
 * on the server and is mirrored to every connected client. Hosts shared
 * state that is *not* per-player:
 *
 *   - World narrative replication (faction standings, world facts, etc.)
 *     via USFNarrativeReplicatorComponent.
 *   - Coarse session/world flags that any client may read
 *     (current world phase tag, server-authoritative world time).
 *
 * Per-player narrative state (quests, dialogue progress, identity,
 * endings) lives on ASFPlayerState. Anything every player must see
 * identically lives here.
 */
UCLASS()
class SIGNALFORGERPG_API ASFGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	ASFGameStateBase();

	//~ Begin AActor interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	//~ End AActor interface

	/** Replicated narrative replicator. Owns FastArray world facts and faction standings shared by all clients. */
	UFUNCTION(BlueprintPure, Category = "Narrative")
	USFNarrativeReplicatorComponent* GetNarrativeReplicator() const { return NarrativeReplicator; }

	/**
	 * Current high-level world phase, expressed as a gameplay tag
	 * (e.g. World.Phase.Prologue, World.Phase.Insurrection, World.Phase.Endgame).
	 * Designers gate spawners, AI behaviors, and ambient music off this tag.
	 * Server only — clients receive via replication.
	 */
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "World|Phase")
	void SetWorldPhase(FGameplayTag NewPhase);

	UFUNCTION(BlueprintPure, Category = "World|Phase")
	FGameplayTag GetWorldPhase() const { return WorldPhase; }

	/** Broadcast on all machines whenever the world phase changes (after replication on clients). */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSFWorldPhaseChanged, FGameplayTag, OldPhase, FGameplayTag, NewPhase);
	UPROPERTY(BlueprintAssignable, Category = "World|Phase")
	FSFWorldPhaseChanged OnWorldPhaseChanged;

protected:
	/** Replicator component — owns the shared FastArray narrative state. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narrative")
	TObjectPtr<USFNarrativeReplicatorComponent> NarrativeReplicator;

	/** Replicated world phase tag. */
	UPROPERTY(ReplicatedUsing = OnRep_WorldPhase, VisibleAnywhere, BlueprintReadOnly, Category = "World|Phase")
	FGameplayTag WorldPhase;

	UFUNCTION()
	void OnRep_WorldPhase(FGameplayTag OldPhase);

private:
	/** Cached previous phase used to fire OnWorldPhaseChanged on the server. */
	FGameplayTag PreviousWorldPhase;
};
