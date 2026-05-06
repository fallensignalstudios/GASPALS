#include "Dialogue/Data/SFDialogueComponent.h"
#include "Dialogue/Data/SFConversationDataAsset.h"
#include "GameFramework/Actor.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogSFDialogue, Log, All);

USFDialogueComponent::USFDialogueComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool USFDialogueComponent::StartConversation(USFConversationDataAsset* InConversation, AActor* InSourceActor)
{
	if (bConversationActive)
	{
		UE_LOG(LogSFDialogue, Warning, TEXT("StartConversation called while a conversation is already active on %s. Ending previous conversation."),
			*GetNameSafe(GetOwner()));
		EndConversation();
	}

	if (!IsValid(InConversation))
	{
		UE_LOG(LogSFDialogue, Warning, TEXT("StartConversation failed: InConversation was null."));
		return false;
	}

	TArray<FString> ValidationErrors;
	TArray<FString> ValidationWarnings;
	if (!InConversation->ValidateConversation(ValidationErrors, ValidationWarnings))
	{
		UE_LOG(LogSFDialogue, Warning, TEXT("StartConversation failed: Conversation asset '%s' is invalid."),
			*GetNameSafe(InConversation));

		for (const FString& Warning : ValidationWarnings)
		{
			UE_LOG(LogSFDialogue, Warning, TEXT("Dialogue validation warning: %s"), *Warning);
		}

		for (const FString& Error : ValidationErrors)
		{
			UE_LOG(LogSFDialogue, Warning, TEXT("Dialogue validation error: %s"), *Error);
		}

		return false;
	}

	for (const FString& Warning : ValidationWarnings)
	{
		UE_LOG(LogSFDialogue, Verbose, TEXT("Dialogue validation warning: %s"), *Warning);
	}

	ActiveConversation = InConversation;
	ActiveSourceActor = InSourceActor;
	bConversationActive = true;
	CurrentNodeId = NAME_None;
	CurrentVisibleChoiceIndices.Reset();

	ApplyConversationStaging();

	if (!MoveToNode(InConversation->EntryNodeId))
	{
		return FailConversation(FString::Printf(
			TEXT("Failed to move to entry node '%s' for conversation '%s'."),
			*InConversation->EntryNodeId.ToString(),
			*GetNameSafe(InConversation)
		));
	}

	OnConversationStarted.Broadcast(ActiveSourceActor);
	return true;
}

void USFDialogueComponent::AdvanceConversation()
{
	if (!bConversationActive)
	{
		return;
	}

	const FSFDialogueNode* CurrentNode = GetCurrentNode();
	if (!CurrentNode)
	{
		FailConversation(TEXT("AdvanceConversation failed: current node could not be resolved."));
		return;
	}

	switch (CurrentNode->NodeType)
	{
	case ESFDialogueNodeType::Line:
		if (CurrentNode->NextNodeId == NAME_None)
		{
			EndConversation();
			return;
		}

		if (!MoveToNode(CurrentNode->NextNodeId))
		{
			FailConversation(FString::Printf(
				TEXT("AdvanceConversation failed: could not move from line node '%s' to next node '%s'."),
				*CurrentNode->NodeId.ToString(),
				*CurrentNode->NextNodeId.ToString()
			));
		}
		break;

	case ESFDialogueNodeType::Choice:
		// Waiting for SelectChoice()
		break;

	case ESFDialogueNodeType::Event:
		// Event nodes should normally auto-process in ProcessCurrentNodeChain().
		// If we somehow land here manually, process the chain safely.
		ProcessCurrentNodeChain();
		break;

	case ESFDialogueNodeType::End:
	default:
		EndConversation();
		break;
	}
}

bool USFDialogueComponent::SelectChoice(int32 ChoiceIndex)
{
	if (!bConversationActive)
	{
		return false;
	}

	const FSFDialogueNode* CurrentNode = GetCurrentNode();
	if (!CurrentNode || CurrentNode->NodeType != ESFDialogueNodeType::Choice)
	{
		UE_LOG(LogSFDialogue, Warning, TEXT("SelectChoice failed: current node is not a valid choice node."));
		return false;
	}

	if (!CurrentVisibleChoiceIndices.IsValidIndex(ChoiceIndex))
	{
		UE_LOG(LogSFDialogue, Warning, TEXT("SelectChoice failed: visible choice index %d is invalid on node '%s'."),
			ChoiceIndex,
			*CurrentNode->NodeId.ToString());
		return false;
	}

	const int32 ActualChoiceIndex = CurrentVisibleChoiceIndices[ChoiceIndex];
	if (!CurrentNode->Choices.IsValidIndex(ActualChoiceIndex))
	{
		UE_LOG(LogSFDialogue, Warning, TEXT("SelectChoice failed: actual choice index %d is invalid on node '%s'."),
			ActualChoiceIndex,
			*CurrentNode->NodeId.ToString());
		return false;
	}

	const FSFDialogueChoice& SelectedChoice = CurrentNode->Choices[ActualChoiceIndex];
	if (!DoesChoicePassTagChecks(SelectedChoice))
	{
		UE_LOG(LogSFDialogue, Warning, TEXT("SelectChoice failed: selected choice on node '%s' no longer passes tag checks."),
			*CurrentNode->NodeId.ToString());
		return false;
	}

	if (SelectedChoice.NextNodeId == NAME_None)
	{
		EndConversation();
		return true;
	}

	if (!MoveToNode(SelectedChoice.NextNodeId))
	{
		return FailConversation(FString::Printf(
			TEXT("SelectChoice failed: could not move from node '%s' to choice destination '%s'."),
			*CurrentNode->NodeId.ToString(),
			*SelectedChoice.NextNodeId.ToString()
		));
	}

	return true;
}

void USFDialogueComponent::EndConversation()
{
	if (!bConversationActive)
	{
		return;
	}

	ClearAdvanceState();
	bConversationActive = false;
	ActiveConversation = nullptr;
	ActiveSourceActor = nullptr;
	CurrentNodeId = NAME_None;
	CurrentVisibleChoiceIndices.Reset();

	OnConversationEnded.Broadcast();
	OnDialogueCameraShotChanged.Broadcast(FSFDialogueCameraShot());
}

FSFDialogueDisplayData USFDialogueComponent::GetCurrentDisplayData() const
{
	FSFDialogueDisplayData DisplayData;

	const FSFDialogueNode* CurrentNode = GetCurrentNode();
	if (!CurrentNode)
	{
		DisplayData.NodeType = ESFDialogueNodeType::End;
		return DisplayData;
	}

	DisplayData.SpeakerId = CurrentNode->SpeakerId;
	DisplayData.LineText = CurrentNode->LineText;
	DisplayData.NodeType = CurrentNode->NodeType;

	if (CurrentNode->NodeType == ESFDialogueNodeType::Choice)
	{
		for (const int32 VisibleChoiceIndex : CurrentVisibleChoiceIndices)
		{
			if (CurrentNode->Choices.IsValidIndex(VisibleChoiceIndex))
			{
				DisplayData.Choices.Add(CurrentNode->Choices[VisibleChoiceIndex]);
			}
		}
	}

	return DisplayData;
}

const FSFDialogueNode* USFDialogueComponent::GetCurrentNode() const
{
	if (!IsValid(ActiveConversation) || CurrentNodeId == NAME_None)
	{
		return nullptr;
	}

	return ActiveConversation->FindNodeById(CurrentNodeId);
}

bool USFDialogueComponent::TryResolveNode(FName NodeId, const FSFDialogueNode*& OutNode) const
{
	OutNode = nullptr;

	if (!bConversationActive)
	{
		UE_LOG(LogSFDialogue, Warning, TEXT("TryResolveNode failed: no active conversation."));
		return false;
	}

	if (!IsValid(ActiveConversation))
	{
		UE_LOG(LogSFDialogue, Warning, TEXT("TryResolveNode failed: ActiveConversation is invalid."));
		return false;
	}

	if (NodeId == NAME_None)
	{
		UE_LOG(LogSFDialogue, Warning, TEXT("TryResolveNode failed: NodeId was None."));
		return false;
	}

	OutNode = ActiveConversation->FindNodeById(NodeId);
	if (!OutNode)
	{
		UE_LOG(LogSFDialogue, Warning, TEXT("TryResolveNode failed: node '%s' was not found in conversation '%s'."),
			*NodeId.ToString(),
			*GetNameSafe(ActiveConversation));
		return false;
	}

	return true;
}

bool USFDialogueComponent::MoveToNode(FName NewNodeId)
{
	ClearAdvanceState();

	const FSFDialogueNode* TargetNode = nullptr;
	if (!TryResolveNode(NewNodeId, TargetNode))
	{
		return false;
	}

	if (!DoesNodePassTagChecks(*TargetNode))
	{
		UE_LOG(LogSFDialogue, Warning, TEXT("MoveToNode blocked by tag checks: node '%s' in conversation '%s'."),
			*NewNodeId.ToString(),
			*GetNameSafe(ActiveConversation));
		return false;
	}

	CurrentNodeId = NewNodeId;
	CurrentVisibleChoiceIndices.Reset();

	BroadcastCurrentNode();
	ProcessCurrentNodeChain();
	return true;
}

void USFDialogueComponent::BroadcastCurrentNode()
{
	CurrentVisibleChoiceIndices.Reset();

	const FSFDialogueNode* CurrentNode = GetCurrentNode();
	if (CurrentNode && CurrentNode->NodeType == ESFDialogueNodeType::Choice)
	{
		for (int32 Index = 0; Index < CurrentNode->Choices.Num(); ++Index)
		{
			if (DoesChoicePassTagChecks(CurrentNode->Choices[Index]))
			{
				CurrentVisibleChoiceIndices.Add(Index);
			}
		}

		if (CurrentVisibleChoiceIndices.Num() == 0)
		{
			UE_LOG(LogSFDialogue, Warning,
				TEXT("Choice node '%s' has no visible choices after tag filtering."),
				*CurrentNode->NodeId.ToString());
		}
	}

	// Configure timers/audio for the new node before notifying listeners.
	ConfigureAdvanceForCurrentNode();

	// Notify UI of the new display data.
	OnDialogueNodeUpdated.Broadcast(GetCurrentDisplayData());

	if (CurrentNode)
	{
		OnDialogueCameraShotChanged.Broadcast(CurrentNode->CameraShot);
	}
}

void USFDialogueComponent::ProcessCurrentNodeChain()
{
	// Guards against bad authored infinite chains.
	constexpr int32 MaxAutoAdvanceSteps = 64;

	for (int32 StepIndex = 0; StepIndex < MaxAutoAdvanceSteps; ++StepIndex)
	{
		if (!bConversationActive)
		{
			return;
		}

		const FSFDialogueNode* CurrentNode = GetCurrentNode();
		if (!CurrentNode)
		{
			FailConversation(TEXT("ProcessCurrentNodeChain failed: current node could not be resolved."));
			return;
		}

		switch (CurrentNode->NodeType)
		{
		case ESFDialogueNodeType::Event:
		{
			if (CurrentNode->EventTag.IsValid())
			{
				OnDialogueEventTriggered.Broadcast(CurrentNode->EventTag);
			}
			else
			{
				UE_LOG(LogSFDialogue, Verbose, TEXT("Event node '%s' has no valid EventTag."),
					*CurrentNode->NodeId.ToString());
			}

			if (CurrentNode->NextNodeId == NAME_None)
			{
				EndConversation();
				return;
			}

			const FSFDialogueNode* NextNode = nullptr;
			if (!TryResolveNode(CurrentNode->NextNodeId, NextNode))
			{
				FailConversation(FString::Printf(
					TEXT("Event node '%s' points to invalid next node '%s'."),
					*CurrentNode->NodeId.ToString(),
					*CurrentNode->NextNodeId.ToString()
				));
				return;
			}

			if (!DoesNodePassTagChecks(*NextNode))
			{
				FailConversation(FString::Printf(
					TEXT("Event node '%s' points to gated next node '%s' that failed tag checks."),
					*CurrentNode->NodeId.ToString(),
					*CurrentNode->NextNodeId.ToString()
				));
				return;
			}

			CurrentNodeId = CurrentNode->NextNodeId;
			BroadcastCurrentNode();
			break;
		}

		case ESFDialogueNodeType::End:
			EndConversation();
			return;

		case ESFDialogueNodeType::Choice:
		case ESFDialogueNodeType::Line:
		default:
			return;
		}
	}

	FailConversation(TEXT("ProcessCurrentNodeChain exceeded MaxAutoAdvanceSteps. Possible dialogue loop detected."));
}

bool USFDialogueComponent::DoesNodePassTagChecks(const FSFDialogueNode& Node) const
{
	FGameplayTagContainer OwnedTags;
	GetOwnedDialogueTags(OwnedTags);

	if (OwnedTags.HasAny(Node.BlockedTags))
	{
		return false;
	}

	if (!Node.RequiredTags.IsEmpty() && !OwnedTags.HasAll(Node.RequiredTags))
	{
		return false;
	}

	return true;
}

bool USFDialogueComponent::DoesChoicePassTagChecks(const FSFDialogueChoice& Choice) const
{
	FGameplayTagContainer OwnedTags;
	GetOwnedDialogueTags(OwnedTags);

	if (OwnedTags.HasAny(Choice.BlockedTags))
	{
		return false;
	}

	if (!Choice.RequiredTags.IsEmpty() && !OwnedTags.HasAll(Choice.RequiredTags))
	{
		return false;
	}

	return true;
}

bool USFDialogueComponent::FailConversation(const FString& Reason)
{
	UE_LOG(LogSFDialogue, Warning, TEXT("Dialogue failure on '%s': %s"),
		*GetNameSafe(GetOwner()),
		*Reason);

	EndConversation();
	return false;
}

void USFDialogueComponent::GetOwnedDialogueTags(FGameplayTagContainer& OutTags) const
{
	OutTags.Reset();

	// Intentional placeholder.
	// Later, pull from:
	// - player ASC owned tags
	// - quest state tags
	// - world state subsystem
	// - reputation / faction tags
}

void USFDialogueComponent::ClearAdvanceState()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoAdvanceTimerHandle);
	}

	if (ActiveDialogueAudioComponent)
	{
		ActiveDialogueAudioComponent->OnAudioFinished.RemoveDynamic(this, &USFDialogueComponent::HandleDialogueVoiceFinished);
		ActiveDialogueAudioComponent->Stop();
		ActiveDialogueAudioComponent = nullptr;
	}
}

float USFDialogueComponent::CalculateWordsPerMinuteDelay(const FText& InText, float InWordsPerMinute) const
{
	const FString TextString = InText.ToString().TrimStartAndEnd();
	if (TextString.IsEmpty() || InWordsPerMinute <= 0.0f)
	{
		return 0.0f;
	}

	TArray<FString> Words;
	TextString.ParseIntoArrayWS(Words);

	const float WordCount = static_cast<float>(Words.Num());
	const float Minutes = WordCount / InWordsPerMinute;
	const float Seconds = Minutes * 60.0f;

	return Seconds;
}

void USFDialogueComponent::HandleDialogueVoiceFinished()
{
	ActiveDialogueAudioComponent = nullptr;

	const FSFDialogueNode* CurrentNode = GetCurrentNode();
	if (!CurrentNode || CurrentNode->NodeType != ESFDialogueNodeType::Line)
	{
		return;
	}

	AdvanceConversation();
}

void USFDialogueComponent::ConfigureAdvanceForCurrentNode()
{
	ClearAdvanceState();

	if (!bConversationActive)
	{
		return;
	}

	const FSFDialogueNode* CurrentNode = GetCurrentNode();
	if (!CurrentNode)
	{
		return;
	}

	if (CurrentNode->NodeType != ESFDialogueNodeType::Line)
	{
		return;
	}

	switch (CurrentNode->AdvanceMode)
	{
	case ESFDialogueAdvanceMode::Manual:
		break;

	case ESFDialogueAdvanceMode::FixedDuration:
	{
		if (CurrentNode->AdvanceDelaySeconds > 0.0f)
		{
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimer(
					AutoAdvanceTimerHandle,
					this,
					&USFDialogueComponent::AdvanceConversation,
					CurrentNode->AdvanceDelaySeconds,
					false
				);
			}
		}
		break;
	}

	case ESFDialogueAdvanceMode::WordsPerMinute:
	{
		const float Delay = CalculateWordsPerMinuteDelay(CurrentNode->LineText, CurrentNode->WordsPerMinute);
		if (Delay > 0.0f)
		{
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimer(
					AutoAdvanceTimerHandle,
					this,
					&USFDialogueComponent::AdvanceConversation,
					Delay,
					false
				);
			}
		}
		break;
	}

	case ESFDialogueAdvanceMode::AudioFinished:
	{
		if (CurrentNode->VoiceClip)
		{
			ActiveDialogueAudioComponent = UGameplayStatics::SpawnSound2D(this, CurrentNode->VoiceClip);
			if (ActiveDialogueAudioComponent)
			{
				ActiveDialogueAudioComponent->OnAudioFinished.AddDynamic(this, &USFDialogueComponent::HandleDialogueVoiceFinished);
			}
		}
		break;
	}

	default:
		break;
	}
}

void USFDialogueComponent::ApplyConversationStaging()
{
	if (!ActiveConversation)
	{
		return;
	}

	const FSFDialogueStagingData& Staging = ActiveConversation->StagingData;
	if (Staging.PositioningMode == ESFDialoguePositioningMode::None)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	AActor* SourceActor = ActiveSourceActor;

	if (Staging.PositioningMode == ESFDialoguePositioningMode::TeleportPlayer && OwnerActor)
	{
		OwnerActor->SetActorLocationAndRotation(
			Staging.PlayerTransform.GetLocation(),
			Staging.PlayerTransform.GetRotation().Rotator()
		);
	}
	else if (Staging.PositioningMode == ESFDialoguePositioningMode::TeleportBoth)
	{
		if (OwnerActor)
		{
			OwnerActor->SetActorLocationAndRotation(
				Staging.PlayerTransform.GetLocation(),
				Staging.PlayerTransform.GetRotation().Rotator()
			);
		}

		if (SourceActor)
		{
			SourceActor->SetActorLocationAndRotation(
				Staging.SourceTransform.GetLocation(),
				Staging.SourceTransform.GetRotation().Rotator()
			);
		}
	}

	if (Staging.bSnapPlayerRotationToSource && OwnerActor && SourceActor)
	{
		const FVector Direction = (SourceActor->GetActorLocation() - OwnerActor->GetActorLocation()).GetSafeNormal2D();
		if (!Direction.IsNearlyZero())
		{
			const FRotator LookRotation = Direction.Rotation();
			OwnerActor->SetActorRotation(FRotator(0.f, LookRotation.Yaw, 0.f));
		}
	}
}