// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeStructs.h" // Canonical FSFDialogueOptionView / FSFDialogueMoment
#include "SFDialogueStateSnapshot.generated.h"

class USFConversationDataAsset;

/**
 * Arbitrary key/value pair for dialogue-session-scoped variables, serialized
 * as strings for simplicity. You can interpret them as ints/floats/enums as
 * needed in your runtime.
 */
USTRUCT(BlueprintType)
struct FSFDialogueVariableEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Snapshot")
    FName Key = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Snapshot")
    FString Value;
};

/**
 * High-level snapshot of an in-progress dialogue session.
 *
 * This is intentionally runtime-agnostic; it does not store pointers to
 * participants, components, or the dialogue runtime - only IDs and data
 * sufficient to restore/continue the conversation.
 *
 * NOTE: FSFDialogueOptionView and FSFDialogueMoment live in
 * SFNarrativeStructs.h and are the canonical view/runtime types. This
 * snapshot reuses them rather than redeclaring its own.
 */
USTRUCT(BlueprintType)
struct FSFDialogueStateSnapshot
{
    GENERATED_BODY()

    /** Owning conversation asset (soft reference for save/load). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Snapshot")
    TSoftObjectPtr<USFConversationDataAsset> Conversation;

    /** Logical dialogue identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Snapshot")
    FName DialogueId = NAME_None;

    /** Optional per-session identifier (helps you distinguish multiple parallel sessions). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Snapshot")
    FGuid SessionId;

    /** Participant ID for the local player (if applicable). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Snapshot")
    FName PlayerSpeakerId = NAME_None;

    /** Participant ID for the primary non-player speaker (NPC, object, etc.). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Snapshot")
    FName OtherSpeakerId = NAME_None;

    /** Current dialogue moment (speaker, line, choices). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Snapshot")
    FSFDialogueMoment CurrentMoment;

    /** All node IDs that have been visited in this session so far. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Snapshot")
    TArray<FName> VisitedNodeIds;

    /** Branch tags accumulated by choices and nodes (for analytics or gating). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Snapshot")
    FGameplayTagContainer AccumulatedBranchTags;

    /** Arbitrary per-session variables captured as key/value pairs. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Snapshot")
    TArray<FSFDialogueVariableEntry> Variables;

    /** True if this snapshot represents a finished conversation (no further choices). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Snapshot")
    bool bIsCompleted = false;

    /** Exit reason if completed (mirrors ESFDialogueExitReason). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Snapshot")
    ESFDialogueExitReason ExitReason = ESFDialogueExitReason::Cancelled;

    /** Utility: returns true if the snapshot has a valid conversation and node id. */
    bool IsValidActive() const
    {
        return Conversation.IsValid() || !DialogueId.IsNone();
    }

    /** Utility: get a variable by key; returns empty string if not found. */
    FString GetVariableValue(FName Key) const
    {
        for (const FSFDialogueVariableEntry& Entry : Variables)
        {
            if (Entry.Key == Key)
            {
                return Entry.Value;
            }
        }
        return FString();
    }

    /** Utility: set or update a variable. */
    void SetVariableValue(FName Key, const FString& NewValue)
    {
        for (FSFDialogueVariableEntry& Entry : Variables)
        {
            if (Entry.Key == Key)
            {
                Entry.Value = NewValue;
                return;
            }
        }

        FSFDialogueVariableEntry NewEntry;
        NewEntry.Key = Key;
        NewEntry.Value = NewValue;
        Variables.Add(MoveTemp(NewEntry));
    }
};
