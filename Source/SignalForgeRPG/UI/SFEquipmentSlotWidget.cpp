#include "UI/SFEquipmentSlotWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"

void USFEquipmentSlotWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshFromEntry();
}

void USFEquipmentSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SlotButton)
	{
		SlotButton->OnClicked.RemoveDynamic(this, &USFEquipmentSlotWidget::HandleSlotButtonClicked);
		SlotButton->OnClicked.AddDynamic(this, &USFEquipmentSlotWidget::HandleSlotButtonClicked);
	}

	RefreshFromEntry();
}

void USFEquipmentSlotWidget::SetSlotData(const FSFEquipmentDisplayEntry& Entry)
{
	CurrentEntry = Entry;
	RefreshFromEntry();
}

void USFEquipmentSlotWidget::RefreshFromEntry()
{
	if (SlotNameText)
	{
		SlotNameText->SetText(GetSlotDisplayNameText());
	}

	if (ItemNameText)
	{
		ItemNameText->SetText(GetItemDisplayText());
	}

	if (StateText)
	{
		StateText->SetText(GetStateDisplayText());
	}

	if (ItemIconImage)
	{
		if (CurrentEntry.EquippedItemIcon)
		{
			ItemIconImage->SetBrushFromTexture(CurrentEntry.EquippedItemIcon);
			ItemIconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			ItemIconImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (EquippedStateWidget)
	{
		EquippedStateWidget->SetVisibility(
			CurrentEntry.bHasItemEquipped ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (EmptyStateWidget)
	{
		EmptyStateWidget->SetVisibility(
			CurrentEntry.bHasItemEquipped ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}

	if (ActiveStateWidget)
	{
		ActiveStateWidget->SetVisibility(
			CurrentEntry.bIsActive ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (SlotButton)
	{
		SlotButton->SetIsEnabled(CurrentEntry.bIsInteractable);
	}

	BP_OnSlotDataRefreshed();
}

FText USFEquipmentSlotWidget::GetSlotDisplayNameText() const
{
	return CurrentEntry.SlotDisplayName.IsEmpty()
		? FText::FromString(TEXT("Slot"))
		: CurrentEntry.SlotDisplayName;
}

FText USFEquipmentSlotWidget::GetItemDisplayText() const
{
	if (!CurrentEntry.bHasItemEquipped)
	{
		return FText::FromString(TEXT("Empty"));
	}

	if (!CurrentEntry.EquippedItemName.IsEmpty())
	{
		return CurrentEntry.EquippedItemName;
	}

	return FText::FromString(TEXT("Equipped"));
}

FText USFEquipmentSlotWidget::GetStateDisplayText() const
{
	if (!CurrentEntry.bHasItemEquipped)
	{
		return FText::FromString(TEXT("Empty"));
	}

	if (CurrentEntry.bIsWeaponSlot)
	{
		return CurrentEntry.bIsActive
			? FText::FromString(TEXT("Active"))
			: FText::FromString(TEXT("Equipped"));
	}

	return FText::FromString(TEXT("Equipped"));
}

void USFEquipmentSlotWidget::HandleSlotButtonClicked()
{
	if (!CurrentEntry.bIsInteractable)
	{
		return;
	}

	OnSlotClicked.Broadcast(CurrentEntry);
}