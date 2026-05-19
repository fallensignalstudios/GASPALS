#include "UI/SFUserWidgetBase.h"
#include "UI/SFPlayerHUDWidgetController.h"

#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"

static void PropagateControllerToSFChildren(
	UWidget* Root,
	USFPlayerHUDWidgetController* InWidgetController)
{
	if (!Root)
	{
		return;
	}

	// If this child is itself a UserWidget, set its controller directly so its
	// own native hook fires and recurses into ITS widget tree.
	if (USFUserWidgetBase* ChildSF = Cast<USFUserWidgetBase>(Root))
	{
		if (ChildSF->PlayerHUDWidgetController != InWidgetController)
		{
			ChildSF->SetPlayerHUDWidgetController(InWidgetController);
		}
		return;
	}

	// Otherwise, walk this widget's children if it's a panel.
	if (UPanelWidget* Panel = Cast<UPanelWidget>(Root))
	{
		const int32 NumChildren = Panel->GetChildrenCount();
		for (int32 i = 0; i < NumChildren; ++i)
		{
			PropagateControllerToSFChildren(Panel->GetChildAt(i), InWidgetController);
		}
	}
}

void USFUserWidgetBase::SetPlayerHUDWidgetController(USFPlayerHUDWidgetController* InWidgetController)
{
	PlayerHUDWidgetController = InWidgetController;
	NativeOnPlayerHUDWidgetControllerSet();
	OnWidgetControllerSet();

	UE_LOG(LogTemp, Display,
		TEXT("USFUserWidgetBase '%s': SetPlayerHUDWidgetController(%s) — propagating to children"),
		*GetNameSafe(this),
		*GetNameSafe(InWidgetController));

	// Walk the WidgetTree's panel children. WidgetTree::ForEachWidget can be
	// inconsistent across versions when child UserWidgets are involved, so we
	// drive the recursion ourselves through the panel hierarchy.
	if (WidgetTree)
	{
		if (UWidget* Root = WidgetTree->RootWidget)
		{
			if (UPanelWidget* RootPanel = Cast<UPanelWidget>(Root))
			{
				const int32 NumChildren = RootPanel->GetChildrenCount();
				for (int32 i = 0; i < NumChildren; ++i)
				{
					PropagateControllerToSFChildren(
						RootPanel->GetChildAt(i),
						InWidgetController);
				}
			}
			else if (Root != this)
			{
				PropagateControllerToSFChildren(Root, InWidgetController);
			}
		}
	}
}