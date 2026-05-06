#include "SFDialogueNarrativeMap.h"

void USFDialogueNarrativeMap::GetEntriesForNode(
    FName ConversationId,
    FName NodeId,
    TArray<FSFDialogueNarrativeEntry>& OutEntries) const
{
    OutEntries.Reset();

    for (const FSFDialogueNarrativeEntry& Entry : Entries)
    {
        if (Entry.Key.ConversationId == ConversationId &&
            Entry.Key.NodeId == NodeId)
        {
            OutEntries.Add(Entry);
        }
    }
}

bool USFDialogueNarrativeMap::GetEntry(
    const FSFDialogueKey& Key,
    FSFDialogueNarrativeEntry& OutEntry) const
{
    for (const FSFDialogueNarrativeEntry& Entry : Entries)
    {
        if (Entry.Key == Key)
        {
            OutEntry = Entry;
            return true;
        }
    }

    return false;
}

void USFDialogueNarrativeMap::GetEntriesWithTag(
    FGameplayTag Tag,
    TArray<FSFDialogueNarrativeEntry>& OutEntries) const
{
    OutEntries.Reset();

    if (!Tag.IsValid())
    {
        return;
    }

    for (const FSFDialogueNarrativeEntry& Entry : Entries)
    {
        if (Entry.NarrativeTags.HasTag(Tag))
        {
            OutEntries.Add(Entry);
        }
    }
}