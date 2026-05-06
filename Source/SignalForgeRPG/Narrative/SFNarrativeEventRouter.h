#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeStructs.h"
#include "SFNarrativeConstants.h"
#include "SFNarrativeEventRouter.generated.h"

class USFNarrativeEventHub;
class USFQuestInstance;

/**
 * Routes low?level FSFNarrativeDelta instances and higher?level FSFNarrativeChangeSet
 * objects into the blueprint?facing USFNarrativeEventHub.
 *
 * This class is intentionally dumb?stateful: it only tracks what it needs to decide
 * which hub events to fire (e.g., last known standing band for a faction).
 *
 * Typical flow:
 *   - USFNarrativeComponent receives a delta/change set from gameplay.
 *   - Component calls EventRouter->RouteDelta(...) and/or RouteChangeSet(...).
 *   - Router inspects the data and invokes methods on USFNarrativeEventHub.
 */
UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFNarrativeEventRouter : public UObject
{
    GENERATED_BODY()

public:
    /** Initialize the router with an event hub and (optionally) an owning narrative component. */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Routing")
    void Initialize(USFNarrativeEventHub* InEventHub, UObject* InOwnerContext);

    /** The event hub we route into. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Routing")
    USFNarrativeEventHub* GetEventHub() const { return EventHub; }

    /** Route a single narrative delta into the hub. */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Routing")
    void RouteDelta(const FSFNarrativeDelta& Delta);

    /** Route a high?level change set into the hub. */
    UFUNCTION(BlueprintCallable, Category = "Narrative|Routing")
    void RouteChangeSet(const FSFNarrativeChangeSet& ChangeSet);

protected:
    /** Hub to broadcast authored events through. */
    UPROPERTY(Transient)
    TObjectPtr<USFNarrativeEventHub> EventHub = nullptr;

    /** Optional context (narrative component, world, player, etc.) for resolving quest instances and such. */
    UPROPERTY(Transient)
    TObjectPtr<UObject> OwnerContext = nullptr;

    /** Lightweight cache of last known faction standings so we can detect band transitions. */
    UPROPERTY(Transient)
    TMap<FGameplayTag, FSFFactionStandingValue> CachedFactionStanding;

    /** Lightweight cache of last known identity axis values. */
    UPROPERTY(Transient)
    TMap<FGameplayTag, float> CachedIdentityValues;

protected:
    //
    // Internal helpers – one per domain.
    //

    void RouteQuestDelta(const FSFNarrativeDelta& Delta);
    void RouteDialogueDelta(const FSFNarrativeDelta& Delta);
    void RouteWorldFactDelta(const FSFNarrativeDelta& Delta);
    void RouteFactionDelta(const FSFNarrativeDelta& Delta);
    void RouteIdentityDelta(const FSFNarrativeDelta& Delta);
    void RouteOutcomeDelta(const FSFNarrativeDelta& Delta);
    void RouteEndingDelta(const FSFNarrativeDelta& Delta);
    void RouteSystemDelta(const FSFNarrativeDelta& Delta);

    void RouteTaskResults(const TArray<FSFTaskProgressResult>& TaskResults);
    void RouteWorldFactChanges(const TArray<FSFWorldFactChange>& FactChanges);
    void RouteFactionChanges(const TArray<FSFFactionChange>& FactionChanges);
    void RouteIdentityChanges(const TArray<FSFIdentityChange>& IdentityChanges);
    void RouteAppliedOutcomes(const TArray<FSFOutcomeApplication>& Outcomes);
    void RouteEndingStates(const TArray<FSFEndingState>& EndingStates);

    /** Convenience method to try to resolve a quest instance by ID from OwnerContext, if supported. */
    USFQuestInstance* ResolveQuestInstance(FName QuestId) const;
};