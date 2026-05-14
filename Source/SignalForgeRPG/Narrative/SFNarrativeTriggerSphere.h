// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Narrative/SFNarrativeTypes.h"
#include "SFNarrativeTriggerSphere.generated.h"

class AActor;
class ASFPlayerCharacter;
class USphereComponent;
class USFConversationDataAsset;
class USFNarrativeComponent;
class USFQuestDefinition;

/**
 * Lightweight world-fact payload authored on the trigger sphere. Each entry
 * is applied to the overlapping player's narrative component when the trigger
 * fires successfully. We mirror the FSFWorldFactKey/Value pair as a struct
 * (rather than exposing the variant directly) because authoring two structs
 * side-by-side in details panels is fiddly and the trigger is the only place
 * we currently need this exact pattern.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeTriggerFactWrite
{
    GENERATED_BODY()

    /** Which fact to record on the player (tag + optional context). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Trigger")
    FSFWorldFactKey Key;

    /** Value to write. Use Bool/Int/Float/Name/Tag variants per ValueType. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Trigger")
    FSFWorldFactValue Value;
};

/**
 * ASFNarrativeTriggerSphere
 *
 * Drop-in actor that fires narrative actions (start quest, start conversation,
 * record world facts) when the player walks into its sphere. Designed for the
 * "minimum viable scripting" case: designer drags the actor into a level, picks
 * a quest asset and/or a conversation asset, and ships it.
 *
 * Behavioral defaults (matching the chosen configuration):
 *   - Player pawn only. NPC overlaps are ignored.
 *   - Fire once per save, persistently. On successful fire we write a world
 *     fact "Narrative.Trigger.Fired.<TriggerId>" on the player's narrative
 *     component, and refuse to fire again whenever that fact is present.
 *     The fact survives save/load via the existing narrative save service.
 *   - Collision is disabled on the sphere as soon as the trigger fires.
 *
 * Authoring tips:
 *   - Set TriggerId to a stable unique FName per actor instance (e.g.
 *     "Tutorial.Intro.GreetCompanion"). This is what gets persisted, so
 *     renaming the actor in the World Outliner does NOT invalidate the save.
 *   - Leave QuestToStart / ConversationToStart unset for any action you
 *     don't want. The trigger only does what you authored.
 *
 * Not in scope (intentional, ask later if needed):
 *   - FSFNarrativeConditionSet gating. The trigger is a dumb door; the
 *     narrative system itself should gate on the receiving side (e.g.
 *     a quest with a precondition that fails silently if not eligible).
 *   - Cooldown re-fire. Use bFireOnce false + bPersistFiredState false
 *     in a future iteration if needed.
 *   - Non-player actors. Add a future bPlayerOnly toggle if NPC overlap
 *     becomes useful (companions reacting to volume entry, etc.).
 */
UCLASS(Blueprintable, ClassGroup = (Narrative), meta = (DisplayName = "Narrative Trigger Sphere"))
class SIGNALFORGERPG_API ASFNarrativeTriggerSphere : public AActor
{
    GENERATED_BODY()

public:
    ASFNarrativeTriggerSphere();

    /**
     * Stable identifier used to persist the "this trigger has fired" world
     * fact. Must be unique per trigger instance you care about persisting.
     * Recommended convention: dotted scope, e.g. Tutorial.Intro.GreetCompanion.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Trigger")
    FName TriggerId = NAME_None;

    /** Radius in cm of the overlap sphere. Default ~3m which reads as "into the doorway". */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Trigger", meta = (ClampMin = "1.0"))
    float SphereRadius = 300.f;

    /**
     * Quest data asset to start on the player when the trigger fires.
     * Leave unset to skip the quest-start action.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Trigger|Actions")
    TSoftObjectPtr<USFQuestDefinition> QuestToStart;

    /** Optional state id to start the quest at. None = the quest's default start state. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Trigger|Actions")
    FName QuestStartStateId = NAME_None;

    /**
     * Conversation data asset to begin on the player when the trigger fires.
     * Leave unset to skip the dialogue action. The player's narrative
     * component must have a dialogue component bound.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Trigger|Actions")
    TSoftObjectPtr<USFConversationDataAsset> ConversationToStart;

    /**
     * World facts to record on the player's narrative component when this
     * trigger fires. Applied BEFORE quest / conversation start so quest
     * eligibility checks can see them.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Trigger|Actions")
    TArray<FSFNarrativeTriggerFactWrite> FactsToWriteOnFire;

    /** Tag stamped on the "fired" world fact for analytics / debug. Optional. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Trigger|Advanced")
    FGameplayTagContainer DebugTags;

    /** Returns true if this trigger has already fired (per the player's narrative state). */
    UFUNCTION(BlueprintPure, Category = "Narrative|Trigger")
    bool HasAlreadyFired() const { return bHasFiredThisSession; }

    /**
     * Force the trigger to fire against a specific narrative-owning actor.
     * Useful for scripted invocations from Blueprint that bypass the overlap.
     * Returns true if any action was actually applied.
     */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Trigger")
    bool ForceFire(AActor* TargetActor);

    /**
     * Re-arm a previously-fired trigger. Clears the local already-fired flag
     * AND the persisted world fact (if any) on the supplied narrative owner.
     * Intended for debug / quest-restart tooling, not gameplay use.
     */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Trigger")
    void ResetFiredState(AActor* TargetActor);

protected:
    virtual void BeginPlay() override;

    /** Visual + overlap volume. Root component. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USphereComponent> TriggerSphere = nullptr;

    UFUNCTION()
    void HandleSphereBeginOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    /**
     * BP hooks for designers who want to attach effects (VFX, audio, level
     * scripting) without having to subclass in C++.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Narrative|Trigger")
    void BP_OnPlayerEntered(AActor* PlayerActor);

    UFUNCTION(BlueprintImplementableEvent, Category = "Narrative|Trigger")
    void BP_OnTriggerFired(AActor* PlayerActor);

private:
    /** Mirror of the persisted "fired" state for fast in-frame checks. */
    UPROPERTY(Transient)
    bool bHasFiredThisSession = false;

    /**
     * Build the world fact key used to persist "this trigger has fired".
     * Returns false if TriggerId is None (in which case persistence is skipped).
     */
    bool BuildFiredFactKey(FSFWorldFactKey& OutKey) const;

    /** Check the player's narrative state for the persisted fired fact. */
    bool QueryPersistedFiredState(USFNarrativeComponent* Narrative) const;

    /** Stamp the persisted fired fact on the player's narrative component. */
    void StampPersistedFiredState(USFNarrativeComponent* Narrative) const;

    /** Apply the action payload (facts, quest, dialogue) to the given narrative owner. */
    bool ApplyActionsTo(AActor* TargetActor, USFNarrativeComponent* Narrative);

    /** Disable collision so subsequent overlaps don't fire callbacks. */
    void DisableTriggerCollision();
};
