// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "SFQuestDatabase.h"
#include "SFQuestDefinition.h"

USFQuestDefinition* USFQuestDatabase::GetQuestById(const FPrimaryAssetId& QuestId) const
{
    if (!QuestId.IsValid())
    {
        return nullptr;
    }

    for (const TSoftObjectPtr<USFQuestDefinition>& SoftQuest : AllQuests)
    {
        if (!SoftQuest.IsValid())
        {
            // Compare by asset id using the soft path if not loaded yet.
            const FSoftObjectPath& Path = SoftQuest.ToSoftObjectPath();
            if (Path.GetAssetPathName() == QuestId.PrimaryAssetName)
            {
                return SoftQuest.LoadSynchronous();
            }
        }
        else
        {
            if (SoftQuest.Get()->GetPrimaryAssetId() == QuestId)
            {
                return SoftQuest.Get();
            }
        }
    }

    return nullptr;
}

USFQuestDefinition* USFQuestDatabase::GetQuestByName(const FName QuestName) const
{
    if (QuestName.IsNone())
    {
        return nullptr;
    }

    for (const TSoftObjectPtr<USFQuestDefinition>& SoftQuest : AllQuests)
    {
        if (!SoftQuest.IsValid())
        {
            const FSoftObjectPath& Path = SoftQuest.ToSoftObjectPath();
            if (Path.GetAssetPathName() == QuestName)
            {
                return SoftQuest.LoadSynchronous();
            }
        }
        else
        {
            if (SoftQuest.Get()->GetFName() == QuestName)
            {
                return SoftQuest.Get();
            }
        }
    }

    return nullptr;
}

void USFQuestDatabase::GetQuestsWithTag(
    FGameplayTag Tag,
    TArray<USFQuestDefinition*>& OutQuests) const
{
    OutQuests.Reset();

    if (!Tag.IsValid())
    {
        return;
    }

    for (const TSoftObjectPtr<USFQuestDefinition>& SoftQuest : AllQuests)
    {
        USFQuestDefinition* Quest = SoftQuest.LoadSynchronous();
        if (!Quest)
        {
            continue;
        }

        if (Quest->QuestTags.HasTag(Tag))
        {
            OutQuests.Add(Quest);
        }
    }
}

void USFQuestDatabase::GetQuestsWithAnyTags(
    const FGameplayTagContainer& Tags,
    TArray<USFQuestDefinition*>& OutQuests) const
{
    OutQuests.Reset();

    if (Tags.Num() == 0)
    {
        return;
    }

    for (const TSoftObjectPtr<USFQuestDefinition>& SoftQuest : AllQuests)
    {
        USFQuestDefinition* Quest = SoftQuest.LoadSynchronous();
        if (!Quest)
        {
            continue;
        }

        if (Quest->QuestTags.HasAny(Tags))
        {
            OutQuests.Add(Quest);
        }
    }
}

void USFQuestDatabase::GetQuestsWithAllTags(
    const FGameplayTagContainer& Tags,
    TArray<USFQuestDefinition*>& OutQuests) const
{
    OutQuests.Reset();

    if (Tags.Num() == 0)
    {
        return;
    }

    for (const TSoftObjectPtr<USFQuestDefinition>& SoftQuest : AllQuests)
    {
        USFQuestDefinition* Quest = SoftQuest.LoadSynchronous();
        if (!Quest)
        {
            continue;
        }

        if (Quest->QuestTags.HasAll(Tags))
        {
            OutQuests.Add(Quest);
        }
    }
}

void USFQuestDatabase::GetMainStoryQuests(
    TArray<USFQuestDefinition*>& OutQuests) const
{
    if (!MainQuestTag.IsValid())
    {
        OutQuests.Reset();
        return;
    }

    GetQuestsWithTag(MainQuestTag, OutQuests);
}

void USFQuestDatabase::GetSideQuests(
    TArray<USFQuestDefinition*>& OutQuests) const
{
    if (!SideQuestTag.IsValid())
    {
        OutQuests.Reset();
        return;
    }

    GetQuestsWithTag(SideQuestTag, OutQuests);
}

void USFQuestDatabase::GetRepeatableQuests(
    TArray<USFQuestDefinition*>& OutQuests) const
{
    if (!RepeatableQuestTag.IsValid())
    {
        OutQuests.Reset();
        return;
    }

    GetQuestsWithTag(RepeatableQuestTag, OutQuests);
}
