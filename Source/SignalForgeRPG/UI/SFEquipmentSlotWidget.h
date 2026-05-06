#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/SFEquipmentDisplayTypes.h"
#include "SFEquipmentSlotWidget.generated.h"

class UImage;
class UTextBlock;
class UWidget;
class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipmentSlotClickedSignature, const FSFEquipmentDisplayEntry&, Entry);

UCLASS(BlueprintType, Blueprintable)
class SIGNALFORGERPG_API USFEquipmentSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void SetSlotData(const FSFEquipmentDisplayEntry& Entry);

	UFUNCTION(BlueprintPure, Category = "Equipment")
	FSFEquipmentDisplayEntry GetSlotData() const
	{
		return CurrentEntry;
	}

	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool HasEquippedItem() const
	{
		return CurrentEntry.bHasItemEquipped;
	}

	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool IsWeaponSlot() const
	{
		return CurrentEntry.bIsWeaponSlot;
	}

	UFUNCTION(BlueprintPure, Category = "Equipment")
	FText GetSlotDisplayNameText() const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	FText GetItemDisplayText() const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	FText GetStateDisplayText() const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool ShouldShowWeaponIcon() const
	{
		return CurrentEntry.bHasItemEquipped && CurrentEntry.EquippedItemIcon != nullptr;
	}

	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool IsActiveSlot() const
	{
		return CurrentEntry.bIsActive;
	}

	UFUNCTION(BlueprintPure, Category = "Equipment")
	ESFEquipmentSlot GetEquipmentSlot() const
	{
		return CurrentEntry.EquipmentSlot;
	}

	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FOnEquipmentSlotClickedSignature OnSlotClicked;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	virtual void RefreshFromEntry();

	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment")
	void BP_OnSlotDataRefreshed();

	UFUNCTION()
	void HandleSlotButtonClicked();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	FSFEquipmentDisplayEntry CurrentEntry;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UButton> SlotButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UTextBlock> SlotNameText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UTextBlock> ItemNameText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UTextBlock> StateText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UImage> ItemIconImage = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UWidget> EquippedStateWidget = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UWidget> EmptyStateWidget = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UWidget> ActiveStateWidget = nullptr;
};