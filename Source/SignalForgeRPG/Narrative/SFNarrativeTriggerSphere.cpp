// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "Narrative/SFNarrativeTriggerSphere.h"

#include "Narrative/SFNarrativeComponent.h"
#include "Narrative/SFQuestDefinition.h"
#include "Characters/SFPlayerCharacter.h"
#include "Dialogue/Data/SFConversationDataAsset.h"
#include "Components/SphereComponent.h"
#include "Engine/AssetManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogSFNarrativeTrigger, Log, All);

namespace
{
    // We persist "this trigger fired" by writing a world fact whose context
    // is the TriggerId. The fact tag itself is constant; the context FName
    // disambiguates between triggers. Lowercase dotted style to stay
    // consistent with the existing Narrative.* tag tree.
    static const FName GFiredFactTagName = TEXT("Narrative.Trigger.Fired");
}

// =============================================================================
// Construction
// =============================================================================

ASFNarrativeTriggerSphere::ASFNarrativeTriggerSphere()
{
    PrimaryActorTick.bCanEverTick = false;

    TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
    SetRootComponent(TriggerSphere);

    TriggerSphere->InitSphereRadius(SphereRadius);
    TriggerSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TriggerSphere->SetCollisionObjectType(ECC_WorldDynamic);

    // Block nothing; overlap pawns only. NPCs share the Pawn channel so we
    // can't filter purely by channel -- we filter by cast in the handler.
    TriggerSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    TriggerSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    TriggerSphere->SetGenerateOverlapEvents(true);

#if WITH_EDITORONLY_DATA
    // Show the wireframe in the editor without committing to a separate
    // billboard component. SphereComponent already draws its bounds.
    TriggerSphere->ShapeColor = FColor(64, 192, 255);
#endif
}

void ASFNarrativeTriggerSphere::BeginPlay()
{
    Super::BeginPlay();

    if (TriggerSphere)
    {
        // Re-apply the authored radius in case it changed since CDO construction
        // (designers commonly tweak this in details).
        TriggerSphere->SetSphereRadius(SphereRadius, /*bUpdateOverlaps=*/true);
        TriggerSphere->OnComponentBeginOverlap.AddUniqueDynamic(
            this, &ASFNarrativeTriggerSphere::HandleSphereBeginOverlap);
    }

    if (TriggerId.IsNone())
    {
        UE_LOG(LogSFNarrativeTrigger, Warning,
            TEXT("%s has no TriggerId set; firing will not persist across save/load."), *GetName());
    }
}

// =============================================================================
// Overlap entry point
// =============================================================================

void ASFNarrativeTriggerSphere::HandleSphereBeginOverlap(
    UPrimitiveComponent* /*OverlappedComponent*/,
    AActor* OtherActor,
    UPrimitiveComponent* /*OtherComp*/,
    int32 /*OtherBodyIndex*/,
    bool /*bFromSweep*/,
    const FHitResult& /*SweepResult*/)
{
    // Player-only gate. Companions / NPCs / projectiles bounce off.
    ASFPlayerCharacter* Player = Cast<ASFPlayerCharacter>(OtherActor);
    if (!Player)
    {
        return;
    }

    BP_OnPlayerEntered(Player);

    if (bHasFiredThisSession)
    {
        return;
    }

    USFNarrativeComponent* Narrative = Player->GetNarrativeComponent();
    if (!Narrative)
    {
        UE_LOG(LogSFNarrativeTrigger, Warning,
            TEXT("%s overlapped by player %s but player has no narrative component; skipping."),
            *GetName(), *Player->GetName());
        return;
    }

    // Check persisted state. A previous play session may have fired this
    // trigger and saved + reloaded into the same map; we honor that.
    if (QueryPersistedFiredState(Narrative))
    {
        bHasFiredThisSession = true;
        DisableTriggerCollision();
        return;
    }

    if (ApplyActionsTo(Player, Narrative))
    {
        bHasFiredThisSession = true;
        StampPersistedFiredState(Narrative);
        DisableTriggerCollision();
        BP_OnTriggerFired(Player);
    }
}

// =============================================================================
// Manual fire / reset
// =============================================================================

bool ASFNarrativeTriggerSphere::ForceFire(AActor* TargetActor)
{
    if (bHasFiredThisSession)
    {
        return false;
    }

    USFNarrativeComponent* Narrative = TargetActor
        ? TargetActor->FindComponentByClass<USFNarrativeComponent>()
        : nullptr;
    if (!Narrative)
    {
        UE_LOG(LogSFNarrativeTrigger, Warning,
            TEXT("%s ForceFire failed: no narrative component on %s."),
            *GetName(), TargetActor ? *TargetActor->GetName() : TEXT("<null>"));
        return false;
    }

    if (QueryPersistedFiredState(Narrative))
    {
        bHasFiredThisSession = true;
        DisableTriggerCollision();
        return false;
    }

    if (ApplyActionsTo(TargetActor, Narrative))
    {
        bHasFiredThisSession = true;
        StampPersistedFiredState(Narrative);
        DisableTriggerCollision();
        BP_OnTriggerFired(TargetActor);
        return true;
    }
    return false;
}

void ASFNarrativeTriggerSphere::ResetFiredState(AActor* TargetActor)
{
    bHasFiredThisSession = false;

    if (TriggerSphere)
    {
        TriggerSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        TriggerSphere->SetGenerateOverlapEvents(true);
    }

    if (!TargetActor || TriggerId.IsNone())
    {
        return;
    }
    USFNarrativeComponent* Narrative = TargetActor->FindComponentByClass<USFNarrativeComponent>();
    if (!Narrative)
    {
        return;
    }

    FSFWorldFactKey Key;
    if (!BuildFiredFactKey(Key))
    {
        return;
    }

    // We don't currently have a RemoveWorldFact API; the cheapest reliable
    // reset is to overwrite the fact with a value that explicitly means
    // "not fired". Downstream queries can use HasWorldFact + BoolValue.
    Narrative->SetWorldFact(Key, FSFWorldFactValue::MakeBool(false));
}

// =============================================================================
// Action application
// =============================================================================

bool ASFNarrativeTriggerSphere::ApplyActionsTo(AActor* TargetActor, USFNarrativeComponent* Narrative)
{
    if (!Narrative)
    {
        return false;
    }

    bool bDidAnything = false;

    // 1. World facts first so quest/dialogue eligibility can see them.
    for (const FSFNarrativeTriggerFactWrite& Write : FactsToWriteOnFire)
    {
        if (!Write.Key.FactTag.IsValid())
        {
            continue;
        }
        if (Narrative->SetWorldFact(Write.Key, Write.Value))
        {
            bDidAnything = true;
        }
    }

    // 2. Quest start. Resolve the soft pointer to a primary asset id; the
    // narrative component is the single source of truth for the start state.
    if (!QuestToStart.IsNull())
    {
        // Sync-load is acceptable here: trigger fires are infrequent and the
        // quest payload is small. Async would buy us nothing and complicate
        // the "did we actually start?" return path.
        USFQuestDefinition* QuestDef = Cast<USFQuestDefinition>(QuestToStart.LoadSynchronous());
        if (QuestDef)
        {
            const FPrimaryAssetId QuestId = QuestDef->GetPrimaryAssetId();
            if (QuestId.IsValid())
            {
                if (Narrative->StartQuestByAssetId(QuestId, QuestStartStateId))
                {
                    bDidAnything = true;
                }
            }
            else
            {
                UE_LOG(LogSFNarrativeTrigger, Warning,
                    TEXT("%s: quest asset has no PrimaryAssetId; cannot start."), *GetName());
            }
        }
    }

    // 3. Conversation start. The narrative component's BeginConversation
    // requires a bound DialogueComponent on the player.
    if (!ConversationToStart.IsNull())
    {
        USFConversationDataAsset* Convo = Cast<USFConversationDataAsset>(ConversationToStart.LoadSynchronous());
        if (Convo)
        {
            if (Narrative->BeginConversation(Convo, this))
            {
                bDidAnything = true;
            }
            else
            {
                UE_LOG(LogSFNarrativeTrigger, Warning,
                    TEXT("%s: BeginConversation refused (no authority, no dialogue component, or one already active)."),
                    *GetName());
            }
        }
    }

    return bDidAnything;
}

// =============================================================================
// Persistence helpers
// =============================================================================

bool ASFNarrativeTriggerSphere::BuildFiredFactKey(FSFWorldFactKey& OutKey) const
{
    if (TriggerId.IsNone())
    {
        return false;
    }
    OutKey.FactTag = FGameplayTag::RequestGameplayTag(GFiredFactTagName, /*ErrorIfNotFound=*/false);
    if (!OutKey.FactTag.IsValid())
    {
        // Tag isn't registered. We still want persistence to work in projects
        // that haven't added the tag yet, but FSFWorldFactKey hashes on tag
        // equality so we'd lose disambiguation. Warn loudly so the team adds
        // the tag, and fall back to a sentinel tag that at least disambiguates
        // on ContextId for the in-session duration.
        UE_LOG(LogSFNarrativeTrigger, Warning,
            TEXT("%s: gameplay tag '%s' is not registered. Add it to GameplayTags so the trigger persists across saves."),
            *GetName(), *GFiredFactTagName.ToString());
    }
    OutKey.ContextId = TriggerId;
    return OutKey.FactTag.IsValid();
}

bool ASFNarrativeTriggerSphere::QueryPersistedFiredState(USFNarrativeComponent* Narrative) const
{
    if (!Narrative)
    {
        return false;
    }
    FSFWorldFactKey Key;
    if (!BuildFiredFactKey(Key))
    {
        return false;
    }
    FSFWorldFactValue Value;
    if (!Narrative->GetWorldFactValue(Key, Value))
    {
        return false;
    }
    // Treat ResetFiredState's "false" sentinel as not-fired so re-arm works.
    if (Value.ValueType == ESFNarrativeFactValueType::Bool)
    {
        return Value.BoolValue;
    }
    // Presence of the fact with any other shape still means we fired.
    return true;
}

void ASFNarrativeTriggerSphere::StampPersistedFiredState(USFNarrativeComponent* Narrative) const
{
    if (!Narrative)
    {
        return;
    }
    FSFWorldFactKey Key;
    if (!BuildFiredFactKey(Key))
    {
        return;
    }
    Narrative->SetWorldFact(Key, FSFWorldFactValue::MakeBool(true));
}

void ASFNarrativeTriggerSphere::DisableTriggerCollision()
{
    if (!TriggerSphere)
    {
        return;
    }
    TriggerSphere->SetGenerateOverlapEvents(false);
    TriggerSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
