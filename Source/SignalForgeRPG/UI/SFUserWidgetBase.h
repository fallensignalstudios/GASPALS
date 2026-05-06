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

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void PlayHUDResumeAnimation();
};