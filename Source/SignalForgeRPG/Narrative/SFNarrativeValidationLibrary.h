// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"

#include "SFNarrativeTypes.h"
#include "SFNarrativeSaveTypes.h"
#include "SFNarrativeEventTableRow.h"
#include "SFDialogueNarrativeMap.h"
#include "SFQuestDatabase.h"
#include "SFNarrativeValidationLibrary.generated.h"

class USFQuestDefinition;
class USFNarrativeComponent;

/** Result severity. */
UENUM(BlueprintType)
enum class ESFNarrativeValidationSeverity : uint8
{
    Info,
    Warning,
    Error
};

/** Single validation message. */
USTRUCT(BlueprintType)
struct FSFNarrativeValidationMessage
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Validation")
    ESFNarrativeValidationSeverity Severity = ESFNarrativeValidationSeverity::Info;

    /** Optional asset or owner this message refers to. */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Validation")
    TSoftObjectPtr<UObject> Asset;

    /** Optional identifier (quest id, event id, dialogue key, etc.). */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Validation")
    FName SubjectId = NAME_None;

    /** Human-readable message. */
    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Validation")
    FText Message;
};

/** Aggregate result. */
USTRUCT(BlueprintType)
struct FSFNarrativeValidationResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Validation")
    TArray<FSFNarrativeValidationMessage> Messages;

    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Validation")
    int32 ErrorCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Narrative|Validation")
    int32 WarningCount = 0;

    bool HasErrors()  const { return ErrorCount > 0; }
    bool HasWarnings()const { return WarningCount > 0; }
    bool IsClean()    const { return ErrorCount == 0 && WarningCount == 0; }

    void Reset()
    {
        Messages.Reset();
        ErrorCount = 0;
        WarningCount = 0;
    }

    void AddMessage(
        ESFNarrativeValidationSeverity Severity,
        const FText& Text,
        UObject* InAsset = nullptr,
        FName   InSubjectId = NAME_None)
    {
        FSFNarrativeValidationMessage& Msg = Messages.Emplace_GetRef();
        Msg.Severity = Severity;
        Msg.Asset = InAsset;
        Msg.SubjectId = InSubjectId;
        Msg.Message = Text;

        if (Severity == ESFNarrativeValidationSeverity::Error)
        {
            ++ErrorCount;
        }
        else if (Severity == ESFNarrativeValidationSeverity::Warning)
        {
            ++WarningCount;
        }
    }

    void AddError(const FText& Text, UObject* InAsset = nullptr, FName InSubjectId = NAME_None)
    {
        AddMessage(ESFNarrativeValidationSeverity::Error, Text, InAsset, InSubjectId);
    }

    void AddWarning(const FText& Text, UObject* InAsset = nullptr, FName InSubjectId = NAME_None)
    {
        AddMessage(ESFNarrativeValidationSeverity::Warning, Text, InAsset, InSubjectId);
    }

    void AddInfo(const FText& Text, UObject* InAsset = nullptr, FName InSubjectId = NAME_None)
    {
        AddMessage(ESFNarrativeValidationSeverity::Info, Text, InAsset, InSubjectId);
    }

    /** Merge another result into this one, preserving messages and counts. */
    void Merge(const FSFNarrativeValidationResult& Other)
    {
        Messages.Append(Other.Messages);
        ErrorCount += Other.ErrorCount;
        WarningCount += Other.WarningCount;
    }
};

/**
 * Dev-only validation helpers for narrative content and integration.
 *
 * Intended for editor tools, commandlets, and tests – not runtime shipping code.
 */
UCLASS()
class SIGNALFORGERPG_API USFNarrativeValidationLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    //
    // Quest validation
    //

    /** Validate a single quest definition for structural issues. */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Validation")
    static FSFNarrativeValidationResult ValidateQuestDefinition(USFQuestDefinition* QuestDef);

    /** Validate all quests in a quest database (duplicates, broken quests, etc.). */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Validation")
    static FSFNarrativeValidationResult ValidateQuestDatabase(USFQuestDatabase* QuestDatabase);

    //
    // Condition-set validation
    //

    /** Basic static checks on a condition set (empty domains, invalid tags, etc.). */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Validation")
    static FSFNarrativeValidationResult ValidateConditionSet(const FSFNarrativeConditionSet& ConditionSet);

    //
    // Event table validation
    //

    /** Validate all rows in a narrative event table. */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Validation")
    static FSFNarrativeValidationResult ValidateEventTable(UDataTable* EventTable);

    //
    // Dialogue map validation
    //

    /** Validate a dialogue→narrative map for duplicate keys and empty hooks. */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Validation")
    static FSFNarrativeValidationResult ValidateDialogueNarrativeMap(USFDialogueNarrativeMap* MapAsset);

    //
    // Delta / change-set validation
    //

    /** Validate a single narrative delta (domain, tags, asset references). */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Validation")
    static FSFNarrativeValidationResult ValidateNarrativeDelta(const FSFNarrativeDelta& Delta);

    /** Validate an array of narrative deltas. */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Validation")
    static FSFNarrativeValidationResult ValidateNarrativeDeltas(const TArray<FSFNarrativeDelta>& Deltas);

    //
    // Component / wiring validation
    //

    /**
     * Validate that an actor's narrative component and collaborators are wired
     * according to the current architecture (no missing dependencies, no
     * duplicate runtime owners, etc.).
     */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Validation")
    static FSFNarrativeValidationResult ValidateNarrativeComponentConfiguration(AActor* OwnerActor);

    //
    // Save/replication schema validation
    //

    /** Static checks on a snapshot/save structure (versioning, required fields, etc.). */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Validation")
    static FSFNarrativeValidationResult ValidateNarrativeSaveData(const FSFNarrativeSaveData& SaveData);

    //
    // Aggregate project audit
    //

    /**
     * High-level project audit: run all relevant validators in one call.
     * Any parameter can be null; those checks will be skipped.
     */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Validation")
    static FSFNarrativeValidationResult ValidateNarrativeProject(
        USFQuestDatabase* QuestDatabase,
        USFDialogueNarrativeMap* DialogueMap,
        UDataTable* EventTable,
        AActor* ExampleOwnerActor);
};
