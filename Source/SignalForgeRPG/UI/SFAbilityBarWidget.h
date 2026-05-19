#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UI/SFUserWidgetBase.h"
#include "UI/SFAbilitySlotUIData.h"
#include "SFAbilityBarWidget.generated.h"

class APlayerController;
class ASFCharacterBase;
class UPanelWidget;
class USFAbilityBarWidgetController;
class USFAbilitySlotWidget;

/**
 * Native parent for WBP_AbilityBar.
 *
 * Self-populating ability bar:
 *   - On NativeConstruct, locates the local player's USFAbilitySystemComponent
 *     and creates/owns a USFAbilityBarWidgetController bound to it.
 *   - Lays out one USFAbilitySlotWidget per entry in SlotLayout (a designer-
 *     configured list of InputTags). This keeps the bar order STABLE across
 *     ability grants/removes — the controller alone has no deterministic order.
 *   - On controller broadcasts (or when initial data arrives), pushes the
 *     latest FSFAbilitySlotUIData into the matching slot widget.
 *   - Subscribes to APlayerController::OnPossessedPawnChanged so the bar
 *     re-binds when the pawn changes (death, respawn, level transition).
 *
 * Setup in the editor:
 *   - Reparent WBP_AbilityBar to USFAbilityBarWidget.
 *   - Bind 'SlotsContainer' to a UPanelWidget (HorizontalBox recommended).
 *   - Set 'SlotWidgetClass' to your WBP_AbilitySlot (reparented to
 *     USFAbilitySlotWidget).
 *   - Fill 'SlotLayout' with the InputTags in the order they should appear,
 *     each with an optional HotkeyLabel.
 */
USTRUCT(BlueprintType)
struct FSFAbilityBarSlotLayout
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Bar", meta = (Categories = "InputTag"))
	FGameplayTag InputTag;

	/** Optional hotkey hint passed to the slot widget. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Bar")
	FText HotkeyLabel;
};

UCLASS(Abstract, Blueprintable)
class SIGNALFORGERPG_API USFAbilityBarWidget : public USFUserWidgetBase
{
	GENERATED_BODY()

public:
	/** Manually point the bar at a player (useful for split-screen). */
	UFUNCTION(BlueprintCallable, Category = "Ability|Bar")
	void BindToPlayer(ASFCharacterBase* InPlayer);

	UFUNCTION(BlueprintCallable, Category = "Ability|Bar")
	void RefreshSlotsFromController();

	UFUNCTION(BlueprintPure, Category = "Ability|Bar")
	USFAbilityBarWidgetController* GetAbilityBarController() const { return AbilityController; }

	UFUNCTION(BlueprintPure, Category = "Ability|Bar")
	USFAbilitySlotWidget* GetSlotForTag(FGameplayTag InputTag) const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// === Bound widgets ===

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> SlotsContainer = nullptr;

	// === Config ===

	UPROPERTY(EditDefaultsOnly, Category = "Ability|Bar")
	TSubclassOf<USFAbilitySlotWidget> SlotWidgetClass;

	/** Ordered slot layout. Driven by designer; bar order matches this list. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability|Bar")
	TArray<FSFAbilityBarSlotLayout> SlotLayout;

	/** If true, the bar tries to bind on construct. Disable for manual binding. */
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Bar")
	bool bAutoBindOnConstruct = true;

	// === State ===

	UPROPERTY(BlueprintReadOnly, Category = "Ability|Bar")
	TObjectPtr<ASFCharacterBase> BoundCharacter = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Ability|Bar")
	TObjectPtr<USFAbilityBarWidgetController> AbilityController = nullptr;

	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<USFAbilitySlotWidget>> SlotWidgets;

	// === Handlers ===

	UFUNCTION()
	void HandleSlotUpdated(FGameplayTag InputTag, FSFAbilitySlotUIData SlotData);

	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	void BindToLocalPlayer();
	void RebuildSlotWidgets();
	void TeardownController();
};
