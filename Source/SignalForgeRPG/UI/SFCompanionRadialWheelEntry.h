#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/SFRadialWheelTypes.h"
#include "SFCompanionRadialWheelEntry.generated.h"

class UImage;
class UTextBlock;
class UWidget;

/**
 * USFCompanionRadialWheelEntry
 *
 * Single-slot view inside the companion radial wheel. Designers
 * subclass this in BP and bind the optional widgets (icon, label,
 * highlight, disabled overlay). The C++ base only owns the slot data,
 * hover/selection state, and the "refresh" hook.
 *
 * Picking is handled by the parent wheel widget — this widget does not
 * need its own button; we drive selection from the analog stick / mouse
 * angle. A click action is exposed for mouse-driven UX as a fallback.
 */
UCLASS(BlueprintType, Blueprintable)
class SIGNALFORGERPG_API USFCompanionRadialWheelEntry : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Radial")
	void SetSlotData(const FSFRadialWheelSlot& InSlot);

	UFUNCTION(BlueprintPure, Category = "Radial")
	const FSFRadialWheelSlot& GetSlotData() const { return Slot; }

	/** Index this entry occupies on the wheel; the wheel sets it on layout. */
	UFUNCTION(BlueprintCallable, Category = "Radial")
	void SetSlotIndex(int32 InIndex) { SlotIndex = InIndex; }

	UFUNCTION(BlueprintPure, Category = "Radial")
	int32 GetSlotIndex() const { return SlotIndex; }

	/** Called by the wheel when this entry becomes the highlighted slot. */
	UFUNCTION(BlueprintCallable, Category = "Radial")
	void SetHovered(bool bInHovered);

	UFUNCTION(BlueprintPure, Category = "Radial")
	bool IsHovered() const { return bHovered; }

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	/** BP-side hook: re-skin slot from current Slot data. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Radial")
	void BP_OnSlotRefreshed();

	/** BP-side hook: hover state changed. Useful for animations. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Radial")
	void BP_OnHoverChanged(bool bInHovered);

	/** Default text/icon refresh — only runs if BindWidgetOptional widgets exist. */
	UFUNCTION(BlueprintCallable, Category = "Radial")
	void RefreshDefaultBindings();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Radial")
	FSFRadialWheelSlot Slot;

	UPROPERTY(BlueprintReadOnly, Category = "Radial")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Radial")
	bool bHovered = false;

	// Optional default bindings — designers may use any of these or none.

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Radial")
	TObjectPtr<UTextBlock> LabelText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Radial")
	TObjectPtr<UTextBlock> DescriptionText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Radial")
	TObjectPtr<UImage> IconImage = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Radial")
	TObjectPtr<UWidget> HoverHighlight = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Radial")
	TObjectPtr<UWidget> CurrentMarker = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Radial")
	TObjectPtr<UWidget> DisabledOverlay = nullptr;
};
