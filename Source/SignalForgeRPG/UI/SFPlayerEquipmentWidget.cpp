#include "UI/SFPlayerEquipmentWidget.h"

#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SFEquipmentComponent.h"
#include "UI/SFEquipmentSlotWidget.h"
#include "UI/SFEquipmentWidgetController.h"

void USFPlayerEquipmentWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetController)
	{
		WidgetController = NewObject<USFEquipmentWidgetController>(this);
	}

	if (WidgetController)
	{
		WidgetController->OnEquipmentDisplayDataUpdated.RemoveDynamic(this, &USFPlayerEquipmentWidget::HandleEquipmentDisplayDataUpdated);
		WidgetController->OnEquipmentDisplayDataUpdated.AddDynamic(this, &USFPlayerEquipmentWidget::HandleEquipmentDisplayDataUpdated);
	}
}

void USFPlayerEquipmentWidget::NativeDestruct()
{
	DeinitializeEquipmentWidget();
	Super::NativeDestruct();
}

void USFPlayerEquipmentWidget::InitializeEquipmentWidget(USFEquipmentComponent* InEquipmentComponent)
{
	EquipmentComponent = InEquipmentComponent;

	if (!WidgetController)
	{
		WidgetController = NewObject<USFEquipmentWidgetController>(this, USFEquipmentWidgetController::StaticClass());
	}

	if (WidgetController)
	{
		WidgetController->OnEquipmentDisplayDataUpdated.RemoveDynamic(this, &USFPlayerEquipmentWidget::HandleEquipmentDisplayDataUpdated);
		WidgetController->OnEquipmentDisplayDataUpdated.AddDynamic(this, &USFPlayerEquipmentWidget::HandleEquipmentDisplayDataUpdated);
		WidgetController->Initialize(EquipmentComponent);
	}
	else
	{
		RebuildSlotWidgets();
	}
}

void USFPlayerEquipmentWidget::DeinitializeEquipmentWidget()
{
	if (WidgetController)
	{
		WidgetController->OnEquipmentDisplayDataUpdated.RemoveDynamic(this, &USFPlayerEquipmentWidget::HandleEquipmentDisplayDataUpdated);
		WidgetController->Deinitialize();
		WidgetController = nullptr;
	}

	EquipmentComponent = nullptr;

	if (ArmorSlotList)
	{
		ArmorSlotList->ClearChildren();
	}

	if (WeaponSlotList)
	{
		WeaponSlotList->ClearChildren();
	}
}

TArray<FSFEquipmentDisplayEntry> USFPlayerEquipmentWidget::GetCurrentDisplayEntries() const
{
	return WidgetController ? WidgetController->GetCurrentDisplayEntries() : TArray<FSFEquipmentDisplayEntry>();
}

void USFPlayerEquipmentWidget::HandleEquipmentDisplayDataUpdated()
{
	RebuildSlotWidgets();
}

void USFPlayerEquipmentWidget::HandleSlotWidgetClicked(const FSFEquipmentDisplayEntry& Entry)
{
	OnEquipmentSlotClicked.Broadcast(Entry);
}

void USFPlayerEquipmentWidget::RebuildSlotWidgets()
{
	if (ArmorSlotList)
	{
		ArmorSlotList->ClearChildren();
	}

	if (WeaponSlotList)
	{
		WeaponSlotList->ClearChildren();
	}

	if (!WidgetController)
	{
		return;
	}

	TSubclassOf<USFEquipmentSlotWidget> SlotWidgetClass = EquipmentSlotWidgetClass;
	if (!SlotWidgetClass)
	{
		SlotWidgetClass = USFEquipmentSlotWidget::StaticClass();
	}

	const TArray<FSFEquipmentDisplayEntry> Entries = WidgetController->GetCurrentDisplayEntries();

	for (const FSFEquipmentDisplayEntry& Entry : Entries)
	{
		USFEquipmentSlotWidget* SlotWidget = CreateWidget<USFEquipmentSlotWidget>(this, SlotWidgetClass);
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->SetSlotData(Entry);
		SlotWidget->OnSlotClicked.RemoveDynamic(this, &USFPlayerEquipmentWidget::HandleSlotWidgetClicked);
		SlotWidget->OnSlotClicked.AddDynamic(this, &USFPlayerEquipmentWidget::HandleSlotWidgetClicked);

		if (IsArmorSlotType(Entry.SlotType))
		{
			if (ArmorSlotList)
			{
				if (UVerticalBoxSlot* AddedSlot = ArmorSlotList->AddChildToVerticalBox(SlotWidget))
				{
					AddedSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
				}
			}
		}
		else if (IsWeaponSlotType(Entry.SlotType))
		{
			if (WeaponSlotList)
			{
				if (UVerticalBoxSlot* AddedSlot = WeaponSlotList->AddChildToVerticalBox(SlotWidget))
				{
					AddedSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
				}
			}
		}
	}
}

bool USFPlayerEquipmentWidget::IsArmorSlotType(ESFEquipmentSlotType SlotType) const
{
	return SlotType == ESFEquipmentSlotType::Head
		|| SlotType == ESFEquipmentSlotType::Chest
		|| SlotType == ESFEquipmentSlotType::Arms
		|| SlotType == ESFEquipmentSlotType::Legs
		|| SlotType == ESFEquipmentSlotType::Boots;
}

bool USFPlayerEquipmentWidget::IsWeaponSlotType(ESFEquipmentSlotType SlotType) const
{
	return SlotType == ESFEquipmentSlotType::PrimaryWeapon
		|| SlotType == ESFEquipmentSlotType::SecondaryWeapon
		|| SlotType == ESFEquipmentSlotType::HeavyWeapon;
}