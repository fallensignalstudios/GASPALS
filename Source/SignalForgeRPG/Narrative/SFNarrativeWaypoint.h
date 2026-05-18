// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "SFNarrativeWaypoint.generated.h"

class UBillboardComponent;
class USceneComponent;
class USFQuestDefinition;

/**
 * Drop-in level actor that anchors a quest waypoint to a world location.
 *
 * The actor registers with USFNarrativeWaypointSubsystem on BeginPlay using
 * its (QuestToTrack -> QuestId) and TargetTaskId as the lookup key. The
 * subsystem will surface its world location as the waypoint for that task
 * whenever the quest is active and the task is incomplete.
 *
 * Use this when the marker needs to follow a moving NPC, sit at a doorway,
 * or be repositioned in-editor without editing the quest data asset.
 *
 * Authoring tip: leave the actor visible-in-editor only (the default
 * billboard is editor-only) so the marker doesn't render in-game.
 */
UCLASS(Blueprintable)
class SIGNALFORGERPG_API ASFNarrativeWaypoint : public AActor
{
    GENERATED_BODY()

public:
    ASFNarrativeWaypoint();

    // -- AActor --
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    // -- Authoring --

    /** Quest this waypoint belongs to. Resolved through the asset's QuestId. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|Waypoint")
    TSoftObjectPtr<USFQuestDefinition> QuestToTrack;

    /** TaskId within the quest's state graph this waypoint anchors to. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|Waypoint")
    FName TargetTaskId = NAME_None;

    /** Optional override for the HUD label. Falls back to the task's authored text. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|Waypoint")
    FText DisplayTextOverride;

    /** Optional override for the icon-selection tag. Empty means use the task's authored tag. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|Waypoint")
    FGameplayTag IconTagOverride;

    /** When true, the actor's collision/visibility shut off the moment the task completes. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Quest|Waypoint")
    bool bAutoHideWhenTaskComplete = true;

    // -- Runtime helpers --

    /**
     * Build the registration key the subsystem looks up. Returns an empty
     * string when the required fields haven't been set, which the subsystem
     * uses as a signal to log a designer warning instead of silently
     * dropping the actor.
     */
    FString BuildRegistrationKey() const;

    /** Re-register with the subsystem (e.g. after the task id was changed at runtime). */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|Waypoint")
    void RefreshSubsystemRegistration();

    /** Designer hooks for VFX / SFX. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Narrative|Quest|Waypoint")
    void BP_OnBecameActive();

    UFUNCTION(BlueprintImplementableEvent, Category = "Narrative|Quest|Waypoint")
    void BP_OnBecameInactive();

    UFUNCTION(BlueprintImplementableEvent, Category = "Narrative|Quest|Waypoint")
    void BP_OnTaskCompleted();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> Root = nullptr;

#if WITH_EDITORONLY_DATA
    UPROPERTY()
    TObjectPtr<UBillboardComponent> EditorSprite = nullptr;
#endif

    /** True while we are currently registered with the subsystem. */
    bool bRegisteredWithSubsystem = false;
};
