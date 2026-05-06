#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeStructs.h"
#include "SFNarrativeFactSubsystem.generated.h"

class USFNarrativeStateSubsystem;

/**
 * Read?optimized façade over the world?level narrative state focused on
 * *facts* and simple derived queries.
 *
 * Storage and mutation live in USFNarrativeStateSubsystem; this subsystem
 * is for fast lookups, convenience helpers, and Blueprint?friendly predicates.
 */
UCLASS()
class SIGNALFORGERPG_API USFNarrativeFactSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Return the underlying state subsystem (canonical data store). */
    UFUNCTION(BlueprintPure, Category = "Narrative|Facts")
    USFNarrativeStateSubsystem* GetStateSubsystem() const;

    //
    // Direct fact access
    //

    /** Get a world fact by key. Returns true if found. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Facts")
    bool GetFact(const FSFWorldFactKey& Key, FSFWorldFactValue& OutValue) const;

    /** True if a fact with this key exists. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Facts")
    bool HasFact(const FSFWorldFactKey& Key) const;

    /** True if a fact tag exists for ANY context. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Facts")
    bool HasAnyFactWithTag(FGameplayTag FactTag) const;

    /** Collect all facts with a given tag (any context). */
    UFUNCTION(BlueprintPure, Category = "Narrative|Facts")
    void GetFactsByTag(FGameplayTag FactTag, TArray<FSFWorldFactSnapshot>& OutFacts) const;

    /** Collect all facts for a given ContextId (any tag). */
    UFUNCTION(BlueprintPure, Category = "Narrative|Facts")
    void GetFactsByContext(FName ContextId, TArray<FSFWorldFactSnapshot>& OutFacts) const;

    /** Return all facts. Primarily for debugging / inspection. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Facts")
    void GetAllFacts(TArray<FSFWorldFactSnapshot>& OutFacts) const;

    //
    // Typed convenience queries
    //

    /** Bool fact == Expected. Missing fact counts as false. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Facts")
    bool IsBoolFactEqual(const FSFWorldFactKey& Key, bool bExpected) const;

    /** Name fact equals ExpectedName (or not). Missing fact counts as not equal. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Facts")
    bool IsNameFactEqual(const FSFWorldFactKey& Key, FName ExpectedName) const;

    /** Tag fact container has RequiredTag. Missing fact counts as false. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Facts")
    bool DoesTagFactContain(const FSFWorldFactKey& Key, FGameplayTag RequiredTag) const;

    /** Numeric fact (int/float) meets numeric condition. Missing fact counts as failure. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Facts")
    bool DoesNumericFactPassCondition(
        const FSFWorldFactKey& Key,
        const FSFNarrativeNumericCondition& Condition) const;

    //
    // Condition?set evaluation (WorldFact domain only)
    //

    /**
     * Evaluate all WorldFact?domain conditions in a condition set against the
     * current world facts. Other domains (QuestState, Faction, Identity, etc.)
     * are ignored here and should be handled by their own subsystems.
     */
    UFUNCTION(BlueprintPure, Category = "Narrative|Facts")
    bool EvaluateWorldFactConditionsInSet(const FSFNarrativeConditionSet& ConditionSet) const;

protected:
    /** Cached pointer to the canonical state subsystem for quick access. */
    UPROPERTY(Transient)
    TObjectPtr<USFNarrativeStateSubsystem> CachedStateSubsystem = nullptr;

    bool EvaluateWorldFactCondition(const FSFNarrativeWorldFactCondition& Condition) const;
};