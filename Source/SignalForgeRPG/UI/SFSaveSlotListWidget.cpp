#include "UI/SFSaveSlotListWidget.h"

#include "UI/SFSaveSlotEntryWidget.h"
#include "Save/SFPlayerSaveService.h"

#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Algo/Sort.h"

#define LOCTEXT_NAMESPACE "SFSaveSlotList"

void USFSaveSlotListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (RefreshButton)
	{
		RefreshButton->OnClicked.AddUniqueDynamic(this, &USFSaveSlotListWidget::HandleRefreshButtonClicked);
	}
	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &USFSaveSlotListWidget::HandleCloseButtonClicked);
	}
	if (NewSaveButton)
	{
		NewSaveButton->OnClicked.AddUniqueDynamic(this, &USFSaveSlotListWidget::HandleNewSaveButtonClicked);
	}

	BindToService();
	RefreshSlotList();
}

void USFSaveSlotListWidget::NativeDestruct()
{
	if (RefreshButton)
	{
		RefreshButton->OnClicked.RemoveDynamic(this, &USFSaveSlotListWidget::HandleRefreshButtonClicked);
	}
	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &USFSaveSlotListWidget::HandleCloseButtonClicked);
	}
	if (NewSaveButton)
	{
		NewSaveButton->OnClicked.RemoveDynamic(this, &USFSaveSlotListWidget::HandleNewSaveButtonClicked);
	}

	UnbindFromService();
	Super::NativeDestruct();
}

USFPlayerSaveService* USFSaveSlotListWidget::GetSaveService() const
{
	if (CachedService)
	{
		return CachedService;
	}
	if (UGameInstance* GI = GetGameInstance())
	{
		// const_cast is fine here -- the cache field is mutable in intent.
		const_cast<USFSaveSlotListWidget*>(this)->CachedService = GI->GetSubsystem<USFPlayerSaveService>();
	}
	return CachedService;
}

// =============================================================================
// Public actions
// =============================================================================

void USFSaveSlotListWidget::RefreshSlotList()
{
	USFPlayerSaveService* Service = GetSaveService();
	if (!Service)
	{
		UpdateStatus(LOCTEXT("NoService", "Save service unavailable."));
		RebuildRows({});
		return;
	}

	TArray<FSFPlayerSaveSlotInfo> Infos = Service->GetAllSlotInfos(UserIndex);
	ApplySort(Infos);
	RebuildRows(Infos);
}

bool USFSaveSlotListWidget::LoadSlot(const FString& SlotName)
{
	USFPlayerSaveService* Service = GetSaveService();
	if (!Service || SlotName.IsEmpty())
	{
		return false;
	}
	UpdateStatus(FText::Format(LOCTEXT("Loading", "Loading {0}..."), FText::FromString(SlotName)));
	return Service->LoadFromSlot(SlotName, nullptr, UserIndex);
}

bool USFSaveSlotListWidget::SaveSlot(const FString& SlotName)
{
	USFPlayerSaveService* Service = GetSaveService();
	if (!Service)
	{
		return false;
	}
	const FString Target = SlotName.IsEmpty() ? SelectedSlotName : SlotName;
	if (Target.IsEmpty())
	{
		UpdateStatus(LOCTEXT("NoTarget", "No slot selected."));
		return false;
	}
	UpdateStatus(FText::Format(LOCTEXT("Saving", "Saving to {0}..."), FText::FromString(Target)));
	return Service->SaveToSlot(Target, nullptr, UserIndex);
}

bool USFSaveSlotListWidget::DeleteSlot(const FString& SlotName)
{
	USFPlayerSaveService* Service = GetSaveService();
	if (!Service || SlotName.IsEmpty())
	{
		return false;
	}
	const bool bDeleted = Service->DeleteSlot(SlotName, UserIndex);
	UpdateStatus(bDeleted
		? FText::Format(LOCTEXT("Deleted", "Deleted {0}."), FText::FromString(SlotName))
		: FText::Format(LOCTEXT("DeleteFailed", "Could not delete {0}."), FText::FromString(SlotName)));
	return bDeleted;
}

bool USFSaveSlotListWidget::SaveToNewSlotFromInput()
{
	if (!NewSlotNameInput)
	{
		return false;
	}
	const FString Typed = NewSlotNameInput->GetText().ToString().TrimStartAndEnd();
	if (Typed.IsEmpty())
	{
		UpdateStatus(LOCTEXT("NeedName", "Type a slot name first."));
		return false;
	}

	// Don't let user collide with the manifest's reserved name. The service
	// will also refuse this, but failing fast in the UI is friendlier.
	if (Typed.StartsWith(TEXT("_")))
	{
		UpdateStatus(LOCTEXT("ReservedName", "Slot names can't start with '_'."));
		return false;
	}

	return SaveSlot(Typed);
}

void USFSaveSlotListWidget::SetSelectedSlotName(const FString& InSlotName)
{
	if (SelectedSlotName == InSlotName)
	{
		return;
	}
	SelectedSlotName = InSlotName;
	for (USFSaveSlotEntryWidget* Row : SpawnedRows)
	{
		if (Row)
		{
			Row->SetIsSelected(Row->GetSlotName() == SelectedSlotName);
		}
	}
	OnSelectionChanged.Broadcast(SelectedSlotName);
	BP_OnSelectionChanged(SelectedSlotName);
}

void USFSaveSlotListWidget::SetSortMode(ESFSaveSlotListSortMode InMode)
{
	if (SortMode == InMode)
	{
		return;
	}
	SortMode = InMode;
	RefreshSlotList();
}

// =============================================================================
// Service event bindings
// =============================================================================

void USFSaveSlotListWidget::BindToService()
{
	if (USFPlayerSaveService* Service = GetSaveService())
	{
		Service->OnSlotListChanged.AddUniqueDynamic(this, &USFSaveSlotListWidget::HandleServiceSlotListChanged);
		Service->OnAfterSave.AddUniqueDynamic(this, &USFSaveSlotListWidget::HandleServiceAfterSave);
		Service->OnAfterLoad.AddUniqueDynamic(this, &USFSaveSlotListWidget::HandleServiceAfterLoad);
	}
}

void USFSaveSlotListWidget::UnbindFromService()
{
	if (USFPlayerSaveService* Service = GetSaveService())
	{
		Service->OnSlotListChanged.RemoveDynamic(this, &USFSaveSlotListWidget::HandleServiceSlotListChanged);
		Service->OnAfterSave.RemoveDynamic(this, &USFSaveSlotListWidget::HandleServiceAfterSave);
		Service->OnAfterLoad.RemoveDynamic(this, &USFSaveSlotListWidget::HandleServiceAfterLoad);
	}
}

void USFSaveSlotListWidget::HandleServiceSlotListChanged(const FString& /*SlotName*/)
{
	RefreshSlotList();
}

void USFSaveSlotListWidget::HandleServiceAfterSave(const FString& SlotName, bool bSuccess)
{
	UpdateStatus(bSuccess
		? FText::Format(LOCTEXT("SaveOk", "Saved to {0}."), FText::FromString(SlotName))
		: FText::Format(LOCTEXT("SaveFail", "Failed to save {0}."), FText::FromString(SlotName)));
	BP_OnSaveResult(SlotName, bSuccess);
	// Manifest registration already triggered OnSlotListChanged -> RefreshSlotList,
	// but for an overwrite of an existing slot the manifest is unchanged, so refresh
	// here too to pick up the new timestamp.
	if (bSuccess)
	{
		RefreshSlotList();
	}
}

void USFSaveSlotListWidget::HandleServiceAfterLoad(const FString& SlotName, bool bSuccess)
{
	UpdateStatus(bSuccess
		? FText::Format(LOCTEXT("LoadOk", "Loaded {0}."), FText::FromString(SlotName))
		: FText::Format(LOCTEXT("LoadFail", "Failed to load {0}."), FText::FromString(SlotName)));
	BP_OnLoadResult(SlotName, bSuccess);
	if (bSuccess)
	{
		OnSlotLoaded.Broadcast(SlotName);
	}
}

// =============================================================================
// Row handlers
// =============================================================================

void USFSaveSlotListWidget::HandleRowClicked(const FString& SlotName)
{
	SetSelectedSlotName(SlotName);
}

void USFSaveSlotListWidget::HandleRowLoadClicked(const FString& SlotName)
{
	SetSelectedSlotName(SlotName);
	LoadSlot(SlotName);
}

void USFSaveSlotListWidget::HandleRowSaveClicked(const FString& SlotName)
{
	SetSelectedSlotName(SlotName);
	SaveSlot(SlotName);
}

void USFSaveSlotListWidget::HandleRowDeleteClicked(const FString& SlotName)
{
	DeleteSlot(SlotName);
}

// =============================================================================
// Top-level buttons
// =============================================================================

void USFSaveSlotListWidget::HandleRefreshButtonClicked()
{
	RefreshSlotList();
}

void USFSaveSlotListWidget::HandleCloseButtonClicked()
{
	BP_OnCloseRequested();
}

void USFSaveSlotListWidget::HandleNewSaveButtonClicked()
{
	SaveToNewSlotFromInput();
}

void USFSaveSlotListWidget::BP_OnCloseRequested_Implementation()
{
	// Default close behavior: remove from parent. BP children can override.
	RemoveFromParent();
}

// =============================================================================
// Rendering
// =============================================================================

void USFSaveSlotListWidget::RebuildRows(const TArray<FSFPlayerSaveSlotInfo>& Infos)
{
	// Tear down old rows
	for (USFSaveSlotEntryWidget* Row : SpawnedRows)
	{
		if (!Row)
		{
			continue;
		}
		Row->OnRowClicked.RemoveDynamic(this, &USFSaveSlotListWidget::HandleRowClicked);
		Row->OnLoadClicked.RemoveDynamic(this, &USFSaveSlotListWidget::HandleRowLoadClicked);
		Row->OnSaveClicked.RemoveDynamic(this, &USFSaveSlotListWidget::HandleRowSaveClicked);
		Row->OnDeleteClicked.RemoveDynamic(this, &USFSaveSlotListWidget::HandleRowDeleteClicked);
	}
	SpawnedRows.Reset();

	if (RowContainer)
	{
		RowContainer->ClearChildren();
	}

	if (!RowWidgetClass)
	{
		UpdateStatus(LOCTEXT("NoRowClass", "RowWidgetClass not set on USFSaveSlotListWidget."));
		UpdateEmptyState(0);
		BP_OnSlotsRebuilt(0);
		return;
	}

	if (!RowContainer)
	{
		// Without a container we can't actually render anything; BP author needs
		// to add a RowContainer to the WBP. Status text makes the failure visible.
		UpdateStatus(LOCTEXT("NoContainer", "Bind a RowContainer panel in the WBP."));
		UpdateEmptyState(0);
		BP_OnSlotsRebuilt(0);
		return;
	}

	SpawnedRows.Reserve(Infos.Num());
	for (const FSFPlayerSaveSlotInfo& Info : Infos)
	{
		USFSaveSlotEntryWidget* Row = CreateWidget<USFSaveSlotEntryWidget>(this, RowWidgetClass);
		if (!Row)
		{
			continue;
		}

		Row->OnRowClicked.AddUniqueDynamic(this, &USFSaveSlotListWidget::HandleRowClicked);
		Row->OnLoadClicked.AddUniqueDynamic(this, &USFSaveSlotListWidget::HandleRowLoadClicked);
		Row->OnSaveClicked.AddUniqueDynamic(this, &USFSaveSlotListWidget::HandleRowSaveClicked);
		Row->OnDeleteClicked.AddUniqueDynamic(this, &USFSaveSlotListWidget::HandleRowDeleteClicked);

		Row->SetSlotInfo(Info);
		Row->SetIsSelected(Info.SlotName == SelectedSlotName);

		RowContainer->AddChild(Row);
		SpawnedRows.Add(Row);
	}

	UpdateEmptyState(Infos.Num());
	BP_OnSlotsRebuilt(Infos.Num());
}

void USFSaveSlotListWidget::ApplySort(TArray<FSFPlayerSaveSlotInfo>& InOut) const
{
	switch (SortMode)
	{
	case ESFSaveSlotListSortMode::NewestFirst:
		Algo::Sort(InOut, [](const FSFPlayerSaveSlotInfo& A, const FSFPlayerSaveSlotInfo& B)
		{
			return A.SaveTimestamp > B.SaveTimestamp;
		});
		break;
	case ESFSaveSlotListSortMode::OldestFirst:
		Algo::Sort(InOut, [](const FSFPlayerSaveSlotInfo& A, const FSFPlayerSaveSlotInfo& B)
		{
			return A.SaveTimestamp < B.SaveTimestamp;
		});
		break;
	case ESFSaveSlotListSortMode::NameAscending:
		Algo::Sort(InOut, [](const FSFPlayerSaveSlotInfo& A, const FSFPlayerSaveSlotInfo& B)
		{
			return A.SlotName < B.SlotName;
		});
		break;
	}
}

void USFSaveSlotListWidget::UpdateEmptyState(int32 SlotCount)
{
	if (EmptyStateText)
	{
		EmptyStateText->SetVisibility(SlotCount == 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (SlotCount == 0 && EmptyStateText->GetText().IsEmpty())
		{
			EmptyStateText->SetText(LOCTEXT("NoSaves", "No saves yet."));
		}
	}
}

void USFSaveSlotListWidget::UpdateStatus(const FText& Message)
{
	if (StatusText)
	{
		StatusText->SetText(Message);
	}
}

#undef LOCTEXT_NAMESPACE
