// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeStructs.h"
#include "SFNarrativeStateSubsystem.generated.h"

/**
 * World-scoped store for narrative state:
 *  - World facts
 *  - Faction standings
 *  - Identity axes
 *  - Ending scores + availability
 *
 * This subsystem should be the single source of truth for narrative data.
 * USFNarrativeComponent and other systems interact with it via
 * FSFNarrativeDelta / FSFNarrativeChangeSet.
 */
UCLASS()
class SIGNALFORGERPG_API USFNarrativeStateSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    //
    // USubsystem
    //

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    //
    // World facts
    //

    UFUNCTION(BlueprintCallable, Category = "Narrative|State|WorldFacts")
    bool SetWorldFact(const FSFWorldFactKey& Key, const FSFWorldFactValue& NewValue, FSFWorldFactValue& OutOldValue);

    UFUNCTION(BlueprintCallable, Category = "Narrative|State|WorldFacts")
    bool RemoveWorldFact(const FSFWorldFactKey& Key, FSFWorldFactValue& OutOldValue);

    UFUNCTION(BlueprintPure, Category = "Narrative|State|WorldFacts")
    bool GetWorldFact(const FSFWorldFactKey& Key, FSFWorldFactValue& OutValue) const;

    UFUNCTION(BlueprintPure, Category = "Narrative|State|WorldFacts")
    bool HasWorldFact(const FSFWorldFactKey& Key) const;

    UFUNCTION(BlueprintPure, Category = "Narrative|State|WorldFacts")
    void GetAllWorldFacts(TArray<FSFWorldFactSnapshot>& OutFacts) const;

    //
    // Factions
    //

    UFUNCTION(BlueprintCallable, Category = "Narrative|State|Factions")
    FSFFactionStandingValue ApplyFactionDelta(const FSFFactionDelta& Delta, FSFFactionStandingValue& OutOldValue);

    UFUNCTION(BlueprintPure, Category = "Narrative|State|Factions")
    bool GetFactionStanding(const FGameplayTag FactionTag, FSFFactionStandingValue& OutValue) const;

    UFUNCTION(BlueprintPure, Category = "Narrative|State|Factions")
    ESFFactionStandingBand GetFactionStandingBand(const FGameplayTag FactionTag) const;

    //
    // Identity axes
    //

    UFUNCTION(BlueprintCallable, Category = "Narrative|State|Identity")
    float ApplyIdentityDelta(const FGameplayTag AxisTag, float Delta, float& OutOldValue, ESFIdentityDriftDirection& OutDriftDirection);

    UFUNCTION(BlueprintPure, Category = "Narrative|State|Identity")
    bool GetIdentityAxisValue(const FGameplayTag AxisTag, float& OutValue) const;

    //
    // Endings
    //

    UFUNCTION(BlueprintCallable, Category = "Narrative|State|Endings")
    float ApplyEndingScoreDelta(const FGameplayTag EndingTag, float Delta, float& OutOldScore);

    UFUNCTION(BlueprintCallable, Category = "Narrative|State|Endings")
    bool SetEndingAvailability(const FGameplayTag EndingTag, ESFEndingAvailability NewAvailability, ESFEndingAvailability& OutOldAvailability);

    UFUNCTION(BlueprintPure, Category = "Narrative|State|Endings")
    bool GetEndingState(const FGameplayTag EndingTag, FSFEndingState& OutState) const;

    //
    // Delta application
    //

    /**
     * Apply a single narrative delta and produce a corresponding change set
     * entry (if any). This does NOT broadcast events; USFNarrativeComponent
     * or other systems can do that after the fact.
     */
    UFUNCTION(BlueprintCallable, Category = "Narrative|State|Delta")
    bool ApplyDelta(const FSFNarrativeDelta& Delta, FSFNarrativeChangeSet& OutChangeSet);

    //
    // Save/load
    //

    UFUNCTION(BlueprintPure, Category = "Narrative|State|Save")
    FSFNarrativeSaveData BuildSaveData() const;

    UFUNCTION(BlueprintCallable, Category = "Narrative|State|Save")
    bool LoadFromSaveData(const FSFNarrativeSaveData& SaveData);

protected:
    //
    // Internal storage
    //

    /** Global world facts. */
    UPROPERTY()
    TMap<FSFWorldFactKey, FSFWorldFactValue> WorldFacts;

    /** Global faction standings. */
    UPROPERTY()
    TMap<FGameplayTag, FSFFactionStandingValue> FactionStandingByTag;

    /** Global identity axes. */
    UPROPERTY()
    TMap<FGameplayTag, float> IdentityAxisValues;

    /** Ending scores and availability. */
    UPROPERTY()
    TMap<FGameplayTag, FSFEndingState> EndingStateByTag;
};
