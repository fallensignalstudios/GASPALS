#include "Dialogue/Data/SFDialogueWidgetController.h"

#include "Characters/SFPlayerAvatarInterface.h"
#include "Dialogue/Data/SFDialogueComponent.h"

void USFDialogueWidgetController::Initialize(AActor* InPlayerAvatar)
{
	PlayerAvatar = InPlayerAvatar;
	DialogueComponent = nullptr;
	bIsDialogueActive = false;
	CurrentDisplayData = FSFDialogueDisplayData();

	ISFPlayerAvatarInterface* AvatarInterface = Cast<ISFPlayerAvatarInterface>(PlayerAvatar);
	if (!AvatarInterface)
	{
		return;
	}

	DialogueComponent = AvatarInterface->GetDialogueComponent();
	if (!DialogueComponent)
	{
		return;
	}

	DialogueComponent->OnConversationStarted.RemoveDynamic(this, &USFDialogueWidgetController::HandleConversationStarted);
	DialogueComponent->OnConversationEnded.RemoveDynamic(this, &USFDialogueWidgetController::HandleConversationEnded);
	DialogueComponent->OnDialogueNodeUpdated.RemoveDynamic(this, &USFDialogueWidgetController::HandleDialogueNodeUpdated);
	DialogueComponent->OnDialoguePauseStateChanged.RemoveDynamic(this, &USFDialogueWidgetController::HandlePauseStateChanged);
	DialogueComponent->OnDialogueEventTriggered.RemoveDynamic(this, &USFDialogueWidgetController::HandleDialogueEvent);

	DialogueComponent->OnConversationStarted.AddDynamic(this, &USFDialogueWidgetController::HandleConversationStarted);
	DialogueComponent->OnConversationEnded.AddDynamic(this, &USFDialogueWidgetController::HandleConversationEnded);
	DialogueComponent->OnDialogueNodeUpdated.AddDynamic(this, &USFDialogueWidgetController::HandleDialogueNodeUpdated);
	DialogueComponent->OnDialoguePauseStateChanged.AddDynamic(this, &USFDialogueWidgetController::HandlePauseStateChanged);
	DialogueComponent->OnDialogueEventTriggered.AddDynamic(this, &USFDialogueWidgetController::HandleDialogueEvent);

	bIsDialogueActive = DialogueComponent->IsConversationActive();
	CurrentDisplayData = DialogueComponent->GetCurrentDisplayData();

	OnDialogueActiveChanged.Broadcast(bIsDialogueActive);

	if (bIsDialogueActive)
	{
		OnDialogueDisplayDataChanged.Broadcast(CurrentDisplayData);
	}
}

void USFDialogueWidgetController::AdvanceDialogue()
{
	if (!DialogueComponent)
	{
		return;
	}

	DialogueComponent->AdvanceConversation();
}

bool USFDialogueWidgetController::SelectChoice(int32 ChoiceIndex)
{
	if (!DialogueComponent)
	{
		return false;
	}

	return DialogueComponent->SelectChoice(ChoiceIndex);
}

bool USFDialogueWidgetController::SkipCurrentLine()
{
	if (!DialogueComponent)
	{
		return false;
	}
	return DialogueComponent->SkipCurrentLine();
}

void USFDialogueWidgetController::PauseDialogue()
{
	if (DialogueComponent)
	{
		DialogueComponent->PauseConversation();
	}
}

void USFDialogueWidgetController::ResumeDialogue()
{
	if (DialogueComponent)
	{
		DialogueComponent->ResumeConversation();
	}
}

void USFDialogueWidgetController::HandleConversationStarted(AActor* SourceActor)
{
	bIsDialogueActive = true;

	if (DialogueComponent)
	{
		CurrentDisplayData = DialogueComponent->GetCurrentDisplayData();
	}

	OnDialogueActiveChanged.Broadcast(bIsDialogueActive);
	OnDialogueDisplayDataChanged.Broadcast(CurrentDisplayData);
}

void USFDialogueWidgetController::HandleConversationEnded()
{
	bIsDialogueActive = false;
	CurrentDisplayData = FSFDialogueDisplayData();

	OnDialogueActiveChanged.Broadcast(bIsDialogueActive);
	OnDialogueDisplayDataChanged.Broadcast(CurrentDisplayData);
}

void USFDialogueWidgetController::HandleDialogueNodeUpdated(const FSFDialogueDisplayData& DisplayData)
{
	CurrentDisplayData = DisplayData;
	OnDialogueDisplayDataChanged.Broadcast(CurrentDisplayData);
}

void USFDialogueWidgetController::HandlePauseStateChanged(bool bPaused)
{
	OnDialoguePauseChanged.Broadcast(bPaused);
}

void USFDialogueWidgetController::HandleDialogueEvent(FGameplayTag EventTag)
{
	OnDialogueEventForwarded.Broadcast(EventTag);
}