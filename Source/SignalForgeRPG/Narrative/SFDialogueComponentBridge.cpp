// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "SFDialogueComponentBridge.h"
#include "SFNarrativeComponent.h"
#include "SFNarrativeLog.h"
#include "Dialogue/Data/SFConversationDataAsset.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerState.h"

USFDialogueComponentBridge::USFDialogueComponentBridge()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void USFDialogueComponentBridge::BeginPlay()
{
    Super::BeginPlay();

    AutoBindComponents();
    BindDialogueEvents();
}

void USFDialogueComponentBridge::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnbindDialogueEvents();
    Super::EndPlay(EndPlayReason);
}

void USFDialogueComponentBridge::AutoBindComponents()
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return;
    }

    if (!DialogueComponent)
    {
        DialogueComponent = OwnerActor->FindComponentByClass<USFDialogueComponent>();
    }

    if (!NarrativeComponent)
    {
        NarrativeComponent = OwnerActor->FindComponentByClass<USFNarrativeComponent>();
    }

    if (!NarrativeComponent)
    {
        // It is valid to have a dialogue bridge without a narrative component (e.g. in tools),
        // but it means we will only fire our own Blueprint delegates.
        NARR_VLOG(TEXT("USFDialogueComponentBridge on %s did not find a USFNarrativeComponent sibling."),
            *GetNameSafe(OwnerActor));
    }
}

void USFDialogueComponentBridge::SetDialogueComponent(USFDialogueComponent* InDialogueComponent)
{
    if (DialogueComponent == InDialogueComponent)
    {
        return;
    }

    UnbindDialogueEvents();
    DialogueComponent = InDialogueComponent;
    BindDialogueEvents();
}

void USFDialogueComponentBridge::SetNarrativeComponent(USFNarrativeComponent* InNarrativeComponent)
{
    NarrativeComponent = InNarrativeComponent;
}

void USFDialogueComponentBridge::BindDialogueEvents()
{
    if (!DialogueComponent)
    {
        return;
    }

    DialogueComponent->OnConversationStarted.AddDynamic(this, &USFDialogueComponentBridge::HandleConversationStarted);
    DialogueComponent->OnConversationEnded.AddDynamic(this, &USFDialogueComponentBridge::HandleConversationEnded);
    DialogueComponent->OnDialogueNodeUpdated.AddDynamic(this, &USFDialogueComponentBridge::HandleDialogueNodeUpdated);
    DialogueComponent->OnDialogueEventTriggered.AddDynamic(this, &USFDialogueComponentBridge::HandleDialogueEventTriggered);
    DialogueComponent->OnDialogueCameraShotChanged.AddDynamic(this, &USFDialogueComponentBridge::HandleDialogueCameraShotChanged);
}

void USFDialogueComponentBridge::UnbindDialogueEvents()
{
    if (!DialogueComponent)
    {
        return;
    }

    DialogueComponent->OnConversationStarted.RemoveDynamic(this, &USFDialogueComponentBridge::HandleConversationStarted);
    DialogueComponent->OnConversationEnded.RemoveDynamic(this, &USFDialogueComponentBridge::HandleConversationEnded);
    DialogueComponent->OnDialogueNodeUpdated.RemoveDynamic(this, &USFDialogueComponentBridge::HandleDialogueNodeUpdated);
    DialogueComponent->OnDialogueEventTriggered.RemoveDynamic(this, &USFDialogueComponentBridge::HandleDialogueEventTriggered);
    DialogueComponent->OnDialogueCameraShotChanged.RemoveDynamic(this, &USFDialogueComponentBridge::HandleDialogueCameraShotChanged);
}

FSFNarrativeInstigator USFDialogueComponentBridge::BuildInstigator() const
{
    FSFNarrativeInstigator Instigator;

    AActor* OwnerActor = GetOwner();
    Instigator.FocusActor = OwnerActor;
    Instigator.NarrativeComponent = NarrativeComponent;

    if (OwnerActor)
    {
        if (APawn* Pawn = Cast<APawn>(OwnerActor))
        {
            Instigator.PlayerState = Pawn->GetPlayerState();
        }
        else
        {
            if (APawn* OwningPawn = Cast<APawn>(OwnerActor->GetOwner()))
            {
                Instigator.PlayerState = OwningPawn->GetPlayerState();
            }
        }
    }

    return Instigator;
}

FSFDialogueMoment USFDialogueComponentBridge::BuildDialogueMoment(const FSFDialogueDisplayData& DisplayData) const
{
    FSFDialogueMoment Moment;
    Moment.CurrentSpeakerId = DisplayData.SpeakerId;
    Moment.bIsTerminal = (DisplayData.NodeType == ESFDialogueNodeType::End);

    // Build a lightweight node definition from the display data so the narrative
    // side can treat it uniformly with definition?driven paths.
    FSFDialogueNodeDefinition NodeDef;
    NodeDef.NodeId = DialogueComponent ? DialogueComponent->GetCurrentNodeId() : NAME_None;
    NodeDef.Line = DisplayData.LineText;
    NodeDef.SpeakerId = DisplayData.SpeakerId;
    NodeDef.bPlayerResponse = (DisplayData.NodeType == ESFDialogueNodeType::Choice);
    // EventTags / MemoryTags / NextNodeIds can be filled if your display data exposes them later.

    Moment.CurrentNode = NodeDef;

    if (DisplayData.NodeType == ESFDialogueNodeType::Choice)
    {
        for (int32 Index = 0; Index < DisplayData.Choices.Num(); ++Index)
        {
            const FSFDialogueChoice& RawChoice = DisplayData.Choices[Index];

            FSFDialogueOptionView Option;
            Option.NodeAddress = FSFDialogueNodeAddress(CurrentDialogueId, RawChoice.NextNodeId);
            Option.Line = RawChoice.ChoiceText;
            Option.SpeakerId = DisplayData.SpeakerId;
            Option.bIsAvailable = true; // At this point DisplayData already filtered by tags.
            // FSFDialogueChoice does not expose authored outcome-preview tags in the canonical dialogue schema.

            Moment.Options.Add(Option);
        }
    }

    return Moment;
}

void USFDialogueComponentBridge::EmitNarrativeDelta(const FSFNarrativeDelta& Delta)
{
    // Local delegate for any listeners bound directly to the bridge.
    OnNarrativeDialogueDeltaEmitted.Broadcast(Delta);

    // Forward to the narrative component if present.
    if (NarrativeComponent)
    {
        NarrativeComponent->ApplyNarrativeDelta(Delta);
    }

    // Native logging helper.
    NARR_LOG_DELTA(Delta);
}

//
// Event handlers bound to USFDialogueComponent
//

void USFDialogueComponentBridge::HandleConversationStarted(AActor* SourceActor)
{
    DialogueDeltaSequence = 0;

    // Derive a reasonable dialogue ID – by default from the conversation asset name.
    CurrentDialogueId = NAME_None;

    if (DialogueComponent)
    {
        if (USFConversationDataAsset* Conversation = DialogueComponent->GetActiveConversation())
        {
            CurrentDialogueId = FName(*Conversation->GetName());
        }
    }

    const FSFNarrativeInstigator Instigator = BuildInstigator();

    FSFNarrativeDelta Delta = FSFNarrativeDelta::MakeBeginDialogue(
        ++DialogueDeltaSequence,
        CurrentDialogueId,
        DialogueComponent ? DialogueComponent->GetCurrentNodeId() : NAME_None,
        /*Priority*/ 0); // If you add priority to the conversation asset, feed it here.

    EmitNarrativeDelta(Delta);

    NARR_LOG(Display, TEXT("Dialogue started: %s on %s (Source=%s)"),
        *CurrentDialogueId.ToString(),
        *GetNameSafe(GetOwner()),
        *GetNameSafe(SourceActor));
}

void USFDialogueComponentBridge::HandleConversationEnded()
{
    const FSFNarrativeInstigator Instigator = BuildInstigator();

    // You may want to deduce a more specific reason; for now we treat all as Completed.
    const ESFDialogueExitReason Reason = ESFDialogueExitReason::Completed;

    FSFNarrativeDelta Delta = FSFNarrativeDelta::MakeExitDialogue(
        ++DialogueDeltaSequence,
        CurrentDialogueId,
        Reason);

    EmitNarrativeDelta(Delta);

    NARR_LOG(Display, TEXT("Dialogue ended: %s on %s"),
        *CurrentDialogueId.ToString(),
        *GetNameSafe(GetOwner()));

    // Reset ID after emitting the final delta.
    CurrentDialogueId = NAME_None;
    LastDialogueMoment = FSFDialogueMoment();
}

void USFDialogueComponentBridge::HandleDialogueNodeUpdated(const FSFDialogueDisplayData& DisplayData)
{
    LastDialogueMoment = BuildDialogueMoment(DisplayData);

    // Narrative?style UI event.
    OnNarrativeDialogueMomentUpdated.Broadcast(CurrentDialogueId, LastDialogueMoment, ++DialogueDeltaSequence);

    // Emit a delta to track advancement / selection.
    switch (DisplayData.NodeType)
    {
    case ESFDialogueNodeType::Line:
    case ESFDialogueNodeType::Choice:
    {
        FSFNarrativeDelta Delta = FSFNarrativeDelta::MakeAdvanceDialogue(
            DialogueDeltaSequence,
            CurrentDialogueId,
            DialogueComponent ? DialogueComponent->GetCurrentNodeId() : NAME_None,
            DisplayData.SpeakerId);

        EmitNarrativeDelta(Delta);
        break;
    }

    case ESFDialogueNodeType::End:
    default:
        // End is handled by HandleConversationEnded().
        break;
    }
}

void USFDialogueComponentBridge::HandleDialogueEventTriggered(FGameplayTag EventTag)
{
    FSFNarrativeDelta Delta = FSFNarrativeDelta::MakeTriggerDialogueEvent(
        ++DialogueDeltaSequence,
        CurrentDialogueId,
        DialogueComponent ? DialogueComponent->GetCurrentNodeId() : NAME_None,
        EventTag);

    EmitNarrativeDelta(Delta);
}

void USFDialogueComponentBridge::HandleDialogueCameraShotChanged(const FSFDialogueCameraShot& CameraShot)
{
    // Right now we only care about this for debugging/logging or future camera?driven tags.
    // If you later pipe camera state into narrative/world facts, this is where you’d do it.
    NARR_VLOG(TEXT("Dialogue camera shot changed on %s (DialogueId=%s)"),
        *GetNameSafe(GetOwner()),
        *CurrentDialogueId.ToString());
}
