#include "UI/SFDialoguePanelWidget.h"

#include "Characters/SFPlayerAvatarInterface.h"
#include "Components/PanelWidget.h"
#include "Components/RichTextBlock.h"
#include "Components/TextBlock.h"
#include "Dialogue/Data/SFDialogueComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UI/SFDialogueChoiceButtonWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogSFDialoguePanel, Log, All);

void USFDialoguePanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (bAutoBindOnConstruct)
	{
		BindToLocalPlayerDialogueComponent();
	}

	RefreshVisibility(BoundDialogueComponent && BoundDialogueComponent->IsConversationActive());
}

void USFDialoguePanelWidget::NativeDestruct()
{
	UnbindFromComponent(BoundDialogueComponent);
	Super::NativeDestruct();
}

void USFDialoguePanelWidget::BindToLocalPlayerDialogueComponent()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		UE_LOG(LogSFDialoguePanel, Verbose, TEXT("BindToLocalPlayerDialogueComponent: no PlayerController yet."));
		return;
	}

	APawn* Pawn = PC->GetPawn();
	if (!Pawn || !Pawn->Implements<USFPlayerAvatarInterface>())
	{
		UE_LOG(LogSFDialoguePanel, Verbose, TEXT("BindToLocalPlayerDialogueComponent: local pawn is not a player avatar (is '%s')."),
			*GetNameSafe(Pawn));
		return;
	}

	USFDialogueComponent* DialogueComp = ISFPlayerAvatarInterface::Execute_GetDialogueComponent(Pawn);
	if (!DialogueComp)
	{
		UE_LOG(LogSFDialoguePanel, Warning, TEXT("BindToLocalPlayerDialogueComponent: player has no USFDialogueComponent."));
		return;
	}

	SetDialogueComponent(DialogueComp);
}

void USFDialoguePanelWidget::SetDialogueComponent(USFDialogueComponent* InComponent)
{
	if (BoundDialogueComponent == InComponent)
	{
		return;
	}

	UnbindFromComponent(BoundDialogueComponent);
	BoundDialogueComponent = InComponent;
	BindToComponent(BoundDialogueComponent);

	// If a conversation is already active, sync immediately so the panel reflects state.
	if (BoundDialogueComponent && BoundDialogueComponent->IsConversationActive())
	{
		ApplyDisplayData(BoundDialogueComponent->GetCurrentDisplayData());
		RefreshVisibility(true);
		OnDialogueOpened();
	}
	else
	{
		RefreshVisibility(false);
	}
}

void USFDialoguePanelWidget::BindToComponent(USFDialogueComponent* InComponent)
{
	if (!InComponent)
	{
		return;
	}

	InComponent->OnConversationStarted.AddDynamic(this, &USFDialoguePanelWidget::HandleConversationStarted);
	InComponent->OnConversationEnded.AddDynamic(this, &USFDialoguePanelWidget::HandleConversationEnded);
	InComponent->OnDialogueNodeUpdated.AddDynamic(this, &USFDialoguePanelWidget::HandleDialogueNodeUpdated);
}

void USFDialoguePanelWidget::UnbindFromComponent(USFDialogueComponent* InComponent)
{
	if (!InComponent)
	{
		return;
	}

	InComponent->OnConversationStarted.RemoveDynamic(this, &USFDialoguePanelWidget::HandleConversationStarted);
	InComponent->OnConversationEnded.RemoveDynamic(this, &USFDialoguePanelWidget::HandleConversationEnded);
	InComponent->OnDialogueNodeUpdated.RemoveDynamic(this, &USFDialoguePanelWidget::HandleDialogueNodeUpdated);
}

void USFDialoguePanelWidget::AdvanceDialogue()
{
	if (BoundDialogueComponent)
	{
		BoundDialogueComponent->AdvanceConversation();
	}
}

bool USFDialoguePanelWidget::SelectChoice(int32 ChoiceIndex)
{
	if (!BoundDialogueComponent)
	{
		return false;
	}
	return BoundDialogueComponent->SelectChoice(ChoiceIndex);
}

void USFDialoguePanelWidget::EndDialogue()
{
	if (BoundDialogueComponent)
	{
		BoundDialogueComponent->EndConversation();
	}
}

void USFDialoguePanelWidget::HandleConversationStarted(AActor* /*SourceActor*/)
{
	if (BoundDialogueComponent)
	{
		ApplyDisplayData(BoundDialogueComponent->GetCurrentDisplayData());
	}
	RefreshVisibility(true);
	OnDialogueOpened();
}

void USFDialoguePanelWidget::HandleConversationEnded()
{
	RebuildChoices({});
	if (SpeakerNameText)
	{
		SpeakerNameText->SetText(FText::GetEmpty());
	}
	if (LineText)
	{
		LineText->SetText(FText::GetEmpty());
	}
	if (LineRichText)
	{
		LineRichText->SetText(FText::GetEmpty());
	}

	RefreshVisibility(false);
	OnDialogueClosed();
}

void USFDialoguePanelWidget::HandleDialogueNodeUpdated(const FSFDialogueDisplayData& DisplayData)
{
	ApplyDisplayData(DisplayData);
}

void USFDialoguePanelWidget::ApplyDisplayData(const FSFDialogueDisplayData& DisplayData)
{
	if (SpeakerNameText)
	{
		SpeakerNameText->SetText(FText::FromName(DisplayData.SpeakerId));
	}

	if (LineText)
	{
		LineText->SetText(DisplayData.LineText);
	}
	if (LineRichText)
	{
		LineRichText->SetText(DisplayData.LineText);
	}

	RebuildChoices(DisplayData.Choices);

	OnDisplayDataChanged(DisplayData);
}

void USFDialoguePanelWidget::RebuildChoices(const TArray<FSFDialogueChoice>& Choices)
{
	if (!ChoicesContainer)
	{
		UE_LOG(LogSFDialoguePanel, Warning,
			TEXT("RebuildChoices: ChoicesContainer is not bound on '%s'. Make sure the BP exposes a UPanelWidget named 'ChoicesContainer'."),
			*GetNameSafe(this));
		return;
	}

	// Clear previous buttons.
	for (USFDialogueChoiceButtonWidget* Button : SpawnedChoiceButtons)
	{
		if (Button)
		{
			Button->OnChoiceClicked.RemoveDynamic(this, &USFDialoguePanelWidget::HandleChoiceButtonClicked);
			Button->RemoveFromParent();
		}
	}
	SpawnedChoiceButtons.Reset();
	ChoicesContainer->ClearChildren();

	if (Choices.Num() == 0)
	{
		return;
	}

	if (!ChoiceButtonClass)
	{
		UE_LOG(LogSFDialoguePanel, Warning,
			TEXT("RebuildChoices: ChoiceButtonClass is not set on '%s'. Set it in the dialogue panel's class defaults."),
			*GetNameSafe(this));
		return;
	}

	for (int32 Index = 0; Index < Choices.Num(); ++Index)
	{
		USFDialogueChoiceButtonWidget* Button =
			CreateWidget<USFDialogueChoiceButtonWidget>(this, ChoiceButtonClass);
		if (!Button)
		{
			continue;
		}

		Button->SetChoiceIndex(Index);
		Button->SetChoiceText(Choices[Index].ChoiceText);
		Button->OnChoiceClicked.AddDynamic(this, &USFDialoguePanelWidget::HandleChoiceButtonClicked);

		ChoicesContainer->AddChild(Button);
		SpawnedChoiceButtons.Add(Button);
	}
}

void USFDialoguePanelWidget::HandleChoiceButtonClicked(int32 ChoiceIndex)
{
	SelectChoice(ChoiceIndex);
}

void USFDialoguePanelWidget::RefreshVisibility(bool bDialogueActive)
{
	if (!bAutoHideWhenInactive)
	{
		return;
	}

	SetVisibility(bDialogueActive ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}
