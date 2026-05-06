#pragma once

#include "CoreMinimal.h"
#include "UI/SFUserWidgetBase.h"
#include "Inventory/SFInventoryDisplayTypes.h"
#include "SFInventoryEntryWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class USizeBox;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryEntryClickedSignature, FGuid, EntryGuid);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryEntryEquipRequestedSignature, FGuid, EntryGuid);

UCLASS(Blueprintable)
class SIGNALFORGERPG_API USFInventoryEntryWidget : public USFUserWidgetBase
{
	GENERATED_BODY()

public:
	// Set the display data for this tile.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetInventoryEntry(const FSFInventoryDisplayEntry& InEntry);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	const FSFInventoryDisplayEntry& GetInventoryEntry() const
	{
		return CurrentEntry;
	}

	// Selection is driven by the owning inventory widget.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetIsSelected(bool bInIsSelected);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool GetIsSelected() const
	{
		return bIsSelected;
	}

	// Fired when the tile is clicked (select).
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryEntryClickedSignature OnEntryClicked;

	// Fired when the tile requests an equip (double-click or equip button).
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryEntryEquipRequestedSignature OnEntryEquipRequested;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// Entire tile click target.
	UPROPERTY(BlueprintReadOnly, Category = "Inventory", meta = (BindWidgetOptional))
	TObjectPtr<UButton> EntryButton = nullptr;

	// Optional small equip button overlay (e.g., bottom-right).
	UPROPERTY(BlueprintReadOnly, Category = "Inventory", meta = (BindWidgetOptional))
	TObjectPtr<UButton> EquipButton = nullptr;

	// Root visual frame for the slot.
	UPROPERTY(BlueprintReadOnly, Category = "Inventory", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> RootBorder = nullptr;

	// Selection highlight overlay.
	UPROPERTY(BlueprintReadOnly, Category = "Inventory", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> SelectionBorder = nullptr;

	// Main item icon.
	UPROPERTY(BlueprintReadOnly, Category = "Inventory", meta = (BindWidgetOptional))
	TObjectPtr<UImage> ItemIconImage = nullptr;

	// Quantity label (bottom-right).
	UPROPERTY(BlueprintReadOnly, Category = "Inventory", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuantityText = nullptr;

	// Equipped marker text/icon (e.g., "E" or a small label).
	UPROPERTY(BlueprintReadOnly, Category = "Inventory", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EquippedText = nullptr;

	// Optional text on the equip button.
	UPROPERTY(BlueprintReadOnly, Category = "Inventory", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EquipButtonText = nullptr;

	// Controls the icon frame size (square slot by default).
	UPROPERTY(BlueprintReadOnly, Category = "Inventory", meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> IconSizeBox = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Presentation")
	float SlotIconSize = 64.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Presentation")
	float WeaponIconHeight = 96.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FSFInventoryDisplayEntry CurrentEntry;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	bool bIsSelected = false;

	UFUNCTION()
	void HandleEntryButtonClicked();

	UFUNCTION()
	void HandleEquipButtonClicked();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	virtual void RefreshVisuals();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	virtual void RefreshSelectionVisuals();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	virtual void RefreshActionState();

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void BP_OnEntrySet(const FSFInventoryDisplayEntry& InEntry);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void BP_OnSelectionChanged(bool bNewIsSelected);

	bool ShouldUseTallWeaponFrame() const;
	FText BuildQuantityText() const;
};