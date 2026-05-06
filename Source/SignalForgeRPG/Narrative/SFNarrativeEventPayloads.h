// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeStructs.h"
#include "SFNarrativeEventPayloads.generated.h"

class USFQuestInstance;
class USFConversationDataAsset;
class USFNarrativeComponent;
class AActor;
class APlayerState;

/**
 * Base envelope for any narrative event, carrying generic context.
 * More specific payloads embed this as a field.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeEventContext
{
    GENERATED_BODY()

    /** Owning narrative component (player, world, etc.) that raised this event. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Event")
    TObjectPtr<USFNarrativeComponent> NarrativeComponent = nullptr;

    /** Optional player state primarily responsible for this event. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Event")
    TObjectPtr<APlayerState> PlayerState = nullptr;

    /** Optional primary actor associated with the event (quest giver, target, speaker, etc.). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Event")
    TObjectPtr<AActor> FocusActor = nullptr;

    /** Arbitrary context identifier (location ID, encounter ID, etc.). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Event")
    FName ContextId = NAME_None;

    /** Additional tags describing this specific event instance (phase, mood, channel, etc.). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Event")
    FGameplayTagContainer Tags;
};

/**
 * Quest?related event payload.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeQuestEventPayload
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest")
    FSFNarrativeEventContext Context;

    /** Quest instance, if available. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest")
    TObjectPtr<USFQuestInstance> QuestInstance = nullptr;

    /** Short quest identifier for log/analytics/etc. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest")
    FName QuestId = NAME_None;

    /** Optional quest asset identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest")
    FPrimaryAssetId QuestAssetId;

    /** New completion state (if the event represents a completion?state change). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest")
    ESFQuestCompletionState CompletionState = ESFQuestCompletionState::NotStarted;

    /** State identifier relevant to this event (entered, left, etc.). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest")
    FName StateId = NAME_None;

    /** Task data, if this event concerns a specific task. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest")
    FSFTaskProgressKey TaskKey;

    /** New task count after an operation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest")
    int32 NewTaskCount = 0;

    /** Delta that was applied to the task. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest")
    int32 TaskDelta = 0;

    /** True if this event represents the task becoming completed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Quest")
    bool bTaskCompleted = false;
};

/**
 * Dialogue?related event payload.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeDialogueEventPayload
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    FSFNarrativeEventContext Context;

    /** Underlying dialogue asset, if known. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    TObjectPtr<USFConversationDataAsset> Conversation = nullptr;

    /** Logical dialogue identifier (usually the asset name). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    FName DialogueId = NAME_None;

    /** Node identifier currently in focus. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    FName NodeId = NAME_None;

    /** Snapshot of the current dialogue moment (line + options) if applicable. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    FSFDialogueMoment Moment;

    /** Exit reason (used when conversations end). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    ESFDialogueExitReason ExitReason = ESFDialogueExitReason::Completed;

    /** Event tag for Event nodes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    FGameplayTag EventTag;

    /** Choice information if this payload represents a player selection. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    FSFDialogueOptionView SelectedOption;

    /** Optional speaker responsible for this event (player or NPC). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Dialogue")
    FName SpeakerId = NAME_None;
};

/**
 * World?fact event payload.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeWorldFactEventPayload
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|World")
    FSFNarrativeEventContext Context;

    /** Which fact changed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|World")
    FSFWorldFactKey Key;

    /** Previous value (if known). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|World")
    FSFWorldFactValue OldValue;

    /** New value (for “removed” events this may be empty). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|World")
    FSFWorldFactValue NewValue;

    /** True if this represents a removal instead of an update. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|World")
    bool bRemoved = false;
};

/**
 * Faction standing event payload.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeFactionEventPayload
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Factions")
    FSFNarrativeEventContext Context;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Factions")
    FGameplayTag FactionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Factions")
    FSFFactionStandingValue OldStanding;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Factions")
    FSFFactionStandingValue NewStanding;

    /** New band; OldStanding.StandingBand is the previous one. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Factions")
    ESFFactionStandingBand NewStandingBand = ESFFactionStandingBand::Unknown;
};

/**
 * Identity axis event payload.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeIdentityEventPayload
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Identity")
    FSFNarrativeEventContext Context;

    /** Axis that changed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Identity")
    FGameplayTag AxisTag;

    /** Old value for the axis. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Identity")
    float OldValue = 0.0f;

    /** New value for the axis. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Identity")
    float NewValue = 0.0f;

    /** Interpreted drift direction. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Identity")
    ESFIdentityDriftDirection DriftDirection = ESFIdentityDriftDirection::None;
};

/**
 * Outcome application payload.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeOutcomeEventPayload
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Outcomes")
    FSFNarrativeEventContext Context;

    /** Outcome that was applied. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Outcomes")
    FSFOutcomeApplication Application;
};

/**
 * Ending event payload.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeEndingEventPayload
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Endings")
    FSFNarrativeEventContext Context;

    /** Which ending this payload concerns. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Endings")
    FGameplayTag EndingTag;

    /** New availability for the ending. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Endings")
    ESFEndingAvailability Availability = ESFEndingAvailability::Unknown;

    /** Current score for this ending path. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Endings")
    float Score = 0.0f;

    /** Reasons we consider this ending locked (if applicable). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Endings")
    TArray<FGameplayTag> LockedReasonTags;
};

/**
 * Save / load / checkpoint payload.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeSaveEventPayload
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save")
    FSFNarrativeEventContext Context;

    /** Name of the save slot involved. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save")
    FString SlotName;

    /** Current serialized state, when relevant (e.g. BeforeApplyLoadedState / AfterApplyLoadedState). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save")
    FSFNarrativeSaveData SaveData;

    /** Redirect table used when upgrading or migrating this save. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save")
    FSFNarrativeRedirectTable RedirectTable;

    /** True if this event represents a checkpoint rather than a full user?initiated save. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Save")
    bool bIsCheckpoint = false;
};

/**
 * Generic “narrative change set” payload – primarily for logging and analytics.
 */
USTRUCT(BlueprintType)
struct FSFNarrativeChangeSetEventPayload
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Changes")
    FSFNarrativeEventContext Context;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Narrative|Changes")
    FSFNarrativeChangeSet ChangeSet;
};
