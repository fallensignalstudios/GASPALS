#include "Core/SFGameStateBase.h"
#include "Narrative/SFNarrativeReplicatorComponent.h"
#include "Net/UnrealNetwork.h"

ASFGameStateBase::ASFGameStateBase()
{
	bReplicates = true;

	// GameState lives for the entire match — replicate at a modest rate
	// so world-phase changes and shared narrative deltas reach clients
	// promptly without hogging bandwidth.
	NetUpdateFrequency = 10.0f;
	MinNetUpdateFrequency = 2.0f;

	NarrativeReplicator = CreateDefaultSubobject<USFNarrativeReplicatorComponent>(TEXT("NarrativeReplicator"));
	NarrativeReplicator->SetIsReplicated(true);
}

void ASFGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASFGameStateBase, WorldPhase);

	// NarrativeReplicator replicates as a sub-object; its own UPROPERTYs
	// declare lifetime themselves. No entry needed here.
}

void ASFGameStateBase::BeginPlay()
{
	Super::BeginPlay();

	PreviousWorldPhase = WorldPhase;
}

void ASFGameStateBase::SetWorldPhase(FGameplayTag NewPhase)
{
	if (!HasAuthority())
	{
		return;
	}

	if (NewPhase == WorldPhase)
	{
		return;
	}

	const FGameplayTag OldPhase = WorldPhase;
	WorldPhase = NewPhase;
	PreviousWorldPhase = NewPhase;

	// Fire on server immediately; clients fire from OnRep_WorldPhase.
	OnWorldPhaseChanged.Broadcast(OldPhase, NewPhase);
}

void ASFGameStateBase::OnRep_WorldPhase(FGameplayTag OldPhase)
{
	OnWorldPhaseChanged.Broadcast(OldPhase, WorldPhase);
	PreviousWorldPhase = WorldPhase;
}

// Note: AGameStateBase::GetServerWorldTimeSeconds() is already exposed to
// both C++ and Blueprint and replicates a server-authoritative world time.
// We rely on the inherited implementation rather than shadowing it here.
