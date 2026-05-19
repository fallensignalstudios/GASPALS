#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UI/SFUserWidgetBase.h"
#include "UI/SFAbilitySlotUIData.h"
#include "SFAbilitySlotWidget.generated.h"

class UImage;
class UTextBlock;

/**
 * Native parent for WBP_AbilitySlot.
 *
 * Receives an FSFAbilitySlotUIData payload from the ability bar and updates
 * its bound widgets (icon, ready/cooldown state, optional empty visuals).
 *
 * Bind these widgets in the BP (all optional — bind what you use):
 *   - IconImage (UImage) — receives Data.Icon
 *   - EmptyImage (UImage) — shown when bHasAbility = false
 *   - CooldownOverlay (UWidget — typically an Image or Border) — visible while !bIsReady
 *   - HotkeyText (UTextBlock) — designer-fed hotkey label (e.g. "1", "Q")
 *
 * Designer sets HotkeyLabel in defaults so the same slot widget can be reused
 * for keyboard/gamepad layouts.
 */
UCLASS(Abstract, Blueprintable)
class SIGNALFORGERPG_API USFAbilitySlotWidget : public USFUserWidgetBase
{
	GENERATED_BODY()

public:
	/** Push slot data in. Triggers visual refresh and the BP hook. */
	UFUNCTION(BlueprintCallable, Category = "Ability|Slot")
	void SetSlotData(const FSFAbilitySlotUIData& InData);

	/** Which input tag this slot represents (set once, when the bar lays out). */
	UFUNCTION(BlueprintCallable, Category = "Ability|Slot")
	void SetInputTag(FGameplayTag InTag);

	UFUNCTION(BlueprintPure, Category = "Ability|Slot")
	FGameplayTag GetInputTag() const { return InputTag; }

	UFUNCTION(BlueprintPure, Category = "Ability|Slot")
	const FSFAbilitySlotUIData& GetSlotData() const { return CurrentData; }

	/** Blueprint hook for visual polish (flash on ready, hotkey hint, etc.). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Ability|Slot")
	void OnSlotDataChanged(const FSFAbilitySlotUIData& Data);

	UFUNCTION(BlueprintImplementableEvent, Category = "Ability|Slot")
	void OnReadyStateChanged(bool bIsReady);

protected:
	virtual void NativeConstruct() override;

	// === Bound widgets ===

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> EmptyImage = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> CooldownOverlay = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HotkeyText = nullptr;

	// === Config ===

	/** Designer-defined hotkey label (e.g. "1", "Q"). Optional. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Slot")
	FText HotkeyLabel;

	/** Hide the entire widget when no ability is bound to this slot. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Slot")
	bool bHideWhenEmpty = false;

	// === State ===

	UPROPERTY(BlueprintReadOnly, Category = "Ability|Slot")
	FGameplayTag InputTag;

	UPROPERTY(BlueprintReadOnly, Category = "Ability|Slot")
	FSFAbilitySlotUIData CurrentData;

private:
	void ApplyVisuals();
	bool bLastReady = false;
};
