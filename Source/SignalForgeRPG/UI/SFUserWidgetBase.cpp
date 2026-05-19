#include "UI/SFUserWidgetBase.h"
#include "UI/SFPlayerHUDWidgetController.h"

#include "Blueprint/WidgetTree.h"

void USFUserWidgetBase::SetPlayerHUDWidgetController(USFPlayerHUDWidgetController* InWidgetController)
{
	PlayerHUDWidgetController = InWidgetController;
	NativeOnPlayerHUDWidgetControllerSet();
	OnWidgetControllerSet();

	// Propagate the controller down the widget tree so nested USFUserWidgetBase
	// children (e.g. WBP_InteractionPrompt placed inside WBP_PlayerHUD) receive
	// the same controller and can bind their own delegates without each
	// designer having to wire it up by hand.
	if (WidgetTree)
	{
		WidgetTree->ForEachWidget([InWidgetController, this](UWidget* Widget)
		{
			if (Widget == this)
			{
				return;
			}
			if (USFUserWidgetBase* ChildSF = Cast<USFUserWidgetBase>(Widget))
			{
				if (ChildSF->PlayerHUDWidgetController != InWidgetController)
				{
					ChildSF->SetPlayerHUDWidgetController(InWidgetController);
				}
			}
		});
	}
}