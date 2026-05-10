#include "UI/SFPlayerMenuWidget.h"

#include "Characters/SFCharacterBase.h"
#include "Components/SFEquipmentComponent.h"
#include "Components/SFInventoryComponent.h"
#include "Core/SFPlayerState.h"
#include "Narrative/SFNarrativeComponent.h"
#include "UI/SFPlayerEquipmentWidget.h"
#include "UI/SFPlayerHUDWidgetController.h"
#include "UI/SFPlayerInventoryWidget.h"
#include "UI/SFPlayerQuestLogWidget.h"

void USFPlayerMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void USFPlayerMenuWidget::NativeDestruct()
{
	DeinitializeMenu();
	Super::NativeDestruct();
}

void USFPlayerMenuWidget::InitializeMenu(USFPlayerHUDWidgetController* InHUDWidgetController)
{
	HUDWidgetController = InHUDWidgetController;

	ASFCharacterBase* PlayerCharacter = HUDWidgetController
		? HUDWidgetController->PlayerCharacter.Get()
		: nullptr;

	if (!PlayerCharacter)
	{
		return;
	}

	USFEquipmentComponent* EquipmentComponent = PlayerCharacter->FindComponentByClass<USFEquipmentComponent>();
	USFInventoryComponent* InventoryComponent = PlayerCharacter->FindComponentByClass<USFInventoryComponent>();

	if (EquipmentPanel && EquipmentComponent)
	{
		EquipmentPanel->InitializeEquipmentWidget(EquipmentComponent);
	}

	if (InventoryPanel && InventoryComponent)
	{
		InventoryPanel->InitializeInventoryWidget(InventoryComponent, EquipmentComponent);
	}

	if (QuestLogPanel)
	{
		ASFPlayerState* SFPlayerState = PlayerCharacter->GetPlayerState<ASFPlayerState>();
		USFNarrativeComponent* NarrativeComponent = SFPlayerState ? SFPlayerState->GetNarrativeComponent() : nullptr;

		UE_LOG(LogTemp, Log,
			TEXT("[PlayerMenu] Initializing QuestLogPanel. PlayerCharacter=%s SFPlayerState=%s NarrativeComponent=%s"),
			*GetNameSafe(PlayerCharacter),
			*GetNameSafe(SFPlayerState),
			*GetNameSafe(NarrativeComponent));

		QuestLogPanel->InitializeQuestLogWidget(NarrativeComponent);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[PlayerMenu] QuestLogPanel BindWidgetOptional is null. Drop a WBP_PlayerQuestLog into the menu UMG and rename it to 'QuestLogPanel' (Is Variable checked)."));
	}

	BP_OnMenuInitialized();
}

void USFPlayerMenuWidget::DeinitializeMenu()
{
	if (EquipmentPanel)
	{
		EquipmentPanel->DeinitializeEquipmentWidget();
	}

	if (InventoryPanel)
	{
		InventoryPanel->DeinitializeInventoryWidget();
	}

	if (QuestLogPanel)
	{
		QuestLogPanel->DeinitializeQuestLogWidget();
	}

	HUDWidgetController = nullptr;
}