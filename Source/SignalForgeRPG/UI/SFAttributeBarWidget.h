#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "UI/SFUserWidgetBase.h"
#include "SFAttributeBarWidget.generated.h"

UENUM(BlueprintType)
enum class ESFTrackedAttribute : uint8
{
    None        UMETA(DisplayName = "None"),
    Health      UMETA(DisplayName = "Health"),
    MaxHealth   UMETA(DisplayName = "Max Health"),
    Echo        UMETA(DisplayName = "Echo"),
    MaxEcho     UMETA(DisplayName = "Max Echo"),
    Shields     UMETA(DisplayName = "Shields"),
    MaxShields  UMETA(DisplayName = "Max Shields"),
    Stamina     UMETA(DisplayName = "Stamina"),
    MaxStamina  UMETA(DisplayName = "Max Stamina")
};

UENUM(BlueprintType)
enum class ESFAttributeBarSource : uint8
{
    Player  UMETA(DisplayName = "Player"),
    TrackedEnemy UMETA(DisplayName = "Tracked Enemy")
};

UCLASS()
class SIGNALFORGERPG_API USFAttributeBarWidget : public USFUserWidgetBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetCurrentAndMax(float InCurrentValue, float InMaxValue);

    UFUNCTION(BlueprintPure, Category = "UI")
    float GetPercent() const;

    UFUNCTION(BlueprintPure, Category = "UI")
    float GetCurrentValue() const { return CurrentValue; }

    UFUNCTION(BlueprintPure, Category = "UI")
    float GetMaxValue() const { return MaxValue; }

    UFUNCTION(BlueprintPure, Category = "UI")
    ESFTrackedAttribute GetCurrentAttribute() const { return CurrentAttribute; }

    UFUNCTION(BlueprintPure, Category = "UI")
    ESFTrackedAttribute GetMaxAttribute() const { return MaxAttribute; }

    UFUNCTION(BlueprintPure, Category = "UI|Appearance")
    FLinearColor GetFillColorAndOpacity() const
    {
        FLinearColor Result = FillColor;
        Result.A = FillOpacity;
        return Result;
    }

    UFUNCTION(BlueprintPure, Category = "UI|Appearance")
    FLinearColor GetBackgroundColorAndOpacity() const
    {
        FLinearColor Result = BackgroundColor;
        Result.A = BackgroundOpacity;
        return Result;
    }

    UFUNCTION(BlueprintPure, Category = "UI|Appearance")
    const FSlateBrush& GetFillBrush() const { return FillBrush; }

    UFUNCTION(BlueprintPure, Category = "UI|Appearance")
    const FSlateBrush& GetBackgroundBrush() const { return BackgroundBrush; }

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Attributes")
    ESFAttributeBarSource BarSource = ESFAttributeBarSource::Player;

    UFUNCTION(BlueprintPure, Category = "UI")
    ESFAttributeBarSource GetBarSource() const { return BarSource; }

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Attributes")
    ESFTrackedAttribute CurrentAttribute = ESFTrackedAttribute::Health;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Attributes")
    ESFTrackedAttribute MaxAttribute = ESFTrackedAttribute::MaxHealth;

    UPROPERTY(BlueprintReadOnly, Category = "UI")
    float CurrentValue = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "UI")
    float MaxValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Appearance")
    FLinearColor FillColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Appearance", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float FillOpacity = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Appearance")
    FLinearColor BackgroundColor = FLinearColor::Black;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Appearance", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BackgroundOpacity = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Appearance|Brushes")
    FSlateBrush FillBrush;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Appearance|Brushes")
    FSlateBrush BackgroundBrush;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Appearance")
    bool bUseCustomFillBrush = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Appearance")
    bool bUseCustomBackgroundBrush = false;
};