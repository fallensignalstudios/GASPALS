#include "UI/SFAttributeBarWidget.h"

void USFAttributeBarWidget::SetCurrentAndMax(float InCurrentValue, float InMaxValue)
{
    MaxValue = FMath::Max(0.0f, InMaxValue);
    CurrentValue = FMath::Clamp(InCurrentValue, 0.0f, MaxValue > 0.0f ? MaxValue : InCurrentValue);

    if (CurrentAttribute == ESFTrackedAttribute::None || MaxAttribute == ESFTrackedAttribute::None)
    {
        UE_LOG(LogTemp, Warning, TEXT("Attribute bar widget has unassigned tracked attributes."));
    }
}

float USFAttributeBarWidget::GetPercent() const
{
    return MaxValue > KINDA_SMALL_NUMBER ? FMath::Clamp(CurrentValue / MaxValue, 0.0f, 1.0f) : 0.0f;
}

