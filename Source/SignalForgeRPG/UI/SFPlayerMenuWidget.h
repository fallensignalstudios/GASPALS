#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFPlayerMenuWidget.generated.h"

class USFPlayerHUDWidgetController;
class USFPlayerEquipmentWidget;
class USFPlayerInventoryWidget;

UCLASS(BlueprintType, Blueprintable)
class SIGNALFORGERPG_API USFPlayerMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void InitializeMenu(USFPlayerHUDWidgetController* InHUDWidgetController);

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void DeinitializeMenu();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Menu")
	void PlayCloseMenuAnimation();

	UFUNCTION(BlueprintPure, Category = "Menu")
	USFPlayerHUDWidgetController* GetHUDWidgetController() const
	{
		return HUDWidgetController;
	}

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Menu")
	void BP_OnMenuInitialized();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Menu")
	TObjectPtr<USFPlayerHUDWidgetController> HUDWidgetController = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Menu")
	TObjectPtr<USFPlayerEquipmentWidget> EquipmentPanel = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Menu")
	TObjectPtr<USFPlayerInventoryWidget> InventoryPanel = nullptr;
};