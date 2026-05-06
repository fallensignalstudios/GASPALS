#include "Characters/SFNPCNarrativeIdentityComponent.h"
#include "Net/UnrealNetwork.h"

USFNPCNarrativeIdentityComponent::USFNPCNarrativeIdentityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
	SetIsReplicatedByDefault(true);
}

void USFNPCNarrativeIdentityComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USFNPCNarrativeIdentityComponent, Disposition);
	// FactionTag, NarrativeTags, NPCContextId are author-time data — server
	// is authoritative at spawn but they should not change at runtime. If
	// you need runtime changes (e.g. a defection), promote them to
	// Replicated and add DOREPLIFETIME entries here.
}

void USFNPCNarrativeIdentityComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if (NPCContextId.IsNone())
	{
		if (const AActor* Owner = GetOwner())
		{
			// Default to actor's FName so designers don't have to fill it in
			// for every placed NPC; world facts will still scope per-instance.
			NPCContextId = Owner->GetFName();
		}
	}

	if (bIsHostileByDefault && Disposition == ESFNPCDisposition::Neutral)
	{
		Disposition = ESFNPCDisposition::Hostile;
	}

	PreviousDisposition = Disposition;
}

void USFNPCNarrativeIdentityComponent::SetDisposition(ESFNPCDisposition NewDisposition)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (NewDisposition == Disposition)
	{
		return;
	}

	const ESFNPCDisposition OldDisposition = Disposition;
	Disposition = NewDisposition;
	PreviousDisposition = NewDisposition;

	// Fire on server immediately; clients fire from OnRep_Disposition.
	OnDispositionChanged.Broadcast(OldDisposition, NewDisposition);
}

void USFNPCNarrativeIdentityComponent::OnRep_Disposition(ESFNPCDisposition OldDisposition)
{
	OnDispositionChanged.Broadcast(OldDisposition, Disposition);
	PreviousDisposition = Disposition;
}
