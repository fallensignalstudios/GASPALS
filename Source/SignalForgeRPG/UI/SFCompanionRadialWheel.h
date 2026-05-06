#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/SFRadialWheelTypes.h"
#include "SFCompanionRadialWheel.generated.h"

class UCanvasPanel;
class UPanelWidget;
class USFCompanionRadialWheelEntry;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FSFRadialWheelSlotConfirmed,
	int32, SlotIndex,
	const FSFRadialWheelSlot&, Slot);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FSFRadialWheelHoverChanged,
	int32, NewHoveredIndex);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSFRadialWheelClosed);

/**
 * USFCompanionRadialWheel
 *
 * Native radial-wheel widget. Owns a list of slots and an array of
 * spawned USFCompanionRadialWheelEntry children, lays them around a
 * circle inside a UCanvasPanel anchor, and resolves the hovered slot
 * from a 2D selection vector (analog stick or mouse delta).
 *
 * The widget itself is "dumb" — it doesn't know what stance or order
 * each slot represents, just that a slot exists. The owning
 * USFCompanionCommandController fills SetSlots() with whatever the
 * current page calls for and listens to OnSlotConfirmed.
 */
UCLASS(BlueprintType, Blueprintable)
class SIGNALFORGERPG_API USFCompanionRadialWheel : public UUserWidget
{
	GENERATED_BODY()

public:
	// -------------------------------------------------------------------------
	// Slot setup
	// -------------------------------------------------------------------------

	/** Replaces all slots and rebuilds child entries. Triggers layout. */
	UFUNCTION(BlueprintCallable, Category = "Radial")
	void SetSlots(const TArray<FSFRadialWheelSlot>& InSlots);

	UFUNCTION(BlueprintPure, Category = "Radial")
	const TArray<FSFRadialWheelSlot>& GetSlots() const { return Slots; }

	UFUNCTION(BlueprintPure, Category = "Radial")
	int32 GetSlotCount() const { return Slots.Num(); }

	// -------------------------------------------------------------------------
	// Selection
	// -------------------------------------------------------------------------

	/**
	 * Feeds a 2D vector (e.g. right-stick or normalized mouse-from-center)
	 * into the wheel. If the vector is outside DeadZone, the slot whose
	 * angular bucket contains the vector becomes the hovered slot.
	 * If inside DeadZone, hover clears.
	 */
	UFUNCTION(BlueprintCallable, Category = "Radial")
	void UpdateSelectionFromVector(FVector2D Selection);

	/** Manually set the hovered slot. Pass INDEX_NONE to clear. */
	UFUNCTION(BlueprintCallable, Category = "Radial")
	void SetHoveredIndex(int32 NewIndex);

	UFUNCTION(BlueprintPure, Category = "Radial")
	int32 GetHoveredIndex() const { return HoveredIndex; }

	/** Confirms the currently-hovered slot. Emits OnSlotConfirmed if the slot is enabled. */
	UFUNCTION(BlueprintCallable, Category = "Radial")
	void ConfirmHovered();

	/** Confirms an arbitrary index (used by mouse click pathways). */
	UFUNCTION(BlueprintCallable, Category = "Radial")
	void ConfirmIndex(int32 Index);

	/** Closes the wheel without confirming anything. Emits OnWheelClosed. */
	UFUNCTION(BlueprintCallable, Category = "Radial")
	void CancelWheel();

	// -------------------------------------------------------------------------
	// Delegates
	// -------------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Radial")
	FSFRadialWheelSlotConfirmed OnSlotConfirmed;

	UPROPERTY(BlueprintAssignable, Category = "Radial")
	FSFRadialWheelHoverChanged OnHoverChanged;

	UPROPERTY(BlueprintAssignable, Category = "Radial")
	FSFRadialWheelClosed OnWheelClosed;

	// -------------------------------------------------------------------------
	// Tunables
	// -------------------------------------------------------------------------

	/** Slot subclass to spawn for each slot. Required. Subclass in BP for art. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Radial")
	TSubclassOf<USFCompanionRadialWheelEntry> EntryWidgetClass;

	/** Inputs whose magnitude is below this are treated as "no hover". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DeadZone = 0.25f;

	/** Layout radius (in slate units) used by the default canvas placement. Ignored if the slot panel is not a UCanvasPanel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial", meta = (ClampMin = "0.0"))
	float LayoutRadius = 200.f;

	/** Angle (degrees) for the first slot. 0 = straight up, then clockwise. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial")
	float StartAngleDegrees = 0.f;

	/** Slot size used for default canvas placement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Radial")
	FVector2D EntrySize = FVector2D(120.f, 120.f);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Designer drops the panel that hosts the spawned entries. UCanvasPanel gets auto-layout for free. */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "Radial")
	TObjectPtr<UPanelWidget> SlotPanel = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Radial")
	TArray<FSFRadialWheelSlot> Slots;

	UPROPERTY(BlueprintReadOnly, Category = "Radial")
	TArray<TObjectPtr<USFCompanionRadialWheelEntry>> Entries;

	UPROPERTY(BlueprintReadOnly, Category = "Radial")
	int32 HoveredIndex = INDEX_NONE;

	/** Spawns/refreshes child entries and lays them out. */
	void RebuildEntries();

	/** Positions entries around the circle if SlotPanel is a UCanvasPanel. No-op otherwise. */
	void LayoutEntries();

	/** Returns the angle (radians, 0=up, clockwise) for slot index N out of TotalSlots. */
	float GetSlotAngleRadians(int32 Index, int32 TotalSlots) const;

	/** Maps a 2D selection vector to a slot index. Returns INDEX_NONE if inside DeadZone or no slots. */
	int32 ResolveSlotForVector(FVector2D Selection) const;

	/** BP hook for layout customization (e.g. non-canvas panels). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Radial")
	void BP_OnSlotsRebuilt();

	UFUNCTION(BlueprintImplementableEvent, Category = "Radial")
	void BP_OnHoverChanged(int32 NewIndex);
};
