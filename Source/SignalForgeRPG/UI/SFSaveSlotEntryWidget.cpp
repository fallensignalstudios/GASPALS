#include "UI/SFSaveSlotEntryWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Internationalization/Text.h"

#define LOCTEXT_NAMESPACE "SFSaveSlotEntry"

void USFSaveSlotEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (RowButton)
	{
		RowButton->OnClicked.AddUniqueDynamic(this, &USFSaveSlotEntryWidget::HandleRowButtonClicked);
	}
	if (LoadButton)
	{
		LoadButton->OnClicked.AddUniqueDynamic(this, &USFSaveSlotEntryWidget::HandleLoadButtonClicked);
	}
	if (SaveButton)
	{
		SaveButton->OnClicked.AddUniqueDynamic(this, &USFSaveSlotEntryWidget::HandleSaveButtonClicked);
	}
	if (DeleteButton)
	{
		DeleteButton->OnClicked.AddUniqueDynamic(this, &USFSaveSlotEntryWidget::HandleDeleteButtonClicked);
	}

	RefreshVisuals();
}

void USFSaveSlotEntryWidget::NativeDestruct()
{
	if (RowButton)
	{
		RowButton->OnClicked.RemoveDynamic(this, &USFSaveSlotEntryWidget::HandleRowButtonClicked);
	}
	if (LoadButton)
	{
		LoadButton->OnClicked.RemoveDynamic(this, &USFSaveSlotEntryWidget::HandleLoadButtonClicked);
	}
	if (SaveButton)
	{
		SaveButton->OnClicked.RemoveDynamic(this, &USFSaveSlotEntryWidget::HandleSaveButtonClicked);
	}
	if (DeleteButton)
	{
		DeleteButton->OnClicked.RemoveDynamic(this, &USFSaveSlotEntryWidget::HandleDeleteButtonClicked);
	}

	Super::NativeDestruct();
}

void USFSaveSlotEntryWidget::SetSlotInfo(const FSFPlayerSaveSlotInfo& InInfo)
{
	CurrentInfo = InInfo;
	RefreshVisuals();
	BP_OnSlotInfoSet(CurrentInfo);
}

void USFSaveSlotEntryWidget::SetIsSelected(bool bInSelected)
{
	if (bIsSelected == bInSelected)
	{
		return;
	}
	bIsSelected = bInSelected;
	BP_OnSelectionChanged(bIsSelected);
}

void USFSaveSlotEntryWidget::RefreshVisuals()
{
	if (SlotNameText)
	{
		SlotNameText->SetText(FText::FromString(CurrentInfo.SlotName));
	}

	if (FriendlyNameText)
	{
		const FString& Friendly = CurrentInfo.FriendlyName.IsEmpty() ? CurrentInfo.SlotName : CurrentInfo.FriendlyName;
		FriendlyNameText->SetText(FText::FromString(Friendly));
	}

	if (TimestampText)
	{
		// Local time for display so the user sees their own clock, not UTC.
		const FDateTime Local = CurrentInfo.SaveTimestamp + (FDateTime::Now() - FDateTime::UtcNow());
		TimestampText->SetText(FText::AsDateTime(Local));
	}

	if (LevelText)
	{
		LevelText->SetText(FText::Format(
			LOCTEXT("LevelFmt", "Lv. {0}  ({1} XP)"),
			FText::AsNumber(CurrentInfo.PlayerLevel),
			FText::AsNumber(CurrentInfo.PlayerXP)));
	}

	if (LevelNameText)
	{
		LevelNameText->SetText(FText::FromName(CurrentInfo.LevelName));
	}

	if (PlaytimeText)
	{
		PlaytimeText->SetText(FormatPlaytime(CurrentInfo.AccumulatedPlaytimeSeconds));
	}

	if (StatusText)
	{
		StatusText->SetText(CurrentInfo.bIsValid
			? FText::Format(LOCTEXT("OkSchema", "Schema v{0}"), FText::AsNumber(CurrentInfo.SchemaVersion))
			: LOCTEXT("Corrupt", "<corrupt or missing>"));
	}

	// Disable interaction buttons when the slot is unreadable.
	const bool bInteractable = CurrentInfo.bIsValid;
	if (LoadButton)
	{
		LoadButton->SetIsEnabled(bInteractable);
	}
	// SaveButton is "overwrite this slot" -- always allowed (the act of
	// overwriting fixes a corrupt slot). DeleteButton too.
}

FText USFSaveSlotEntryWidget::FormatPlaytime(float Seconds)
{
	const int32 Total = FMath::Max(0, FMath::FloorToInt(Seconds));
	const int32 Hours = Total / 3600;
	const int32 Minutes = (Total % 3600) / 60;
	const int32 Secs = Total % 60;

	if (Hours > 0)
	{
		return FText::Format(LOCTEXT("PlaytimeHMS", "{0}:{1}:{2}"),
			FText::AsNumber(Hours),
			FText::AsNumber(Minutes),
			FText::AsNumber(Secs));
	}
	return FText::Format(LOCTEXT("PlaytimeMS", "{0}:{1}"),
		FText::AsNumber(Minutes),
		FText::AsNumber(Secs));
}

void USFSaveSlotEntryWidget::HandleRowButtonClicked()
{
	OnRowClicked.Broadcast(CurrentInfo.SlotName);
}

void USFSaveSlotEntryWidget::HandleLoadButtonClicked()
{
	OnLoadClicked.Broadcast(CurrentInfo.SlotName);
}

void USFSaveSlotEntryWidget::HandleSaveButtonClicked()
{
	OnSaveClicked.Broadcast(CurrentInfo.SlotName);
}

void USFSaveSlotEntryWidget::HandleDeleteButtonClicked()
{
	OnDeleteClicked.Broadcast(CurrentInfo.SlotName);
}

#undef LOCTEXT_NAMESPACE
