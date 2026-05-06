#include "UI/SFCompanionRadialWheel.h"

#include "UI/SFCompanionRadialWheelEntry.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"

void USFCompanionRadialWheel::NativeConstruct()
{
	Super::NativeConstruct();
	RebuildEntries();
}

void USFCompanionRadialWheel::NativeDestruct()
{
	HoveredIndex = INDEX_NONE;
	Super::NativeDestruct();
}

void USFCompanionRadialWheel::SetSlots(const TArray<FSFRadialWheelSlot>& InSlots)
{
	Slots = InSlots;
	HoveredIndex = INDEX_NONE;
	RebuildEntries();
}

void USFCompanionRadialWheel::RebuildEntries()
{
	if (!SlotPanel)
	{
		// Designer hasn't bound the panel yet. The widget will rebuild on first NativeConstruct.
		return;
	}

	// Clear old entries.
	for (USFCompanionRadialWheelEntry* Entry : Entries)
	{
		if (Entry)
		{
			Entry->RemoveFromParent();
		}
	}
	Entries.Reset();
	SlotPanel->ClearChildren();

	if (!EntryWidgetClass || Slots.Num() == 0)
	{
		BP_OnSlotsRebuilt();
		return;
	}

	UWidgetTree* Tree = WidgetTree;
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		USFCompanionRadialWheelEntry* Entry = Tree
			? Tree->ConstructWidget<USFCompanionRadialWheelEntry>(EntryWidgetClass)
			: CreateWidget<USFCompanionRadialWheelEntry>(this, EntryWidgetClass);

		if (!Entry)
		{
			continue;
		}

		Entry->SetSlotIndex(i);
		Entry->SetSlotData(Slots[i]);
		Entry->SetHovered(false);
		SlotPanel->AddChild(Entry);
		Entries.Add(Entry);
	}

	LayoutEntries();
	BP_OnSlotsRebuilt();
}

void USFCompanionRadialWheel::LayoutEntries()
{
	UCanvasPanel* Canvas = Cast<UCanvasPanel>(SlotPanel);
	if (!Canvas)
	{
		// Non-canvas panel: leave layout to BP_OnSlotsRebuilt. Designers using
		// UniformGridPanel / Overlay can place entries themselves.
		return;
	}

	const int32 NumSlots = Entries.Num();
	if (NumSlots == 0)
	{
		return;
	}

	for (int32 i = 0; i < NumSlots; ++i)
	{
		USFCompanionRadialWheelEntry* Entry = Entries[i];
		if (!Entry)
		{
			continue;
		}

		UCanvasPanelSlot* CSlot = Cast<UCanvasPanelSlot>(Entry->Slot);
		if (!CSlot)
		{
			continue;
		}

		const float AngleRad = GetSlotAngleRadians(i, NumSlots);
		// 0 = up, then clockwise. Up = (0,-1) in screen-space.
		const float DirX = FMath::Sin(AngleRad);
		const float DirY = -FMath::Cos(AngleRad);

		const FVector2D Center(DirX * LayoutRadius, DirY * LayoutRadius);
		// Anchor is (0.5, 0.5). Offset by half-size so the entry centers on the radial point.
		CSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CSlot->SetSize(EntrySize);
		CSlot->SetPosition(Center);
	}
}

float USFCompanionRadialWheel::GetSlotAngleRadians(int32 Index, int32 TotalSlots) const
{
	if (TotalSlots <= 0)
	{
		return 0.f;
	}
	const float Step = 2.f * PI / static_cast<float>(TotalSlots);
	return FMath::DegreesToRadians(StartAngleDegrees) + Step * static_cast<float>(Index);
}

int32 USFCompanionRadialWheel::ResolveSlotForVector(FVector2D Selection) const
{
	const int32 Num = Slots.Num();
	if (Num <= 0)
	{
		return INDEX_NONE;
	}
	const float Mag = Selection.Size();
	if (Mag < DeadZone)
	{
		return INDEX_NONE;
	}

	// Convert Selection (X right, Y up — typical analog stick convention) into
	// our angular space (0 = up, clockwise positive).
	// atan2(X, Y) gives 0 at (0,1)=up and increases clockwise.
	float SelectionAngle = FMath::Atan2(Selection.X, Selection.Y);
	if (SelectionAngle < 0.f)
	{
		SelectionAngle += 2.f * PI;
	}

	// Each slot covers (2*PI / N) radians, centered on its slot angle.
	const float Step = 2.f * PI / static_cast<float>(Num);
	const float StartRad = FMath::DegreesToRadians(StartAngleDegrees);

	// Bring SelectionAngle into the start frame, then bucket.
	float Rel = SelectionAngle - StartRad + (Step * 0.5f);
	while (Rel < 0.f) { Rel += 2.f * PI; }
	while (Rel >= 2.f * PI) { Rel -= 2.f * PI; }

	int32 Bucket = static_cast<int32>(Rel / Step);
	if (Bucket < 0) Bucket = 0;
	if (Bucket >= Num) Bucket = Num - 1;
	return Bucket;
}

void USFCompanionRadialWheel::UpdateSelectionFromVector(FVector2D Selection)
{
	const int32 NewIndex = ResolveSlotForVector(Selection);
	SetHoveredIndex(NewIndex);
}

void USFCompanionRadialWheel::SetHoveredIndex(int32 NewIndex)
{
	if (NewIndex == HoveredIndex)
	{
		return;
	}
	if (NewIndex != INDEX_NONE && !Entries.IsValidIndex(NewIndex))
	{
		NewIndex = INDEX_NONE;
	}

	if (Entries.IsValidIndex(HoveredIndex))
	{
		if (USFCompanionRadialWheelEntry* Old = Entries[HoveredIndex])
		{
			Old->SetHovered(false);
		}
	}

	HoveredIndex = NewIndex;

	if (Entries.IsValidIndex(HoveredIndex))
	{
		if (USFCompanionRadialWheelEntry* New = Entries[HoveredIndex])
		{
			New->SetHovered(true);
		}
	}

	OnHoverChanged.Broadcast(HoveredIndex);
	BP_OnHoverChanged(HoveredIndex);
}

void USFCompanionRadialWheel::ConfirmHovered()
{
	ConfirmIndex(HoveredIndex);
}

void USFCompanionRadialWheel::ConfirmIndex(int32 Index)
{
	if (!Slots.IsValidIndex(Index))
	{
		return;
	}
	const FSFRadialWheelSlot& Slot = Slots[Index];
	if (!Slot.bEnabled)
	{
		return;
	}
	OnSlotConfirmed.Broadcast(Index, Slot);
}

void USFCompanionRadialWheel::CancelWheel()
{
	SetHoveredIndex(INDEX_NONE);
	OnWheelClosed.Broadcast();
}
