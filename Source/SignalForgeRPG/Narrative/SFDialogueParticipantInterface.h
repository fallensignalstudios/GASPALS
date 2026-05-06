// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "SFNarrativeTypes.h"
#include "SFDialogueParticipantInterface.generated.h"

class USFConversationDataAsset;
class USFDialogueComponent;
class USFNarrativeComponent;

/**
 * Marked Blueprintable so Blueprint classes can implement this interface
 * (required for the BlueprintNativeEvent members below — UHT rejects
 * Blueprint Event members on non-Blueprintable interfaces).
 */
UINTERFACE(BlueprintType, Blueprintable)
class SIGNALFORGERPG_API USFDialogueParticipantInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * Interface implemented by actors that can participate in dialogue.
 *
 * This keeps USFDialogueComponent and the narrative system decoupled from
 * specific character classes – any pawn/actor can become a participant by
 * implementing this interface.
 */
class SIGNALFORGERPG_API ISFDialogueParticipantInterface
{
    GENERATED_BODY()

public:

    //
    // Identity / presentation
    //

    /** Unique participant ID used inside conversation data (e.g. SpeakerId on nodes). */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue|Participant")
    FName GetDialogueParticipantId() const;

    /** Display name for UI when this participant is speaking. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue|Participant")
    FText GetDialogueDisplayName() const;

    /** Optional portrait / icon for UI. Can be null if unused. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue|Participant")
    UTexture2D* GetDialoguePortrait() const;

    //
    // Tag surface (for gating lines/choices)
    //

    /**
     * Tags used when evaluating dialogue node/choice tag checks.
     * This should mirror what USFDialogueComponent::GetOwnedDialogueTags wants:
     *  - character state tags
     *  - quest progress tags
     *  - relationship/faction tags
     *  - world facts relevant to this participant
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue|Participant")
    void GetDialogueTags(FGameplayTagContainer& OutTags) const;

    //
    // Lifecycle hooks
    //

    /** Called when a conversation involving this participant starts. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue|Participant")
    void OnDialogueStarted(USFConversationDataAsset* Conversation, USFDialogueComponent* OwningComponent);

    /** Called when a conversation involving this participant ends. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue|Participant")
    void OnDialogueEnded(USFConversationDataAsset* Conversation, ESFDialogueExitReason ExitReason);

    /** Called when this participant becomes the active speaker for a dialogue node. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue|Participant")
    void OnDialogueLineSpoken(const FSFDialogueDisplayData& DisplayData);

    /** Called when the local player selects a choice addressed to this participant (e.g. for reaction). */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue|Participant")
    void OnDialogueChoiceSelected(const FSFDialogueChoice& ChoiceData);

    //
    // Narrative integration
    //

    /** Optional: return a narrative component associated with this participant, if any. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue|Participant")
    USFNarrativeComponent* GetAssociatedNarrativeComponent() const;

    /** Optional: return the world/context ID this participant should use for narrative facts. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue|Participant")
    FName GetNarrativeContextId() const;
};
