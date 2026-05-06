#include "Dialogue/Data/SFDialogueWidgetController.h"

#include "Characters/SFPlayerCharacter.h"
#include "Dialogue/Data/SFDialogueComponent.h"

void USFDialogueWidgetController::Initialize(ASFPlayerCharacter* InPlayerCharacter)
{
	PlayerCharacter = InPlayerCharacter;
	DialogueComponent = nullptr;
	bIsDialogueActive = false;
	CurrentDisplayData = FSFDialogueDisplayData();

	if (!PlayerCharacter)
	{
		return;
	}

	DialogueComponent = PlayerCharacter->GetDialogueComponent();
	if (!DialogueComponent)
	{
		return;
	}

	DialogueComponent->OnConversationStarted.RemoveDynamic(this, &USFDialogueWidgetController::HandleConversationStarted);
	DialogueComponent->OnConversationEnded.RemoveDynamic(this, &USFDialogueWidgetController::HandleConversationEnded);
	DialogueComponent->OnDialogueNodeUpdated.RemoveDynamic(this, &USFDialogueWidgetController::HandleDialogueNodeUpdated);

	DialogueComponent->OnConversationStarted.AddDynamic(this, &USFDialogueWidgetController::HandleConversationStarted);
	DialogueComponent->OnConversationEnded.AddDynamic(this, &USFDialogueWidgetController::HandleConversationEnded);
	DialogueComponent->OnDialogueNodeUpdated.AddDynamic(this, &USFDialogueWidgetController::HandleDialogueNodeUpdated);

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