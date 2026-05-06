// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Dialogue/Data/SFDialogueComponent.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeStructs.h"
#include "SFNarrativeDelegates.h"
#include "SFDialogueComponentBridge.generated.h"

class USFNarrativeComponent;

/**
 * Bridge from the standalone dialogue system (USFDialogueComponent) into the
 * Sovereign Call narrative system.
 *
 * Responsibilities:
 *  - Listen to dialogue lifecycle events on a paired USFDialogueComponent.
 *  - Emit FSFNarrativeDelta instances toward the owning USFNarrativeComponent.
 *  - Build FSFDialogueMoment views from SFDialogueDisplayData for narrative/UI.
 *
 * This component does NOT own dialogue flow logic; it only observes and forwards.
 */
UCLASS(ClassGroup = (SignalForge), meta = (BlueprintSpawnableComponent))
class SIGNALFORGERPG_API USFDialogueComponentBridge : public UActorComponent
{
    GENERATED_BODY()

public:
    USFDialogueComponentBridge();

    /** Get the dialogue component we are currently bound to (if any). */
    UFUNCTION(BlueprintPure, Category = "Narrative|Dialogue")
    USFDialogueComponent* GetDialogueComponent() const { return DialogueComponent; }

    /** Get the narrative component we will send deltas to (if any). */
    UFUNCTION(BlueprintPure, Category = "Narrative|Dialogue")
    USFNarrativeComponent* GetNarrativeComponent() const { return NarrativeComponent; }

    /** Manually re?bind to a specific dialogue component (e.g., if you spawn it at runtime). */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Dialogue")
    void SetDialogueComponent(USFDialogueComponent* InDialogueComponent);

    /** Manually re?bind to a specific narrative component. */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Dialogue")
    void SetNarrativeComponent(USFNarrativeComponent* InNarrativeComponent);

    /** Current dialogue ID used for deltas/logging (derived from conversation asset name by default). */
    UFUNCTION(BlueprintPure, Category = "Narrative|Dialogue")
    FName GetCurrentDialogueId() const { return CurrentDialogueId; }

    /** Last resolved dialogue moment snapshot, for quick polling UIs. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Dialogue")
    const FSFDialogueMoment& GetLastDialogueMoment() const { return LastDialogueMoment; }

    //
    // Narrative?facing delegates – raised by the bridge after it adapts the raw dialogue events.
    //

    /** High?level moment stream suitable for narrative?driven UIs. */
    UPROPERTY(BlueprintAssignable, Category = "Narrative|Dialogue")
    FSFFOnDialogueMomentUpdated OnNarrativeDialogueMomentUpdated;

    /** Emitted whenever we produce a FSFNarrativeDelta from a dialogue event. */
    UPROPERTY(BlueprintAssignable, Category = "Narrative|Dialogue")
    FSFFOnNarrativeDeltaEmitted OnNarrativeDialogueDeltaEmitted;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Attempt to auto?discover sibling components on the same actor. */
    void AutoBindComponents();

    /** (Re)bind all necessary event handlers to the current DialogueComponent. */
    void BindDialogueEvents();
    void UnbindDialogueEvents();

    /** Build a narrative?style instigator object for the current owner/dialogue. */
    FSFNarrativeInstigator BuildInstigator() const;

    /** Convert current SFDialogue display data into an FSFDialogueMoment. */
    FSFDialogueMoment BuildDialogueMoment(const FSFDialogueDisplayData& DisplayData) const;

    /** Emit a narrative delta and forward it to the narrative component and delegate. */
    void EmitNarrativeDelta(const FSFNarrativeDelta& Delta);

protected:
    //
    // Bound components
    //

    UPROPERTY(Transient)
    TObjectPtr<USFDialogueComponent> DialogueComponent = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USFNarrativeComponent> NarrativeComponent = nullptr;

    //
    // Runtime state
    //

    /** Logical ID for the current conversation (usually the asset’s name). */
    UPROPERTY(Transient)
    FName CurrentDialogueId = NAME_None;

    /** Monotonic sequence number for dialogue?related deltas. */
    UPROPERTY(Transient)
    int32 DialogueDeltaSequence = 0;

    /** Last constructed dialogue moment snapshot. */
    UPROPERTY(Transient)
    FSFDialogueMoment LastDialogueMoment;

    //
    // Handlers wired into USFDialogueComponent’s delegates
    //

    UFUNCTION()
    void HandleConversationStarted(AActor* SourceActor);

    UFUNCTION()
    void HandleConversationEnded();

    UFUNCTION()
    void HandleDialogueNodeUpdated(const FSFDialogueDisplayData& DisplayData);

    UFUNCTION()
    void HandleDialogueEventTriggered(FGameplayTag EventTag);

    UFUNCTION()
    void HandleDialogueCameraShotChanged(const FSFDialogueCameraShot& CameraShot);
};
