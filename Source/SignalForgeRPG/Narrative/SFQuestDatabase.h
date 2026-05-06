// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "SFNarrativeTypes.h"
#include "SFQuestDatabase.generated.h"

class USFQuestDefinition;

/**
 * Central registry for all quest definitions, with lookup helpers by
 * asset id, tag, and category.
 *
 * This is mostly read-only at runtime and is intended for use by
 * USFQuestRuntime, menus, codex, and tooling.
 */
UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFQuestDatabase : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    /** All quest definition assets known to the game. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quests")
    TArray<TSoftObjectPtr<USFQuestDefinition>> AllQuests;

    /** Optional tag that marks “main storyline” quests. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quests|Tags")
    FGameplayTag MainQuestTag;

    /** Optional tag that marks “side quests”. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quests|Tags")
    FGameplayTag SideQuestTag;

    /** Optional tag that marks “repeatable” content. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quests|Tags")
    FGameplayTag RepeatableQuestTag;

    //
    // Lookup helpers
    //

    /** Resolve a quest by primary asset id. Returns nullptr if not found / not loaded. */
    UFUNCTION(BlueprintPure, Category = "Quests")
    USFQuestDefinition* GetQuestById(const FPrimaryAssetId& QuestId) const;

    /** Resolve a quest by soft path or name (convenience). */
    UFUNCTION(BlueprintPure, Category = "Quests")
    USFQuestDefinition* GetQuestByName(const FName QuestName) const;

    /** Get all quests that have the given gameplay tag on their definition. */
    UFUNCTION(BlueprintPure, Category = "Quests")
    void GetQuestsWithTag(FGameplayTag Tag, TArray<USFQuestDefinition*>& OutQuests) const;

    /** Get all quests that match ANY of the given tags. */
    UFUNCTION(BlueprintPure, Category = "Quests")
    void GetQuestsWithAnyTags(const FGameplayTagContainer& Tags, TArray<USFQuestDefinition*>& OutQuests) const;

    /** Get all quests that match ALL of the given tags. */
    UFUNCTION(BlueprintPure, Category = "Quests")
    void GetQuestsWithAllTags(const FGameplayTagContainer& Tags, TArray<USFQuestDefinition*>& OutQuests) const;

    /** Convenience: return all quests classified as “main”. */
    UFUNCTION(BlueprintPure, Category = "Quests")
    void GetMainStoryQuests(TArray<USFQuestDefinition*>& OutQuests) const;

    /** Convenience: return all quests classified as “side”. */
    UFUNCTION(BlueprintPure, Category = "Quests")
    void GetSideQuests(TArray<USFQuestDefinition*>& OutQuests) const;

    /** Convenience: return all quests marked as repeatable. */
    UFUNCTION(BlueprintPure, Category = "Quests")
    void GetRepeatableQuests(TArray<USFQuestDefinition*>& OutQuests) const;
};
