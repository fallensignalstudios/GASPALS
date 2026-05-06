// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"

#include "SFNarrativeTypes.h"
#include "SFNarrativeSaveTypes.h"
#include "SFNarrativeBlueprintLibrary.generated.h"

class USFNarrativeStateSubsystem;
class USFNarrativeFactSubsystem;
class USFQuestDefinition;
class USFQuestInstance;

/**
 * High-level Blueprint helpers for working with the narrative system.
 *
 * This is the primary entry point for designers using Blueprints:
 *  - Evaluate condition sets
 *  - Apply narrative deltas
 *  - Quick fact / faction / identity helpers
 */
UCLASS()
class SIGNALFORGERPG_API USFNarrativeBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    //
    // Subsystem access
    //

    UFUNCTION(BlueprintPure, Category = "Narrative", meta = (WorldContext = "WorldContextObject"))
    static USFNarrativeStateSubsystem* GetNarrativeState(const UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, Category = "Narrative", meta = (WorldContext = "WorldContextObject"))
    static USFNarrativeFactSubsystem* GetNarrativeFacts(const UObject* WorldContextObject);

    //
    // Conditions
    //

    /** Evaluate a condition set against the current narrative state (+optional quest instance). */
    UFUNCTION(BlueprintPure, Category = "Narrative|Conditions", meta = (WorldContext = "WorldContextObject"))
    static bool EvaluateNarrativeConditionSet(
        const UObject* WorldContextObject,
        const FSFNarrativeConditionSet& ConditionSet,
        const USFQuestDefinition* QuestDef,
        const USFQuestInstance* QuestInstance);

    //
    // Deltas
    //

    /** Apply a single narrative delta to the world state. */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Delta", meta = (WorldContext = "WorldContextObject"))
    static bool ApplyNarrativeDelta(
        const UObject* WorldContextObject,
        const FSFNarrativeDelta& Delta,
        FSFNarrativeChangeSet& OutChangeSet);

    /** Apply a batch of narrative deltas (e.g. from dialogue choice or event row). */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Delta", meta = (WorldContext = "WorldContextObject"))
    static bool ApplyNarrativeDeltas(
        const UObject* WorldContextObject,
        const TArray<FSFNarrativeDelta>& Deltas,
        FSFNarrativeChangeSet& OutCombinedChangeSet);

    //
    // World fact helpers
    //

    UFUNCTION(BlueprintCallable, Category = "Narrative|Facts", meta = (WorldContext = "WorldContextObject"))
    static void SetBoolFact(
        const UObject* WorldContextObject,
        const FSFWorldFactKey& Key,
        bool bValue);

    UFUNCTION(BlueprintPure, Category = "Narrative|Facts", meta = (WorldContext = "WorldContextObject"))
    static bool GetBoolFact(
        const UObject* WorldContextObject,
        const FSFWorldFactKey& Key,
        bool& bOutValue);

    //
    // Faction helpers
    //

    UFUNCTION(BlueprintPure, Category = "Narrative|Faction", meta = (WorldContext = "WorldContextObject"))
    static ESFFactionStandingBand GetFactionBand(
        const UObject* WorldContextObject,
        FGameplayTag FactionTag);

    //
    // Identity axis helpers
    //

    UFUNCTION(BlueprintPure, Category = "Narrative|Identity", meta = (WorldContext = "WorldContextObject"))
    static float GetIdentityAxisValue(
        const UObject* WorldContextObject,
        FGameplayTag AxisTag,
        bool& bOutHasValue);
};
