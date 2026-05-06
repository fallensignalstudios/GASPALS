#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/SFEquipmentDisplayTypes.h"
#include "SFPlayerEquipmentWidget.generated.h"

class UVerticalBox;
class USFEquipmentComponent;
class USFEquipmentWidgetController;
class USFEquipmentSlotWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerEquipmentSlotClickedSignature, const FSFEquipmentDisplayEntry&, Entry);

UCLASS(BlueprintType, Blueprintable)
class SIGNALFORGERPG_API USFPlayerEquipmentWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void InitializeEquipmentWidget(USFEquipmentComponent* InEquipmentComponent);

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void DeinitializeEquipmentWidget();

	UFUNCTION(BlueprintPure, Category = "Equipment")
	USFEquipmentWidgetController* GetWidgetController() const
	{
		return WidgetController;
	}

	UFUNCTION(BlueprintPure, Category = "Equipment")
	TArray<FSFEquipmentDisplayEntry> GetCurrentDisplayEntries() const;

	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FOnPlayerEquipmentSlotClickedSignature OnEquipmentSlotClicked;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void HandleEquipmentDisplayDataUpdated();

	UFUNCTION()
	void HandleSlotWidgetClicked(const FSFEquipmentDisplayEntry& Entry);

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void RebuildSlotWidgets();

	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool IsArmorSlotType(ESFEquipmentSlotType SlotType) const;

	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool IsWeaponSlotType(ESFEquipmentSlotType SlotType) const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	TSubclassOf<USFEquipmentSlotWidget> EquipmentSlotWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<USFEquipmentWidgetController> WidgetController = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<USFEquipmentComponent> EquipmentComponent = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UVerticalBox> ArmorSlotList = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UVerticalBox> WeaponSlotList = nullptr;
};