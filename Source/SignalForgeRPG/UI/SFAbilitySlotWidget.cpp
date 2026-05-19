#include "UI/SFAbilitySlotWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void USFAbilitySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (HotkeyText && !HotkeyLabel.IsEmpty())
	{
		HotkeyText->SetText(HotkeyLabel);
	}

	bLastReady = CurrentData.bIsReady;
	ApplyVisuals();
}

void USFAbilitySlotWidget::SetInputTag(FGameplayTag InTag)
{
	InputTag = InTag;
	CurrentData.InputTag = InTag;
	ApplyVisuals();
}

void USFAbilitySlotWidget::SetHotkeyLabel(const FText& InLabel)
{
	HotkeyLabel = InLabel;
	if (HotkeyText)
	{
		HotkeyText->SetText(HotkeyLabel);
	}
}

void USFAbilitySlotWidget::SetSlotData(const FSFAbilitySlotUIData& InData)
{
	const bool bReadyChanged = (CurrentData.bIsReady != InData.bIsReady);
	CurrentData = InData;

	// Preserve the layout-assigned InputTag if the incoming data omits it.
	if (!CurrentData.InputTag.IsValid())
	{
		CurrentData.InputTag = InputTag;
	}
	else
	{
		InputTag = CurrentData.InputTag;
	}

	ApplyVisuals();

	OnSlotDataChanged(CurrentData);
	if (bReadyChanged)
	{
		bLastReady = CurrentData.bIsReady;
		OnReadyStateChanged(CurrentData.bIsReady);
	}
}

void USFAbilitySlotWidget::ApplyVisuals()
{
	const bool bHas = CurrentData.bHasAbility;

	if (IconImage)
	{
		if (bHas && CurrentData.Icon)
		{
			IconImage->SetBrushFromTexture(CurrentData.Icon);
			IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			IconImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (EmptyImage)
	{
		EmptyImage->SetVisibility(bHas ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	if (CooldownOverlay)
	{
		const bool bShowCooldown = bHas && !CurrentData.bIsReady;
		CooldownOverlay->SetVisibility(bShowCooldown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (bHideWhenEmpty)
	{
		SetVisibility(bHas ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}
