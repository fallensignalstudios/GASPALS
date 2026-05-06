#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SFNarrativeTypes.h"
#include "SFNarrativeReplicatorComponent.generated.h"

class USFNarrativeComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSFNarrativeDeltaEvent, const FSFNarrativeDelta&, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSFNarrativeSequenceEvent, int32, Sequence);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSFNarrativeResyncEvent);

UCLASS(ClassGroup = (Narrative), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class SIGNALFORGERPG_API USFNarrativeReplicatorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USFNarrativeReplicatorComponent();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Narrative|Replication")
    virtual bool Initialize(USFNarrativeComponent* InOwnerComponent);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Replication")
    virtual int32 AllocateSequence();

    UFUNCTION(BlueprintCallable, Category = "Narrative|Replication")
    virtual bool PushAuthoritativeDelta(const FSFNarrativeDelta& Delta);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Replication")
    virtual bool ApplyReplicatedDelta(const FSFNarrativeDelta& Delta);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Replication")
    virtual void MarkClientCaughtUp(int32 Sequence);

    UFUNCTION(BlueprintCallable, Category = "Narrative|Replication")
    virtual void ForceFullResync();

    UFUNCTION(BlueprintPure, Category = "Narrative|Replication")
    virtual int32 GetLastAppliedSequence() const;

    UFUNCTION(BlueprintPure, Category = "Narrative|Replication")
    virtual int32 GetLastQueuedSequence() const;

    UFUNCTION(BlueprintPure, Category = "Narrative|Replication")
    virtual int32 GetLastAcknowledgedSequence() const;

    UFUNCTION(BlueprintPure, Category = "Narrative|Replication")
    virtual bool IsAwaitingResync() const;

public:
    UPROPERTY(BlueprintAssignable, Category = "Narrative|Replication")
    FSFNarrativeDeltaEvent OnDeltaApplied;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Replication")
    FSFNarrativeSequenceEvent OnSequenceAdvanced;

    UPROPERTY(BlueprintAssignable, Category = "Narrative|Replication")
    FSFNarrativeResyncEvent OnFullResyncRequested;

protected:
    UFUNCTION()
    void OnRep_ReplicatedDeltas();

    UFUNCTION(Server, Reliable)
    void ServerAcknowledgeSequence(int32 Sequence);

    bool CanAcceptDelta(const FSFNarrativeDelta& Delta) const;
    bool ShouldRequestResync(const FSFNarrativeDelta& Delta) const;
    void PruneAcknowledgedDeltas();
    void SortReplicatedDeltas();
    void BroadcastSequenceAdvanced(int32 Sequence);

protected:
    UPROPERTY(Transient)
    TObjectPtr<USFNarrativeComponent> OwnerComponent = nullptr;

    UPROPERTY(ReplicatedUsing = OnRep_ReplicatedDeltas)
    TArray<FSFNarrativeDelta> ReplicatedDeltas;

    UPROPERTY(VisibleInstanceOnly, Category = "Narrative|Replication")
    int32 LastAppliedSequence = 0;

    UPROPERTY(VisibleInstanceOnly, Category = "Narrative|Replication")
    int32 LastQueuedSequence = 0;

    UPROPERTY(VisibleInstanceOnly, Category = "Narrative|Replication")
    int32 LastAcknowledgedSequence = 0;

    UPROPERTY(VisibleInstanceOnly, Category = "Narrative|Replication")
    bool bAwaitingFullResync = false;

    UPROPERTY(EditDefaultsOnly, Category = "Narrative|Replication", meta = (ClampMin = "1"))
    int32 MaxBufferedDeltas = 256;
};