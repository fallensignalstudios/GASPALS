#include "Faction/SFFactionStatics.h"

#include "Characters/SFCharacterBase.h"
#include "Characters/SFNPCNarrativeIdentityComponent.h"
#include "Core/SignalForgeDeveloperSettings.h"
#include "Faction/SFFactionComponent.h"
#include "Faction/SFFactionRelationshipData.h"

// NOTE: Dynamic per-player standing-band overrides are intentionally not
// wired here yet. The narrative replicator currently exposes its faction
// standings on USFNarrativeComponent (not on PlayerState directly), and
// pulling that into combat queries every hit deserves a dedicated cache.
//
// Hook point: extend GetEffectiveAttitudeBetweenActors to query a
// USFNarrativeComponent on the source's PlayerState and override the
// static attitude when the standing band is Hostile or Allied.
// (See SFNarrativeConstants.h::StandingBandHostileMax / Allied for the
// thresholds.)

bool USFFactionStatics::AreHostile(const AActor* FromActor, const AActor* ToActor)
{
	return GetEffectiveAttitudeBetweenActors(FromActor, ToActor) == ESFFactionAttitude::Hostile;
}

ESFFactionAttitude USFFactionStatics::GetStaticAttitudeBetweenFactions(FGameplayTag SourceFaction, FGameplayTag TargetFaction)
{
	if (const USFFactionRelationshipData* Data = GetRelationshipData())
	{
		return Data->GetAttitude(SourceFaction, TargetFaction);
	}

	// No asset configured: treat same tag as friendly, everything else neutral.
	if (SourceFaction.IsValid() && TargetFaction.IsValid() && SourceFaction.MatchesTagExact(TargetFaction))
	{
		return ESFFactionAttitude::Friendly;
	}
	return ESFFactionAttitude::Neutral;
}

ESFFactionAttitude USFFactionStatics::GetEffectiveAttitudeBetweenActors(const AActor* FromActor, const AActor* ToActor)
{
	if (!FromActor || !ToActor || FromActor == ToActor)
	{
		// Self-targeting -> friendly (combat gate will reject it as a separate self-damage check).
		return ESFFactionAttitude::Friendly;
	}

	const FGameplayTag FromFaction = GetFactionTag(FromActor);
	const FGameplayTag ToFaction = GetFactionTag(ToActor);

	// TODO: dynamic standing-band override (see header comment above).
	return GetStaticAttitudeBetweenFactions(FromFaction, ToFaction);
}

FGameplayTag USFFactionStatics::GetFactionTag(const AActor* Actor)
{
	if (!Actor)
	{
		return FGameplayTag();
	}

	if (const USFFactionComponent* Faction = Actor->FindComponentByClass<USFFactionComponent>())
	{
		const FGameplayTag Tag = Faction->GetFactionTag();
		if (Tag.IsValid())
		{
			return Tag;
		}
	}

	if (const USFNPCNarrativeIdentityComponent* Identity = Actor->FindComponentByClass<USFNPCNarrativeIdentityComponent>())
	{
		return Identity->GetFactionTag();
	}

	return FGameplayTag();
}

FGenericTeamId USFFactionStatics::TeamIdFromFactionTag(FGameplayTag FactionTag)
{
	if (!FactionTag.IsValid())
	{
		return FGenericTeamId::NoTeam;
	}

	// Stable hash of the tag name into the 0..254 team-id space. 255 is reserved (NoTeam).
	const uint32 Hash = GetTypeHash(FactionTag.GetTagName());
	const uint8 TeamIndex = static_cast<uint8>(Hash % 255u);
	return FGenericTeamId(TeamIndex);
}

const USFFactionRelationshipData* USFFactionStatics::GetRelationshipData()
{
	const USignalForgeDeveloperSettings* Settings = GetDefault<USignalForgeDeveloperSettings>();
	if (!Settings)
	{
		return nullptr;
	}
	return Settings->GetDefaultFactionRelationships();
}
