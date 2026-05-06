// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "SFNarrativeTypes.h"
#include "SFQuestRewardDefinition.generated.h"

/**
 * Strategy for selecting rewards from a reward set.
 */
UENUM(BlueprintType)
enum class ESFQuestRewardSelectionMode : uint8
{
    All,            // Grant all rewards in the set.
    RandomOne,      // Pick exactly one reward entry at random.
    RandomSubset,   // Pick a random subset using Weights and MaxSelected.
    Custom          // Let game-specific code interpret the entries.
};

/**
 * Non-item, non-asset “simple” rewards that are very common.
 */
USTRUCT(BlueprintType)
struct FSFSimpleReward
{
    GENERATED_BODY()

    /** Experience points to grant (player, class, weapon, etc. – up to your game to route). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward")
    int32 Experience = 0;

    /** Generic currency amount (gold, credits, etc.). Use CurrencyTag to distinguish. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward")
    int32 CurrencyAmount = 0;

    /** Tag describing which currency this refers to (e.g. Economy.Currency.Gold). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward")
    FGameplayTag CurrencyTag;

    /** Arbitrary tags to add to the player / world when this reward is granted. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward")
    FGameplayTagContainer GrantedTags;
};

/**
 * A single “item-like” reward entry – could be an inventory item, recipe, ability, etc.
 * Actual handling is up to your inventory/ability systems; this just carries identifiers.
 */
USTRUCT(BlueprintType)
struct FSFRewardItemDefinition
{
    GENERATED_BODY()

    /** Optional soft reference to an item asset you define elsewhere. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward")
    TSoftObjectPtr<UObject> ItemAsset;

    /** Fallback / alternate identifier if you don’t want to use soft refs (e.g. row name). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward")
    FName ItemId = NAME_None;

    /** How many of this item to grant. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward", meta = (ClampMin = "1"))
    int32 Quantity = 1;

    /** Optional category tag for routing (e.g. Reward.Item.Weapon). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward")
    FGameplayTag ItemCategoryTag;

    /** Weight used when randomly selecting rewards from a pool. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward", meta = (ClampMin = "0.0"))
    float Weight = 1.0f;
};

/**
 * Reward that modifies faction standing as part of a quest resolution.
 */
USTRUCT(BlueprintType)
struct FSFRewardFactionDelta
{
    GENERATED_BODY()

    /** Which faction is affected. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward")
    FGameplayTag FactionTag;

    /** Standing delta to apply; semantics are up to your faction system. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward")
    float StandingDelta = 0.0f;

    /** Additional trust/fear/respect/etc. if you want more detailed changes. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward")
    FSFFactionDelta DetailedDelta;
};

/**
 * Reward that modifies an identity axis.
 */
USTRUCT(BlueprintType)
struct FSFRewardIdentityDelta
{
    GENERATED_BODY()

    /** Axis to modify. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward")
    FGameplayTag AxisTag;

    /** Amount to add to the axis. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward")
    float Delta = 0.0f;
};

/**
 * Reward that directly applies a narrative outcome asset.
 */
USTRUCT(BlueprintType)
struct FSFRewardOutcomeApplication
{
    GENERATED_BODY()

    /** Outcome asset to apply when this reward is granted. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward")
    FPrimaryAssetId OutcomeAssetId;

    /** Optional context identifier (NPC, location, quest, etc.). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward")
    FName ContextId = NAME_None;
};

/**
 * Single atomic reward entry; a reward set is a list of these.
 */
USTRUCT(BlueprintType)
struct FSFQuestRewardEntry
{
    GENERATED_BODY()

    /** General scalar rewards like XP, currency, and tags. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward")
    FSFSimpleReward SimpleReward;

    /** Items (or item-like assets) to grant. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward")
    TArray<FSFRewardItemDefinition> Items;

    /** Faction standing changes. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward")
    TArray<FSFRewardFactionDelta> FactionDeltas;

    /** Identity axis changes. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward")
    TArray<FSFRewardIdentityDelta> IdentityDeltas;

    /** Narrative outcomes to apply. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward")
    TArray<FSFRewardOutcomeApplication> Outcomes;

    /** Optional label for tooling / quest design UI. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward")
    FString DebugLabel;

    /** Weight for this entire entry when selection mode uses random choice. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward", meta = (ClampMin = "0.0"))
    float EntryWeight = 1.0f;
};

/**
 * A grouped collection of reward entries that can be attached to:
 *  - a whole quest (success/failure),
 *  - a quest state,
 *  - an objective (FSFQuestObjectiveDefinition),
 *  - or a step (FSFQuestStepDefinition).
 *
 * Runtime systems decide *when* and *how* to grant these.
 */
USTRUCT(BlueprintType)
struct FSFQuestRewardSet
{
    GENERATED_BODY()

    /** How to interpret the Entries array when granting this set. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward")
    ESFQuestRewardSelectionMode SelectionMode = ESFQuestRewardSelectionMode::All;

    /** Individual entries that may be granted, depending on SelectionMode. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward")
    TArray<FSFQuestRewardEntry> Entries;

    /** Max number of entries to select when using RandomSubset. 0/negative = no explicit cap. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward", meta = (ClampMin = "0"))
    int32 MaxRandomlySelectedEntries = 0;

    /** Optional tags that categorize this reward set (e.g. “Success”, “Failure”, “Bonus”). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward")
    FGameplayTagContainer RewardTags;

    bool IsEmpty() const
    {
        return Entries.Num() == 0;
    }
};

/**
 * Optional data asset for reusable reward tables.
 * You can reference these from quests/objectives/steps via a soft pointer.
 */
UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFQuestRewardTable : public UDataAsset
{
    GENERATED_BODY()

public:
    /** Named reward sets you can reuse across the game. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Narrative|Reward")
    TMap<FName, FSFQuestRewardSet> RewardSets;
};
