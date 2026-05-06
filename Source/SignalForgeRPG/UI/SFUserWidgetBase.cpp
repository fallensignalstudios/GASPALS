#include "UI/SFUserWidgetBase.h"
#include "UI/SFPlayerHUDWidgetController.h"

void USFUserWidgetBase::SetPlayerHUDWidgetController(USFPlayerHUDWidgetController* InWidgetController)
{
	PlayerHUDWidgetController = InWidgetController;
	OnWidgetControllerSet();
}