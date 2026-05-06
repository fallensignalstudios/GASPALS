#include "UI/SFCompanionRadialWheelEntry.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"

void USFCompanionRadialWheelEntry::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshDefaultBindings();
}

void USFCompanionRadialWheelEntry::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshDefaultBindings();
	BP_OnSlotRefreshed();
}

void USFCompanionRadialWheelEntry::SetSlotData(const FSFRadialWheelSlot& InSlot)
{
	SlotData = InSlot;
	RefreshDefaultBindings();
	BP_OnSlotRefreshed();
}

void USFCompanionRadialWheelEntry::SetHovered(bool bInHovered)
{
	if (bRadialHovered == bInHovered)
	{
		return;
	}
	bRadialHovered = bInHovered;

	if (HoverHighlight)
	{
		HoverHighlight->SetVisibility(bRadialHovered ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	BP_OnHoverChanged(bRadialHovered);
}

void USFCompanionRadialWheelEntry::RefreshDefaultBindings()
{
	if (LabelText)
	{
		LabelText->SetText(SlotData.Label);
	}
	if (DescriptionText)
	{
		DescriptionText->SetText(SlotData.Description);
	}
	if (CurrentMarker)
	{
		CurrentMarker->SetVisibility(SlotData.bIsCurrent ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	if (DisabledOverlay)
	{
		DisabledOverlay->SetVisibility(SlotData.bEnabled ? ESlateVisibility::Hidden : ESlateVisibility::HitTestInvisible);
	}
	if (IconImage)
	{
		// Async-resolve the soft icon if set, else clear.
		if (!SlotData.Icon.IsNull())
		{
			if (UTexture2D* Loaded = SlotData.Icon.Get())
			{
				IconImage->SetBrushFromTexture(Loaded);
			}
			else
			{
				FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
				TWeakObjectPtr<USFCompanionRadialWheelEntry> WeakThis(this);
				const FSoftObjectPath Path = SlotData.Icon.ToSoftObjectPath();
				Streamable.RequestAsyncLoad(Path, FStreamableDelegate::CreateLambda([WeakThis, Path]()
				{
					if (USFCompanionRadialWheelEntry* Strong = WeakThis.Get())
					{
						if (UTexture2D* Tex = Cast<UTexture2D>(Path.ResolveObject()))
						{
							if (Strong->IconImage)
							{
								Strong->IconImage->SetBrushFromTexture(Tex);
							}
						}
					}
				}));
			}
		}
		else
		{
			IconImage->SetBrushFromTexture(nullptr);
		}
	}
}
