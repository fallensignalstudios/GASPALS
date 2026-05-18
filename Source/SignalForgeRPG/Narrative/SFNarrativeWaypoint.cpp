// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "Narrative/SFNarrativeWaypoint.h"

#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Narrative/SFNarrativeWaypointSubsystem.h"
#include "Narrative/SFQuestDefinition.h"

DEFINE_LOG_CATEGORY_STATIC(LogSFWaypoint, Log, All);

ASFNarrativeWaypoint::ASFNarrativeWaypoint()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

#if WITH_EDITORONLY_DATA
    EditorSprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("EditorSprite"));
    if (EditorSprite)
    {
        EditorSprite->bIsEditorOnly = true;
        EditorSprite->SetupAttachment(Root);
        EditorSprite->bHiddenInGame = true;
    }
#endif
}

FString ASFNarrativeWaypoint::BuildRegistrationKey() const
{
    if (QuestToTrack.IsNull() || TargetTaskId.IsNone())
    {
        return FString();
    }

    // Resolve the quest definition so we can extract its stable QuestId. The
    // soft pointer is loaded synchronously because waypoints are infrequent
    // and the cost is dwarfed by the quest's own runtime footprint.
    const USFQuestDefinition* Def = QuestToTrack.LoadSynchronous();
    if (!Def || Def->QuestId.IsNone())
    {
        return FString();
    }

    return FString::Printf(TEXT("%s|%s"),
        *Def->QuestId.ToString(), *TargetTaskId.ToString());
}

void ASFNarrativeWaypoint::BeginPlay()
{
    Super::BeginPlay();
    RefreshSubsystemRegistration();
}

void ASFNarrativeWaypoint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (bRegisteredWithSubsystem)
    {
        if (UWorld* World = GetWorld())
        {
            if (USFNarrativeWaypointSubsystem* Subsystem = World->GetSubsystem<USFNarrativeWaypointSubsystem>())
            {
                Subsystem->UnregisterWaypointActor(this);
            }
        }
        bRegisteredWithSubsystem = false;
    }
    Super::EndPlay(EndPlayReason);
}

void ASFNarrativeWaypoint::RefreshSubsystemRegistration()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    USFNarrativeWaypointSubsystem* Subsystem = World->GetSubsystem<USFNarrativeWaypointSubsystem>();
    if (!Subsystem)
    {
        return;
    }

    // De-register first so a re-register picks up the new key cleanly.
    if (bRegisteredWithSubsystem)
    {
        Subsystem->UnregisterWaypointActor(this);
        bRegisteredWithSubsystem = false;
    }

    const FString Key = BuildRegistrationKey();
    if (Key.IsEmpty())
    {
        UE_LOG(LogSFWaypoint, Warning,
            TEXT("%s: cannot register waypoint. QuestToTrack='%s', TargetTaskId='%s'."),
            *GetName(), *QuestToTrack.ToString(), *TargetTaskId.ToString());
        return;
    }

    Subsystem->RegisterWaypointActor(this);
    bRegisteredWithSubsystem = true;
    BP_OnBecameActive();
}

#if WITH_EDITOR
void ASFNarrativeWaypoint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const FName ChangedName = PropertyChangedEvent.MemberProperty
        ? PropertyChangedEvent.MemberProperty->GetFName()
        : NAME_None;

    static const FName QuestToTrackName(GET_MEMBER_NAME_CHECKED(ASFNarrativeWaypoint, QuestToTrack));
    static const FName TargetTaskIdName(GET_MEMBER_NAME_CHECKED(ASFNarrativeWaypoint, TargetTaskId));

    if (ChangedName == QuestToTrackName || ChangedName == TargetTaskIdName)
    {
        // World-context check: only re-register when the level is actually playing.
        if (GetWorld() && GetWorld()->HasBegunPlay())
        {
            RefreshSubsystemRegistration();
        }
    }
}
#endif
