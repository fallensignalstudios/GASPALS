// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "SFNarrativeReplicatorComponent.h"

#include "Net/UnrealNetwork.h"
#include "SFNarrativeComponent.h"

USFNarrativeReplicatorComponent::USFNarrativeReplicatorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void USFNarrativeReplicatorComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!OwnerComponent)
    {
        OwnerComponent = GetOwner() ? GetOwner()->FindComponentByClass<USFNarrativeComponent>() : nullptr;
    }
}

void USFNarrativeReplicatorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(USFNarrativeReplicatorComponent, ReplicatedDeltas);
}

bool USFNarrativeReplicatorComponent::Initialize(USFNarrativeComponent* InOwnerComponent)
{
    OwnerComponent = InOwnerComponent;
    ReplicatedDeltas.Reset();
    LastAppliedSequence = 0;
    LastQueuedSequence = 0;
    LastAcknowledgedSequence = 0;
    bAwaitingFullResync = false;
    return OwnerComponent != nullptr;
}

int32 USFNarrativeReplicatorComponent::AllocateSequence()
{
    LastQueuedSequence = FMath::Max(LastQueuedSequence + 1, 1);
    return LastQueuedSequence;
}

bool USFNarrativeReplicatorComponent::PushAuthoritativeDelta(const FSFNarrativeDelta& Delta)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !CanAcceptDelta(Delta))
    {
        return false;
    }

    LastQueuedSequence = FMath::Max(LastQueuedSequence, Delta.Sequence);
    ReplicatedDeltas.Add(Delta);
    SortReplicatedDeltas();

    if (ReplicatedDeltas.Num() > MaxBufferedDeltas)
    {
        ReplicatedDeltas.RemoveAt(0, ReplicatedDeltas.Num() - MaxBufferedDeltas);
    }

    BroadcastSequenceAdvanced(Delta.Sequence);
    PruneAcknowledgedDeltas();
    return true;
}

bool USFNarrativeReplicatorComponent::ApplyReplicatedDelta(const FSFNarrativeDelta& Delta)
{
    if (!CanAcceptDelta(Delta))
    {
        return false;
    }

    if (Delta.Sequence <= LastAppliedSequence)
    {
        return false;
    }

    if (ShouldRequestResync(Delta))
    {
        bAwaitingFullResync = true;
        OnFullResyncRequested.Broadcast();
        return false;
    }

    LastAppliedSequence = Delta.Sequence;
    bAwaitingFullResync = false;
    OnDeltaApplied.Broadcast(Delta);
    BroadcastSequenceAdvanced(LastAppliedSequence);

    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        ServerAcknowledgeSequence(LastAppliedSequence);
    }

    return true;
}

void USFNarrativeReplicatorComponent::MarkClientCaughtUp(int32 Sequence)
{
    LastAcknowledgedSequence = FMath::Max(LastAcknowledgedSequence, Sequence);
    PruneAcknowledgedDeltas();
}

void USFNarrativeReplicatorComponent::ForceFullResync()
{
    bAwaitingFullResync = true;
    OnFullResyncRequested.Broadcast();
}

int32 USFNarrativeReplicatorComponent::GetLastAppliedSequence() const
{
    return LastAppliedSequence;
}

int32 USFNarrativeReplicatorComponent::GetLastQueuedSequence() const
{
    return LastQueuedSequence;
}

int32 USFNarrativeReplicatorComponent::GetLastAcknowledgedSequence() const
{
    return LastAcknowledgedSequence;
}

bool USFNarrativeReplicatorComponent::IsAwaitingResync() const
{
    return bAwaitingFullResync;
}

void USFNarrativeReplicatorComponent::OnRep_ReplicatedDeltas()
{
    SortReplicatedDeltas();

    for (const FSFNarrativeDelta& Delta : ReplicatedDeltas)
    {
        if (bAwaitingFullResync)
        {
            break;
        }

        ApplyReplicatedDelta(Delta);
    }
}

void USFNarrativeReplicatorComponent::ServerAcknowledgeSequence_Implementation(int32 Sequence)
{
    MarkClientCaughtUp(Sequence);
}

bool USFNarrativeReplicatorComponent::CanAcceptDelta(const FSFNarrativeDelta& Delta) const
{
    return Delta.Sequence > 0 && Delta.Type != ESFNarrativeDeltaType::None;
}

bool USFNarrativeReplicatorComponent::ShouldRequestResync(const FSFNarrativeDelta& Delta) const
{
    return LastAppliedSequence > 0 && Delta.Sequence > LastAppliedSequence + 1;
}

void USFNarrativeReplicatorComponent::PruneAcknowledgedDeltas()
{
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        ReplicatedDeltas.RemoveAll([this](const FSFNarrativeDelta& Delta)
            {
                return Delta.Sequence <= LastAcknowledgedSequence;
            });
    }
}

void USFNarrativeReplicatorComponent::SortReplicatedDeltas()
{
    ReplicatedDeltas.Sort([](const FSFNarrativeDelta& A, const FSFNarrativeDelta& B)
        {
            return A.Sequence < B.Sequence;
        });
}

void USFNarrativeReplicatorComponent::BroadcastSequenceAdvanced(int32 Sequence)
{
    OnSequenceAdvanced.Broadcast(Sequence);
}
