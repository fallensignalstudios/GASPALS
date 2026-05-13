#include "Faction/SFFactionComponent.h"

#include "Characters/SFNPCNarrativeIdentityComponent.h"
#include "Faction/SFFactionStatics.h"
#include "Net/UnrealNetwork.h"

USFFactionComponent::USFFactionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void USFFactionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USFFactionComponent, FactionTag);
}

void USFFactionComponent::BeginPlay()
{
	Super::BeginPlay();

	// On the server, optionally mirror from an existing NPC identity component
	// so we don't double-author the same FactionTag in two places.
	if (GetOwnerRole() == ROLE_Authority && bMirrorFromNPCIdentity)
	{
		if (AActor* Owner = GetOwner())
		{
			if (USFNPCNarrativeIdentityComponent* Identity = Owner->FindComponentByClass<USFNPCNarrativeIdentityComponent>())
			{
				const FGameplayTag IdentityTag = Identity->GetFactionTag();
				if (IdentityTag.IsValid() && !FactionTag.MatchesTagExact(IdentityTag))
				{
					SetFactionTag(IdentityTag);
				}
			}
		}
	}
}

void USFFactionComponent::SetFactionTag(FGameplayTag NewFaction)
{
	if (FactionTag.MatchesTagExact(NewFaction))
	{
		return;
	}

	const FGameplayTag OldFaction = FactionTag;
	FactionTag = NewFaction;

	// Server immediate broadcast; client gets OnRep_FactionTag.
	OnFactionTagChanged.Broadcast(OldFaction, FactionTag);
}

void USFFactionComponent::OnRep_FactionTag(FGameplayTag OldFaction)
{
	OnFactionTagChanged.Broadcast(OldFaction, FactionTag);
}

FGenericTeamId USFFactionComponent::GetTeamId() const
{
	return USFFactionStatics::TeamIdFromFactionTag(FactionTag);
}

ETeamAttitude::Type USFFactionComponent::GetAttitudeTowardOther(const AActor& Other) const
{
	const ESFFactionAttitude Attitude =
		USFFactionStatics::GetEffectiveAttitudeBetweenActors(GetOwner(), &Other);

	switch (Attitude)
	{
	case ESFFactionAttitude::Friendly:	return ETeamAttitude::Friendly;
	case ESFFactionAttitude::Hostile:	return ETeamAttitude::Hostile;
	case ESFFactionAttitude::Neutral:
	case ESFFactionAttitude::Unknown:
	default:							return ETeamAttitude::Neutral;
	}
}
