#include "UI/SFPlayerInventoryWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/SFInventoryComponent.h"
#include "Components/SFEquipmentComponent.h"
#include "Inventory/SFItemDefinition.h"
#include "UI/SFInventoryEntryWidget.h"

#define LOCTEXT_NAMESPACE "SFPlayerInventoryWidget"

void USFPlayerInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetController)
	{
		WidgetController = NewObject<USFInventoryWidgetController>(this, TEXT("InventoryWidgetController"));
	}

	BuildWidgetLayout();

	if (EquipButton && !EquipButton->OnClicked.IsAlreadyBound(this, &USFPlayerInventoryWidget::HandleEquipButtonClicked))
	{
		EquipButton->OnClicked.AddDynamic(this, &USFPlayerInventoryWidget::HandleEquipButtonClicked);
	}

	if (SortButton && !SortButton->OnClicked.IsAlreadyBound(this, &USFPlayerInventoryWidget::HandleSortButtonClicked))
	{
		SortButton->OnClicked.AddDynamic(this, &USFPlayerInventoryWidget::HandleSortButtonClicked);
	}

	if (FilterButton && !FilterButton->OnClicked.IsAlreadyBound(this, &USFPlayerInventoryWidget::HandleFilterButtonClicked))
	{
		FilterButton->OnClicked.AddDynamic(this, &USFPlayerInventoryWidget::HandleFilterButtonClicked);
	}

	BindToController();
	UpdateButtonLabels();
	RefreshSelectedDetails();
	RefreshActionStates();
}

void USFPlayerInventoryWidget::NativeDestruct()
{
	DeinitializeInventoryWidget();
	UnbindFromController();

	if (EquipButton)
	{
		EquipButton->OnClicked.RemoveDynamic(this, &USFPlayerInventoryWidget::HandleEquipButtonClicked);
	}

	if (SortButton)
	{
		SortButton->OnClicked.RemoveDynamic(this, &USFPlayerInventoryWidget::HandleSortButtonClicked);
	}

	if (FilterButton)
	{
		FilterButton->OnClicked.RemoveDynamic(this, &USFPlayerInventoryWidget::HandleFilterButtonClicked);
	}

	Super::NativeDestruct();
}

void USFPlayerInventoryWidget::InitializeInventoryWidget(USFInventoryComponent* InInventoryComponent, USFEquipmentComponent* InEquipmentComponent)
{
	if (!WidgetController)
	{
		WidgetController = NewObject<USFInventoryWidgetController>(this, TEXT("InventoryWidgetController"));
	}

	BindToController();

	if (WidgetController)
	{
		WidgetController->Initialize(InInventoryComponent, InEquipmentComponent);
		WidgetController->SetShowOnlyEquippable(false);
		WidgetController->RefreshInventoryDisplayData();
	}

	UpdateButtonLabels();
	RefreshSelectedDetails();
	RefreshActionStates();
}

void USFPlayerInventoryWidget::DeinitializeInventoryWidget()
{
	ClearEntryWidgets();
	SelectedEntryId.Invalidate();

	if (WidgetController)
	{
		UnbindFromController();
		WidgetController->Deinitialize();
	}

	RefreshSelectedDetails();
	RefreshActionStates();
	UpdateButtonLabels();
}

void USFPlayerInventoryWidget::BindToController()
{
	if (!WidgetController)
	{
		return;
	}

	if (!WidgetController->OnInventoryDisplayDataUpdated.IsAlreadyBound(this, &USFPlayerInventoryWidget::HandleInventoryDisplayDataUpdated))
	{
		WidgetController->OnInventoryDisplayDataUpdated.AddDynamic(this, &USFPlayerInventoryWidget::HandleInventoryDisplayDataUpdated);
	}

	if (!WidgetController->OnInventorySelectionChanged.IsAlreadyBound(this, &USFPlayerInventoryWidget::HandleInventorySelectionChanged))
	{
		WidgetController->OnInventorySelectionChanged.AddDynamic(this, &USFPlayerInventoryWidget::HandleInventorySelectionChanged);
	}

	if (!WidgetController->OnInventoryEquipRequested.IsAlreadyBound(this, &USFPlayerInventoryWidget::HandleEquipRequested))
	{
		WidgetController->OnInventoryEquipRequested.AddDynamic(this, &USFPlayerInventoryWidget::HandleEquipRequested);
	}
}

void USFPlayerInventoryWidget::UnbindFromController()
{
	if (!WidgetController)
	{
		return;
	}

	WidgetController->OnInventoryDisplayDataUpdated.RemoveDynamic(this, &USFPlayerInventoryWidget::HandleInventoryDisplayDataUpdated);
	WidgetController->OnInventorySelectionChanged.RemoveDynamic(this, &USFPlayerInventoryWidget::HandleInventorySelectionChanged);
	WidgetController->OnInventoryEquipRequested.RemoveDynamic(this, &USFPlayerInventoryWidget::HandleEquipRequested);
}

void USFPlayerInventoryWidget::HandleInventoryDisplayDataUpdated(const TArray<FSFInventoryDisplayEntry>& UpdatedEntries)
{
	const int32 SelectedIndex = WidgetController ? WidgetController->GetSelectedIndex() : INDEX_NONE;
	if (UpdatedEntries.IsValidIndex(SelectedIndex))
	{
		SelectedEntryId = UpdatedEntries[SelectedIndex].InventoryEntryGuid;
	}
	else
	{
		SelectedEntryId.Invalidate();
	}

	RebuildEntryWidgets();

	if (EmptyStateText)
	{
		const bool bIsEmpty = UpdatedEntries.IsEmpty();
		EmptyStateText->SetVisibility(bIsEmpty ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		EmptyStateText->SetText(
			bIsEmpty
			? LOCTEXT("InventoryEmptyState", "No unequipped items available.")
			: FText::GetEmpty());
		BP_OnEmptyStateChanged(bIsEmpty);
	}

	RefreshSelectedDetails();
	RefreshActionStates();
	UpdateButtonLabels();
	BP_OnInventoryEntriesRebuilt();
}

void USFPlayerInventoryWidget::HandleInventorySelectionChanged(int32 NewSelectedIndex)
{
	const FSFInventoryDisplayEntry* SelectedEntry = GetSelectedDisplayEntry();
	SelectedEntryId = SelectedEntry ? SelectedEntry->InventoryEntryGuid : FGuid();

	for (USFInventoryEntryWidget* EntryWidget : SpawnedEntryWidgets)
	{
		if (!EntryWidget)
		{
			continue;
		}

		EntryWidget->SetIsSelected(
			SelectedEntry && EntryWidget->GetInventoryEntry().InventoryEntryGuid == SelectedEntry->InventoryEntryGuid);
	}

	RefreshSelectedDetails();
	RefreshActionStates();

	if (SelectedEntry)
	{
		BP_OnSelectionChanged(*SelectedEntry, true);
	}
	else
	{
		BP_OnSelectionChanged(FSFInventoryDisplayEntry(), false);
	}
}

void USFPlayerInventoryWidget::HandleEquipRequested(ESFInventoryWidgetOpResult Result)
{
	RefreshSelectedDetails();
	RefreshActionStates();
	BP_OnEquipRequestCompleted(Result);
}

void USFPlayerInventoryWidget::HandleEquipButtonClicked()
{
	if (WidgetController)
	{
		WidgetController->EquipSelectedItem();
	}
}

void USFPlayerInventoryWidget::HandleSortButtonClicked()
{
	if (WidgetController)
	{
		WidgetController->CycleSortMode();
	}

	UpdateButtonLabels();
	RefreshActionStates();
}

void USFPlayerInventoryWidget::HandleFilterButtonClicked()
{
	if (WidgetController)
	{
		WidgetController->ToggleEquippableFilter();
	}

	UpdateButtonLabels();
	RefreshActionStates();
}

void USFPlayerInventoryWidget::HandleEntryWidgetClicked(FGuid EntryGuid)
{
	if (!WidgetController || !EntryGuid.IsValid())
	{
		return;
	}

	const TArray<FSFInventoryDisplayEntry>& Entries = WidgetController->GetCurrentDisplayEntries();
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		if (Entries[Index].InventoryEntryGuid == EntryGuid)
		{
			WidgetController->SelectIndex(Index);
			return;
		}
	}
}

void USFPlayerInventoryWidget::BuildWidgetLayout()
{
}

void USFPlayerInventoryWidget::RefreshSelectedDetails()
{
	const FSFInventoryDisplayEntry* SelectedEntry = GetSelectedDisplayEntry();

	if (SelectedItemNameText)
	{
		SelectedItemNameText->SetText(
			SelectedEntry ? SelectedEntry->DisplayName : LOCTEXT("NoSelectionName", "No Item Selected"));
	}

	if (SelectedItemMetaText)
	{
		SelectedItemMetaText->SetText(BuildSelectedMetaText(SelectedEntry));
	}

	if (SelectedItemDescriptionText)
	{
		SelectedItemDescriptionText->SetText(BuildSelectedDescriptionText(SelectedEntry));
	}
}

void USFPlayerInventoryWidget::RefreshActionStates()
{
	const FSFInventoryDisplayEntry* SelectedEntry = GetSelectedDisplayEntry();
	const bool bCanEquip = (SelectedEntry != nullptr) && !SelectedEntry->bIsEquipped;

	if (EquipButton)
	{
		EquipButton->SetIsEnabled(bCanEquip);
	}
}

void USFPlayerInventoryWidget::UpdateButtonLabels()
{
	if (EquipButtonText)
	{
		EquipButtonText->SetText(LOCTEXT("EquipButtonLabel", "Equip"));
	}

	if (SortButtonText)
	{
		SortButtonText->SetText(BuildSortLabelText());
	}

	if (FilterButtonText)
	{
		FilterButtonText->SetText(BuildFilterLabelText());
	}
}

const FSFInventoryDisplayEntry* USFPlayerInventoryWidget::GetSelectedDisplayEntry() const
{
	if (!WidgetController || !WidgetController->HasValidSelection())
	{
		return nullptr;
	}

	const TArray<FSFInventoryDisplayEntry>& Entries = WidgetController->GetCurrentDisplayEntries();
	const int32 SelectedIndex = WidgetController->GetSelectedIndex();

	return Entries.IsValidIndex(SelectedIndex) ? &Entries[SelectedIndex] : nullptr;
}

void USFPlayerInventoryWidget::RebuildEntryWidgets()
{
	ClearEntryWidgets();

	if (!EntryContainer || !WidgetController || !InventoryEntryWidgetClass)
	{
		return;
	}

	const int32 SafeColumns = FMath::Max(1, GridColumns);

	// 1) Get display entries
	const TArray<FSFInventoryDisplayEntry>& Entries = WidgetController->GetCurrentDisplayEntries();
	const int32 RealEntryCount = Entries.Num();

	// 2) Get capacity from the backing inventory
	int32 MaxSlots = 0;

	if (USFInventoryComponent* InventoryComp = WidgetController->GetInventoryComponent())
	{
		// Either expose MaxInventorySlots directly, or add a getter on the component.
		MaxSlots = InventoryComp->GetRemainingSlotCapacity() + InventoryComp->GetInventoryEntryCount();
		// Or, if you expose MaxInventorySlots:
		// MaxSlots = InventoryComp->GetMaxInventorySlots();
	}

	// Fallback: if we couldn't get a capacity, just show real entries.
	if (MaxSlots <= 0)
	{
		MaxSlots = RealEntryCount;
	}

	// Clamp so we never show fewer slots than items we actually have.
	MaxSlots = FMath::Max(MaxSlots, RealEntryCount);

	// 3) Spawn widgets for real entries first
	int32 CurrentIndex = 0;

	for (; CurrentIndex < RealEntryCount; ++CurrentIndex)
	{
		const FSFInventoryDisplayEntry& Entry = Entries[CurrentIndex];

		USFInventoryEntryWidget* EntryWidget =
			CreateWidget<USFInventoryEntryWidget>(this, InventoryEntryWidgetClass);

		if (!EntryWidget)
		{
			continue;
		}

		EntryWidget->SetInventoryEntry(Entry);
		EntryWidget->SetIsSelected(Entry.InventoryEntryGuid == SelectedEntryId);

		if (!EntryWidget->OnEntryClicked.IsAlreadyBound(this, &USFPlayerInventoryWidget::HandleEntryWidgetClicked))
		{
			EntryWidget->OnEntryClicked.AddDynamic(this, &USFPlayerInventoryWidget::HandleEntryWidgetClicked);
		}

		if (!EntryWidget->OnEntryEquipRequested.IsAlreadyBound(this, &USFPlayerInventoryWidget::HandleEntryWidgetEquipRequested))
		{
			EntryWidget->OnEntryEquipRequested.AddDynamic(this, &USFPlayerInventoryWidget::HandleEntryWidgetEquipRequested);
		}

		SpawnedEntryWidgets.Add(EntryWidget);

		const int32 Row = CurrentIndex / SafeColumns;
		const int32 Column = CurrentIndex % SafeColumns;

		if (UUniformGridSlot* GridSlot = EntryContainer->AddChildToUniformGrid(EntryWidget, Row, Column))
		{
			GridSlot->SetHorizontalAlignment(HAlign_Fill);
			GridSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	// 4) Fill remaining capacity with empty slots
	for (; CurrentIndex < MaxSlots; ++CurrentIndex)
	{
		USFInventoryEntryWidget* EmptyWidget =
			CreateWidget<USFInventoryEntryWidget>(this, InventoryEntryWidgetClass);

		if (!EmptyWidget)
		{
			continue;
		}

		// Build a "blank" display entry so the tile shows as empty and never fires real actions.
		FSFInventoryDisplayEntry EmptyEntry;
		EmptyEntry.ItemDefinition = nullptr;
		EmptyEntry.DisplayName = FText::GetEmpty();
		EmptyEntry.Quantity = 0;
		EmptyEntry.bIsEquipped = false;
		EmptyEntry.bIsStackable = false;
		EmptyEntry.ItemId = NAME_None;
		EmptyEntry.InventoryEntryGuid.Invalidate();
		EmptyEntry.SourceInventoryIndex = INDEX_NONE;
		EmptyEntry.Icon = nullptr;

		EmptyWidget->SetInventoryEntry(EmptyEntry);
		EmptyWidget->SetIsSelected(false);

		// Do NOT bind click/equip delegates for empty slots
		// so clicking them is a no-op (or you can handle it in BP).

		SpawnedEntryWidgets.Add(EmptyWidget);

		const int32 Row = CurrentIndex / SafeColumns;
		const int32 Column = CurrentIndex % SafeColumns;

		if (UUniformGridSlot* GridSlot = EntryContainer->AddChildToUniformGrid(EmptyWidget, Row, Column))
		{
			GridSlot->SetHorizontalAlignment(HAlign_Fill);
			GridSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}
}

void USFPlayerInventoryWidget::ClearEntryWidgets()
{
	for (USFInventoryEntryWidget* EntryWidget : SpawnedEntryWidgets)
	{
		if (!EntryWidget)
		{
			continue;
		}

		EntryWidget->OnEntryClicked.RemoveDynamic(this, &USFPlayerInventoryWidget::HandleEntryWidgetClicked);
		EntryWidget->OnEntryEquipRequested.RemoveDynamic(this, &USFPlayerInventoryWidget::HandleEntryWidgetEquipRequested);
		EntryWidget->RemoveFromParent();
	}

	SpawnedEntryWidgets.Reset();

	if (EntryContainer)
	{
		EntryContainer->ClearChildren();
	}
}

FText USFPlayerInventoryWidget::BuildSelectedMetaText(const FSFInventoryDisplayEntry* SelectedEntry) const
{
	if (!SelectedEntry || !SelectedEntry->ItemDefinition)
	{
		return LOCTEXT("NoSelectionMeta", "Select an item to inspect its details.");
	}

	FFormatNamedArguments Args;
	Args.Add(TEXT("Quantity"), FText::AsNumber(SelectedEntry->Quantity));
	Args.Add(TEXT("Type"), FText::FromName(SelectedEntry->ItemId));

	return FText::Format(
		LOCTEXT("SelectedMetaFormat", "Quantity: {Quantity}  •  Id: {Type}"),
		Args);
}

FText USFPlayerInventoryWidget::BuildSelectedDescriptionText(const FSFInventoryDisplayEntry* SelectedEntry) const
{
	if (!SelectedEntry || !SelectedEntry->ItemDefinition)
	{
		return FText::GetEmpty();
	}

	return SelectedEntry->ItemDefinition->Description;
}

FText USFPlayerInventoryWidget::BuildSortLabelText() const
{
	if (!WidgetController)
	{
		return LOCTEXT("SortDefaultLabel", "Sort: Default");
	}

	switch (WidgetController->GetSortMode())
	{
	case ESFInventoryDisplaySortMode::NameAscending:
		return LOCTEXT("SortNameAscLabel", "Sort: Name A-Z");

	case ESFInventoryDisplaySortMode::NameDescending:
		return LOCTEXT("SortNameDescLabel", "Sort: Name Z-A");

	case ESFInventoryDisplaySortMode::QuantityDescending:
		return LOCTEXT("SortQtyDescLabel", "Sort: Qty High-Low");

	case ESFInventoryDisplaySortMode::QuantityAscending:
		return LOCTEXT("SortQtyAscLabel", "Sort: Qty Low-High");

	case ESFInventoryDisplaySortMode::Default:
	default:
		return LOCTEXT("SortDefaultActiveLabel", "Sort: Default");
	}
}

FText USFPlayerInventoryWidget::BuildFilterLabelText() const
{
	if (!WidgetController)
	{
		return LOCTEXT("FilterAllLabel", "Filter: All");
	}

	return WidgetController->GetShowOnlyEquippable()
		? LOCTEXT("FilterEquippableLabel", "Filter: Equippable")
		: LOCTEXT("FilterAllActiveLabel", "Filter: All");
}

void USFPlayerInventoryWidget::HandleEntryWidgetEquipRequested(FGuid EntryGuid)
{
	if (!WidgetController || !EntryGuid.IsValid())
	{
		return;
	}

	const TArray<FSFInventoryDisplayEntry>& Entries = WidgetController->GetCurrentDisplayEntries();
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		if (Entries[Index].InventoryEntryGuid == EntryGuid)
		{
			WidgetController->SelectIndex(Index);
			WidgetController->EquipItemAtIndex(Index);
			return;
		}
	}
}

#undef LOCTEXT_NAMESPACE