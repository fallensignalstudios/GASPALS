#include "UI/SFPlayerMenuWidget.h"

#include "Characters/SFCharacterBase.h"
#include "Components/SFEquipmentComponent.h"
#include "Components/SFInventoryComponent.h"
#include "UI/SFPlayerEquipmentWidget.h"
#include "UI/SFPlayerHUDWidgetController.h"
#include "UI/SFPlayerInventoryWidget.h"

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

	HUDWidgetController = nullptr;
}