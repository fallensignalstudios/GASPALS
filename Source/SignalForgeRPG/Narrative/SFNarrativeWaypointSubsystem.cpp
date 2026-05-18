// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "Narrative/SFNarrativeWaypointSubsystem.h"

#include "Core/SFPlayerState.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Narrative/SFNarrativeComponent.h"
#include "Narrative/SFNarrativeWaypoint.h"
#include "Narrative/SFQuestDefinition.h"
#include "Narrative/SFQuestInstance.h"
#include "Narrative/SFQuestLogWidgetController.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogSFWaypoints, Log, All);

// =============================================================================
// UWorldSubsystem lifecycle
// =============================================================================

bool USFNarrativeWaypointSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    // Avoid spinning up in editor preview / inactive worlds.
    if (UWorld* World = Cast<UWorld>(Outer))
    {
        const EWorldType::Type WT = World->WorldType;
        return WT == EWorldType::Game
            || WT == EWorldType::PIE
            || WT == EWorldType::GamePreview;
    }
    return false;
}

void USFNarrativeWaypointSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ActiveWaypoints.Reset();
    RegisteredActorsByKey.Reset();
}

void USFNarrativeWaypointSubsystem::Deinitialize()
{
    StopAutoResolveRetryTimer();
    UnbindFromNarrativeComponent();

    if (USFQuestLogWidgetController* Controller = BoundQuestLogController.Get())
    {
        Controller->OnTrackedQuestChanged.RemoveDynamic(this, &USFNarrativeWaypointSubsystem::HandleTrackedQuestChanged);
    }
    BoundQuestLogController = nullptr;

    RegisteredActorsByKey.Reset();
    ActiveWaypoints.Reset();
    Super::Deinitialize();
}

void USFNarrativeWaypointSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    TryBindToLocalPlayerNarrativeComponent();
}

// =============================================================================
// Local player narrative-component resolution
// =============================================================================

APlayerController* USFNarrativeWaypointSubsystem::GetLocalPlayerController() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }
    // First local PC. Single-player + listen-server hosts both land here.
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        if (APlayerController* PC = It->Get())
        {
            if (PC->IsLocalController())
            {
                return PC;
            }
        }
    }
    return nullptr;
}

void USFNarrativeWaypointSubsystem::TryBindToLocalPlayerNarrativeComponent()
{
    if (BoundNarrativeComponent.IsValid())
    {
        return;
    }

    APlayerController* PC = GetLocalPlayerController();
    APlayerState* PS = PC ? PC->PlayerState : nullptr;
    ASFPlayerState* SFPS = Cast<ASFPlayerState>(PS);
    USFNarrativeComponent* NC = SFPS ? SFPS->GetNarrativeComponent() : nullptr;
    if (!NC)
    {
        StartAutoResolveRetryTimer();
        return;
    }

    BindToNarrativeComponent(NC);
}

void USFNarrativeWaypointSubsystem::StartAutoResolveRetryTimer()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    if (World->GetTimerManager().IsTimerActive(AutoResolveRetryHandle))
    {
        return;
    }
    AutoResolveRetryElapsedSeconds = 0.0f;
    const float Period = FMath::Max(0.1f, AutoResolveRetryPeriodSeconds);
    World->GetTimerManager().SetTimer(
        AutoResolveRetryHandle,
        this,
        &USFNarrativeWaypointSubsystem::TickAutoResolveRetry,
        Period,
        /*bLoop=*/true);
}

void USFNarrativeWaypointSubsystem::StopAutoResolveRetryTimer()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(AutoResolveRetryHandle);
    }
    AutoResolveRetryElapsedSeconds = 0.0f;
}

void USFNarrativeWaypointSubsystem::TickAutoResolveRetry()
{
    if (BoundNarrativeComponent.IsValid())
    {
        StopAutoResolveRetryTimer();
        return;
    }
    TryBindToLocalPlayerNarrativeComponent();
    if (BoundNarrativeComponent.IsValid())
    {
        StopAutoResolveRetryTimer();
        return;
    }

    AutoResolveRetryElapsedSeconds += FMath::Max(0.1f, AutoResolveRetryPeriodSeconds);
    if (AutoResolveRetryElapsedSeconds >= MaxAutoResolveRetrySeconds)
    {
        UE_LOG(LogSFWaypoints, Warning,
            TEXT("[Waypoints] Gave up resolving local NarrativeComponent after %.1fs. ")
            TEXT("Waypoints will not be surfaced until something explicitly binds the subsystem."),
            AutoResolveRetryElapsedSeconds);
        StopAutoResolveRetryTimer();
    }
}

void USFNarrativeWaypointSubsystem::BindToNarrativeComponent(USFNarrativeComponent* NC)
{
    if (!NC || BoundNarrativeComponent.Get() == NC)
    {
        return;
    }
    UnbindFromNarrativeComponent();

    BoundNarrativeComponent = NC;
    NC->OnQuestStarted.AddDynamic(this, &USFNarrativeWaypointSubsystem::HandleQuestStarted);
    NC->OnQuestStateChanged.AddDynamic(this, &USFNarrativeWaypointSubsystem::HandleQuestStateChanged);
    NC->OnQuestTaskProgressed.AddDynamic(this, &USFNarrativeWaypointSubsystem::HandleQuestTaskProgressed);
    NC->OnQuestAbandoned.AddDynamic(this, &USFNarrativeWaypointSubsystem::HandleQuestAbandoned);

    UE_LOG(LogSFWaypoints, Log,
        TEXT("[Waypoints] Bound to NarrativeComponent %s. Rebuilding active waypoints."),
        *GetNameSafe(NC));

    RebuildActiveWaypoints();
}

void USFNarrativeWaypointSubsystem::UnbindFromNarrativeComponent()
{
    USFNarrativeComponent* NC = BoundNarrativeComponent.Get();
    if (!NC)
    {
        return;
    }
    NC->OnQuestStarted.RemoveDynamic(this, &USFNarrativeWaypointSubsystem::HandleQuestStarted);
    NC->OnQuestStateChanged.RemoveDynamic(this, &USFNarrativeWaypointSubsystem::HandleQuestStateChanged);
    NC->OnQuestTaskProgressed.RemoveDynamic(this, &USFNarrativeWaypointSubsystem::HandleQuestTaskProgressed);
    NC->OnQuestAbandoned.RemoveDynamic(this, &USFNarrativeWaypointSubsystem::HandleQuestAbandoned);
    BoundNarrativeComponent = nullptr;
}

// =============================================================================
// Quest log controller binding
// =============================================================================

void USFNarrativeWaypointSubsystem::AdoptQuestLogWidgetController(USFQuestLogWidgetController* Controller)
{
    if (BoundQuestLogController.Get() == Controller)
    {
        return;
    }
    if (USFQuestLogWidgetController* Old = BoundQuestLogController.Get())
    {
        Old->OnTrackedQuestChanged.RemoveDynamic(this, &USFNarrativeWaypointSubsystem::HandleTrackedQuestChanged);
    }
    BoundQuestLogController = Controller;
    if (Controller)
    {
        Controller->OnTrackedQuestChanged.AddDynamic(this, &USFNarrativeWaypointSubsystem::HandleTrackedQuestChanged);
        // Adopt the current selection immediately.
        SetTrackedQuestId(Controller->GetTrackedQuestId());
    }
}

void USFNarrativeWaypointSubsystem::HandleTrackedQuestChanged(FName InTrackedQuestId, bool /*bIsTracked*/)
{
    SetTrackedQuestId(InTrackedQuestId);
}

void USFNarrativeWaypointSubsystem::SetTrackedQuestId(FName QuestId)
{
    if (TrackedQuestId == QuestId)
    {
        return;
    }
    TrackedQuestId = QuestId;
    RebuildActiveWaypoints();
}

// =============================================================================
// Narrative-side handlers
// =============================================================================

void USFNarrativeWaypointSubsystem::HandleQuestStarted(USFQuestInstance* /*QuestInstance*/)
{
    RebuildActiveWaypoints();
}

void USFNarrativeWaypointSubsystem::HandleQuestStateChanged(USFQuestInstance* /*QuestInstance*/, FName /*StateId*/)
{
    RebuildActiveWaypoints();
}

void USFNarrativeWaypointSubsystem::HandleQuestTaskProgressed(USFQuestInstance* /*QuestInstance*/, FGameplayTag /*TaskTag*/, FName /*ContextId*/, int32 /*Quantity*/)
{
    RebuildActiveWaypoints();
}

void USFNarrativeWaypointSubsystem::HandleQuestAbandoned(USFQuestInstance* /*QuestInstance*/)
{
    RebuildActiveWaypoints();
}

// =============================================================================
// Actor registration
// =============================================================================

void USFNarrativeWaypointSubsystem::RegisterWaypointActor(ASFNarrativeWaypoint* Actor)
{
    if (!Actor)
    {
        return;
    }
    const FString Key = Actor->BuildRegistrationKey();
    if (Key.IsEmpty())
    {
        UE_LOG(LogSFWaypoints, Warning,
            TEXT("[Waypoints] Actor %s tried to register with an empty key. Set its QuestToTrack and TargetTaskId."),
            *Actor->GetName());
        return;
    }
    RegisteredActorsByKey.Add(Key, Actor);
    RebuildActiveWaypoints();
}

void USFNarrativeWaypointSubsystem::UnregisterWaypointActor(ASFNarrativeWaypoint* Actor)
{
    if (!Actor)
    {
        return;
    }
    const FString Key = Actor->BuildRegistrationKey();
    if (Key.IsEmpty())
    {
        // Defensive: scan the map for this actor in case its key got cleared.
        for (auto It = RegisteredActorsByKey.CreateIterator(); It; ++It)
        {
            if (It.Value().Get() == Actor)
            {
                It.RemoveCurrent();
            }
        }
    }
    else if (TWeakObjectPtr<ASFNarrativeWaypoint>* Existing = RegisteredActorsByKey.Find(Key))
    {
        if (Existing->Get() == Actor || !Existing->IsValid())
        {
            RegisteredActorsByKey.Remove(Key);
        }
    }
    RebuildActiveWaypoints();
}

// =============================================================================
// Authored coordinate availability
// =============================================================================

bool USFNarrativeWaypointSubsystem::IsAuthoredCoordinateAvailable(const FSFQuestTaskDefinition& TaskDef) const
{
    if (!TaskDef.bHasWaypoint)
    {
        return false;
    }
    // No level binding -> coordinate is valid in the persistent world.
    if (TaskDef.WaypointLevel.IsNull())
    {
        return true;
    }
    // Bound to a specific level: only surface when the streamed level is loaded.
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }
    const FString LevelPath = TaskDef.WaypointLevel.ToSoftObjectPath().GetLongPackageName();
    for (const ULevel* Level : World->GetLevels())
    {
        if (!Level || !Level->bIsVisible)
        {
            continue;
        }
        if (Level->GetOutermost() && Level->GetOutermost()->GetName() == LevelPath)
        {
            return true;
        }
    }
    return false;
}

// =============================================================================
// Snapshot building
// =============================================================================

bool USFNarrativeWaypointSubsystem::BuildSnapshotForTask(
    USFQuestInstance* Instance,
    const FSFQuestTaskDefinition& TaskDef,
    const FSFQuestObjectiveDisplayEntry* OptionalDisplay,
    FSFWaypointSnapshot& OutSnapshot) const
{
    if (!Instance || !Instance->GetDefinition())
    {
        return false;
    }
    const USFQuestDefinition* Def = Instance->GetDefinition();

    OutSnapshot = FSFWaypointSnapshot{};
    OutSnapshot.QuestId = Def->QuestId;
    OutSnapshot.QuestAssetId = Def->GetPrimaryAssetId();
    OutSnapshot.TaskId = TaskDef.TaskId;
    OutSnapshot.IconTag = TaskDef.WaypointIconTag;
    OutSnapshot.bRevealOnCompass = TaskDef.bRevealOnCompass;
    OutSnapshot.bIsTracked = (TrackedQuestId == Def->QuestId);

    // Display text fallback chain: authored override -> objective display text
    // -> task id -> quest display name (last resort).
    if (!TaskDef.WaypointDisplayText.IsEmpty())
    {
        OutSnapshot.DisplayText = TaskDef.WaypointDisplayText;
    }
    else if (OptionalDisplay && !OptionalDisplay->DisplayText.IsEmpty())
    {
        OutSnapshot.DisplayText = OptionalDisplay->DisplayText;
    }
    else if (!TaskDef.TaskId.IsNone())
    {
        OutSnapshot.DisplayText = FText::FromName(TaskDef.TaskId);
    }
    else
    {
        OutSnapshot.DisplayText = Def->DisplayName;
    }

    // Resolution order: registered actor first, then authored coordinate.
    const FString Key = FString::Printf(TEXT("%s|%s"),
        *Def->QuestId.ToString(), *TaskDef.TaskId.ToString());

    if (const TWeakObjectPtr<ASFNarrativeWaypoint>* RegisteredPtr = RegisteredActorsByKey.Find(Key))
    {
        if (ASFNarrativeWaypoint* RegisteredActor = RegisteredPtr->Get())
        {
            OutSnapshot.WorldLocation = RegisteredActor->GetActorLocation();
            OutSnapshot.Source = ESFWaypointSource::LevelActor;
            OutSnapshot.SourceActor = RegisteredActor;
            return true;
        }
    }

    if (IsAuthoredCoordinateAvailable(TaskDef))
    {
        OutSnapshot.WorldLocation = TaskDef.WaypointLocation;
        OutSnapshot.Source = ESFWaypointSource::AuthoredCoordinate;
        return true;
    }

    OutSnapshot.Source = ESFWaypointSource::Unresolved;
    return false;
}

// =============================================================================
// Rebuild
// =============================================================================

void USFNarrativeWaypointSubsystem::RefreshActiveWaypoints()
{
    RebuildActiveWaypoints();
}

void USFNarrativeWaypointSubsystem::RebuildActiveWaypoints()
{
    TArray<FSFWaypointSnapshot> OldList = ActiveWaypoints;
    FSFWaypointSnapshot OldTracked;
    GetTrackedWaypoint(OldTracked);

    ActiveWaypoints.Reset();

    USFNarrativeComponent* NC = BoundNarrativeComponent.Get();
    if (!NC)
    {
        DiffAndBroadcast(OldList, OldTracked);
        return;
    }

    TArray<USFQuestInstance*> Instances = NC->GetAllQuestInstances();
    for (USFQuestInstance* Instance : Instances)
    {
        if (!Instance)
        {
            continue;
        }
        if (Instance->GetCompletionState() != ESFQuestCompletionState::InProgress)
        {
            continue;
        }
        const USFQuestDefinition* Def = Instance->GetDefinition();
        if (!Def)
        {
            continue;
        }

        // Walk the current state's tasks; skip already-completed ones.
        const FName CurrentStateId = Instance->GetCurrentStateId();
        const FSFQuestStateDefinition* StateDef = nullptr;
        for (const FSFQuestStateDefinition& State : Def->States)
        {
            if (State.StateId == CurrentStateId)
            {
                StateDef = &State;
                break;
            }
        }
        if (!StateDef)
        {
            continue;
        }

        const TMap<FName, int32>& Progress = Instance->GetTaskProgressByTaskId();
        for (const FSFQuestTaskDefinition& TaskDef : StateDef->Tasks)
        {
            if (!TaskDef.bHasWaypoint && !RegisteredActorsByKey.Contains(
                    FString::Printf(TEXT("%s|%s"), *Def->QuestId.ToString(), *TaskDef.TaskId.ToString())))
            {
                continue;
            }
            // Skip completed tasks.
            const int32 CurrentProgress = Progress.Contains(TaskDef.TaskId) ? Progress[TaskDef.TaskId] : 0;
            if (CurrentProgress >= FMath::Max(1, TaskDef.RequiredQuantity))
            {
                continue;
            }

            FSFWaypointSnapshot Snap;
            if (BuildSnapshotForTask(Instance, TaskDef, /*OptionalDisplay=*/nullptr, Snap))
            {
                ActiveWaypoints.Add(MoveTemp(Snap));
            }
        }
    }

    // Tracked waypoints float to the top.
    ActiveWaypoints.StableSort([](const FSFWaypointSnapshot& A, const FSFWaypointSnapshot& B)
    {
        if (A.bIsTracked != B.bIsTracked)
        {
            return A.bIsTracked;
        }
        return A.QuestId.Compare(B.QuestId) < 0;
    });

    DiffAndBroadcast(OldList, OldTracked);
}

// =============================================================================
// Tracked waypoint accessor + broadcasts
// =============================================================================

bool USFNarrativeWaypointSubsystem::GetTrackedWaypoint(FSFWaypointSnapshot& OutWaypoint) const
{
    // 1. Prefer a waypoint whose quest matches the tracked quest id.
    if (!TrackedQuestId.IsNone())
    {
        for (const FSFWaypointSnapshot& Snap : ActiveWaypoints)
        {
            if (Snap.QuestId == TrackedQuestId && Snap.IsValidSnapshot())
            {
                OutWaypoint = Snap;
                return true;
            }
        }
    }
    // 2. Otherwise, fall back to the first valid active waypoint.
    for (const FSFWaypointSnapshot& Snap : ActiveWaypoints)
    {
        if (Snap.IsValidSnapshot())
        {
            OutWaypoint = Snap;
            return true;
        }
    }
    OutWaypoint = FSFWaypointSnapshot{};
    return false;
}

void USFNarrativeWaypointSubsystem::DiffAndBroadcast(const TArray<FSFWaypointSnapshot>& OldList, const FSFWaypointSnapshot& OldTracked)
{
    auto SnapshotEquals = [](const FSFWaypointSnapshot& A, const FSFWaypointSnapshot& B) -> bool
    {
        return A.MakeKey() == B.MakeKey()
            && A.WorldLocation.Equals(B.WorldLocation, 1.0f)
            && A.bIsTracked == B.bIsTracked
            && A.Source == B.Source;
    };

    bool bChanged = OldList.Num() != ActiveWaypoints.Num();
    if (!bChanged)
    {
        for (int32 i = 0; i < ActiveWaypoints.Num(); ++i)
        {
            if (!SnapshotEquals(ActiveWaypoints[i], OldList[i]))
            {
                bChanged = true;
                break;
            }
        }
    }

    if (bChanged)
    {
        OnWaypointsChanged.Broadcast();
    }

    FSFWaypointSnapshot NewTracked;
    GetTrackedWaypoint(NewTracked);
    if (!SnapshotEquals(NewTracked, OldTracked))
    {
        OnTrackedWaypointChanged.Broadcast(NewTracked);
    }
}
