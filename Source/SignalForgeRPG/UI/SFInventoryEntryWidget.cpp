#include "UI/SFInventoryEntryWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Input/Reply.h"
#include "Inventory/SFItemDefinition.h"

#define LOCTEXT_NAMESPACE "SFInventoryEntryWidget"

void USFInventoryEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (EntryButton && !EntryButton->OnClicked.IsAlreadyBound(this, &USFInventoryEntryWidget::HandleEntryButtonClicked))
	{
		EntryButton->OnClicked.AddDynamic(this, &USFInventoryEntryWidget::HandleEntryButtonClicked);
	}

	if (EquipButton && !EquipButton->OnClicked.IsAlreadyBound(this, &USFInventoryEntryWidget::HandleEquipButtonClicked))
	{
		EquipButton->OnClicked.AddDynamic(this, &USFInventoryEntryWidget::HandleEquipButtonClicked);
	}

	RefreshVisuals();
	RefreshSelectionVisuals();
	RefreshActionState();
}

void USFInventoryEntryWidget::NativeDestruct()
{
	if (EntryButton)
	{
		EntryButton->OnClicked.RemoveDynamic(this, &USFInventoryEntryWidget::HandleEntryButtonClicked);
	}

	if (EquipButton)
	{
		EquipButton->OnClicked.RemoveDynamic(this, &USFInventoryEntryWidget::HandleEquipButtonClicked);
	}

	Super::NativeDestruct();
}

void USFInventoryEntryWidget::SetInventoryEntry(const FSFInventoryDisplayEntry& InEntry)
{
	CurrentEntry = InEntry;

	RefreshVisuals();
	RefreshSelectionVisuals();
	RefreshActionState();

	BP_OnEntrySet(CurrentEntry);
}

void USFInventoryEntryWidget::SetIsSelected(bool bInIsSelected)
{
	if (bIsSelected == bInIsSelected)
	{
		return;
	}

	bIsSelected = bInIsSelected;

	RefreshSelectionVisuals();
	BP_OnSelectionChanged(bIsSelected);
}

FReply USFInventoryEntryWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (CurrentEntry.IsValid() && !CurrentEntry.bIsEquipped)
	{
		OnEntryEquipRequested.Broadcast(CurrentEntry.InventoryEntryGuid);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

void USFInventoryEntryWidget::HandleEntryButtonClicked()
{
	if (!CurrentEntry.IsValid())
	{
		return;
	}

	OnEntryClicked.Broadcast(CurrentEntry.InventoryEntryGuid);
}

void USFInventoryEntryWidget::HandleEquipButtonClicked()
{
	if (!CurrentEntry.IsValid() || CurrentEntry.bIsEquipped)
	{
		return;
	}

	OnEntryEquipRequested.Broadcast(CurrentEntry.InventoryEntryGuid);
}

void USFInventoryEntryWidget::RefreshVisuals()
{
	// Icon
	if (ItemIconImage)
	{
		if (CurrentEntry.Icon)
		{
			ItemIconImage->SetBrushFromTexture(CurrentEntry.Icon, true);
			ItemIconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			ItemIconImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// Quantity overlay
	if (QuantityText)
	{
		QuantityText->SetText(BuildQuantityText());
		QuantityText->SetVisibility(CurrentEntry.IsValid() && CurrentEntry.Quantity > 1
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	// Equipped overlay
	if (EquippedText)
	{
		EquippedText->SetVisibility(CurrentEntry.bIsEquipped
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);
		EquippedText->SetText(LOCTEXT("EquippedStateLabel", "E"));
	}

	// Optional equip button label
	if (EquipButtonText)
	{
		EquipButtonText->SetText(LOCTEXT("EquipButtonLabel", "Equip"));
	}

	// Slot frame sizing
	if (IconSizeBox)
	{
		if (ShouldUseTallWeaponFrame())
		{
			IconSizeBox->SetWidthOverride(SlotIconSize);
			IconSizeBox->SetHeightOverride(WeaponIconHeight);
		}
		else
		{
			IconSizeBox->SetWidthOverride(SlotIconSize);
			IconSizeBox->SetHeightOverride(SlotIconSize);
		}
	}
}

void USFInventoryEntryWidget::RefreshSelectionVisuals()
{
	if (SelectionBorder)
	{
		SelectionBorder->SetVisibility(bIsSelected ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}

	if (RootBorder)
	{
		RootBorder->SetRenderOpacity(bIsSelected ? 1.0f : 0.9f);
	}
}

void USFInventoryEntryWidget::RefreshActionState()
{
	const bool bCanEquip = CurrentEntry.IsValid() && !CurrentEntry.bIsEquipped;

	if (EquipButton)
	{
		EquipButton->SetIsEnabled(bCanEquip);
		EquipButton->SetVisibility(bCanEquip ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

bool USFInventoryEntryWidget::ShouldUseTallWeaponFrame() const
{
	if (!CurrentEntry.ItemDefinition)
	{
		return false;
	}

	return CurrentEntry.ItemDefinition->ItemType == ESFItemType::Weapon;
}

FText USFInventoryEntryWidget::BuildQuantityText() const
{
	if (!CurrentEntry.IsValid())
	{
		return FText::GetEmpty();
	}

	// Unique or single items just show no quantity.
	if (!CurrentEntry.bIsStackable && CurrentEntry.Quantity <= 1)
	{
		return FText::GetEmpty();
	}

	return FText::AsNumber(CurrentEntry.Quantity);
}

#undef LOCTEXT_NAMESPACE