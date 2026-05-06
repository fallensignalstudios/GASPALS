// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeSaveTypes.h"
#include "SFNarrativeFunctionLibrary.generated.h"

/**
 * Lower-level narrative helpers: merging change sets, constructing
 * common deltas, and small math/utility functions.
 *
 * This is separate from SFNarrativeBlueprintLibrary so you can call
 * into it freely from C++ without dragging in subsystem accessors.
 */
UCLASS()
class SIGNALFORGERPG_API USFNarrativeFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    //
    // Change-set helpers
    //

    /** Merge B into A (A becomes A ? B, preferring B for overlapping keys). */
    UFUNCTION(BlueprintCallable, Category = "Narrative|ChangeSet")
    static void MergeChangeSets(FSFNarrativeChangeSet& InOutA, const FSFNarrativeChangeSet& B);

    /** Returns true if the change set has no changes. */
    UFUNCTION(BlueprintPure, Category = "Narrative|ChangeSet")
    static bool IsChangeSetEmpty(const FSFNarrativeChangeSet& ChangeSet);

    //
    // Delta constructors
    //

    /** Build a simple world-fact delta (bool). */
    UFUNCTION(BlueprintPure, Category = "Narrative|Delta", meta = (DisplayName = "Make Bool Fact Delta"))
    static FSFNarrativeDelta MakeBoolFactDelta(
        const FSFWorldFactKey& FactKey,
        bool bNewValue);

    /** Build a simple numeric world-fact delta (float). */
    UFUNCTION(BlueprintPure, Category = "Narrative|Delta", meta = (DisplayName = "Make Float Fact Delta"))
    static FSFNarrativeDelta MakeFloatFactDelta(
        const FSFWorldFactKey& FactKey,
        float NewValue);

    /** Build an additive identity-axis delta (e.g. shift axis by +/-N). */
    UFUNCTION(BlueprintPure, Category = "Narrative|Delta", meta = (DisplayName = "Make Identity Axis Add Delta"))
    static FSFNarrativeDelta MakeIdentityAxisAddDelta(
        FGameplayTag AxisTag,
        float DeltaAmount);

    /** Build a faction standing change delta (additive). */
    UFUNCTION(BlueprintPure, Category = "Narrative|Delta", meta = (DisplayName = "Make Faction Standing Add Delta"))
    static FSFNarrativeDelta MakeFactionStandingAddDelta(
        FGameplayTag FactionTag,
        float DeltaAmount);

    //
    // Misc small helpers
    //

    /** True if the delta is effectively a no-op or carries no usable payload. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Delta")
    static bool IsDeltaNoOp(const FSFNarrativeDelta& Delta);

    /** Normalize a scalar to [0,1] given min/max, clamped. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Math")
    static float NormalizeClamped(float Value, float Min, float Max);
};
