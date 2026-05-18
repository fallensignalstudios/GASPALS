#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SFUserWidgetBase.generated.h"

UCLASS()
class SIGNALFORGERPG_API USFUserWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "UI")
	void OnWidgetControllerSet();

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<class USFPlayerHUDWidgetController> PlayerHUDWidgetController;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetPlayerHUDWidgetController(USFPlayerHUDWidgetController* InWidgetController);

protected:
	/**
	 * Native hook fired immediately before OnWidgetControllerSet is broadcast
	 * to Blueprint. C++ subclasses override this to bind controller delegates
	 * without relying on Blueprint scripting.
	 */
	virtual void NativeOnPlayerHUDWidgetControllerSet() {}

public:

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void PlayHUDResumeAnimation();
};