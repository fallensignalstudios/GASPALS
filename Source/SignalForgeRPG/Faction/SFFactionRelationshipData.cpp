#include "Faction/SFFactionRelationshipData.h"

bool USFFactionRelationshipData::FindEntry(FGameplayTag SourceFaction, FSFFactionRelationshipEntry& OutEntry) const
{
	if (!SourceFaction.IsValid())
	{
		return false;
	}

	for (const FSFFactionRelationshipEntry& Entry : Entries)
	{
		if (Entry.SourceFaction.MatchesTagExact(SourceFaction))
		{
			OutEntry = Entry;
			return true;
		}
	}

	return false;
}

ESFFactionAttitude USFFactionRelationshipData::GetAttitude(FGameplayTag SourceFaction, FGameplayTag TargetFaction) const
{
	// Same-faction case: shared self-hostility.
	if (SourceFaction.IsValid() && TargetFaction.IsValid() && SourceFaction.MatchesTagExact(TargetFaction))
	{
		return SelfHostility;
	}

	FSFFactionRelationshipEntry Entry;
	if (FindEntry(SourceFaction, Entry))
	{
		for (const FSFFactionAttitudeRow& Row : Entry.AttitudesTowardOthers)
		{
			if (Row.TargetFaction.MatchesTagExact(TargetFaction))
			{
				return Row.Attitude;
			}
		}

		return Entry.DefaultAttitude;
	}

	return GlobalDefault;
}
