#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UI/SFRadialWheelTypes.h"
#include "SFCompanionCommandController.generated.h"

class APlayerController;
class USFCompanionComponent;
class USFCompanionRadialWheel;
class ASFCompanionCharacter;

/**
 * USFCompanionCommandController
 *
 * Drop this on the local ASFPlayerController. It owns the radial-wheel
 * widget instance and bridges input -> wheel -> companion component.
 *
 * Lifecycle:
 *   - OpenWheel() creates the widget (if not already), pushes Root page
 *     slots, calls USFCompanionComponent::BeginCommandMode (slow-mo),
 *     and adds the widget to the viewport with input mode.
 *   - While open, the player feeds a 2D selection vector via
 *     UpdateSelection() (game thumbstick or mouse-from-center).
 *   - ConfirmHovered() commits the slot — for stance/aggression we
 *     immediately call the matching CommandSet* on the component;
 *     for orders we build an FSFCompanionOrder and route through
 *     CommandFollow/CommandHoldPosition/etc. Pages reload slots.
 *   - CloseWheel() ends slow-mo and removes the widget.
 *
 * Server authority:
 *   The CommandSet* / Command* methods on USFCompanionComponent are
 *   BlueprintAuthorityOnly. In singleplayer / listen server this is
 *   fine. For dedicated-server we'd add a Server RPC here later;
 *   that's deferred to the multiplayer pass per scope decision.
 */
UCLASS(ClassGroup = (Companion), meta = (BlueprintSpawnableComponent))
class SIGNALFORGERPG_API USFCompanionCommandController : public UActorComponent
{
	GENERATED_BODY()

public:
	USFCompanionCommandController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// -------------------------------------------------------------------------
	// Wheel open/close
	// -------------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Companion|Radial")
	void OpenWheel();

	UFUNCTION(BlueprintCallable, Category = "Companion|Radial")
	void CloseWheel();

	UFUNCTION(BlueprintPure, Category = "Companion|Radial")
	bool IsWheelOpen() const { return bWheelOpen; }

	UFUNCTION(BlueprintPure, Category = "Companion|Radial")
	USFCompanionRadialWheel* GetWheelWidget() const { return WheelWidget; }

	// -------------------------------------------------------------------------
	// Per-frame input (call from player controller's tick or input axis)
	// -------------------------------------------------------------------------

	/** Feed the wheel a 2D selection vector (e.g. right-stick or mouse-from-center). */
	UFUNCTION(BlueprintCallable, Category = "Companion|Radial")
	void UpdateSelection(FVector2D Selection);

	/** Confirms whatever slot is currently hovered on the wheel. */
	UFUNCTION(BlueprintCallable, Category = "Companion|Radial")
	void ConfirmHovered();

	// -------------------------------------------------------------------------
	// Page nav
	// -------------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Companion|Radial")
	void GoToPage(ESFRadialWheelPage NewPage);

	UFUNCTION(BlueprintCallable, Category = "Companion|Radial")
	void GoBack();

	UFUNCTION(BlueprintPure, Category = "Companion|Radial")
	ESFRadialWheelPage GetCurrentPage() const { return CurrentPage; }

	// -------------------------------------------------------------------------
	// Move-to-location target (set by raycast before confirming MoveTo slot)
	// -------------------------------------------------------------------------

	/**
	 * Stash a world location to use when the player confirms a "Move To" slot.
	 * BP / PlayerController traces the cursor each frame and pipes the hit
	 * here. Cleared on each wheel open.
	 */
	UFUNCTION(BlueprintCallable, Category = "Companion|Radial")
	void SetPendingMoveLocation(FVector InLocation, bool bInValid);

	/** Stash a target actor for Attack / FocusFire / Use Ability slots. */
	UFUNCTION(BlueprintCallable, Category = "Companion|Radial")
	void SetPendingTargetActor(AActor* InTarget);

	// -------------------------------------------------------------------------
	// Tunables
	// -------------------------------------------------------------------------

	/** Class to spawn for the wheel widget. Required. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Radial")
	TSubclassOf<USFCompanionRadialWheel> WheelWidgetClass;

	/** Commandable abilities the wheel surfaces on the Ability page. Designer-edited. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Radial")
	TArray<FSFRadialWheelSlot> AbilitySlots;

	/** Z order for the wheel widget. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Radial")
	int32 WheelZOrder = 50;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Companion|Radial")
	TObjectPtr<USFCompanionRadialWheel> WheelWidget = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Companion|Radial")
	ESFRadialWheelPage CurrentPage = ESFRadialWheelPage::Root;

	UPROPERTY(BlueprintReadOnly, Category = "Companion|Radial")
	ESFRadialWheelPage PreviousPage = ESFRadialWheelPage::Root;

	bool bWheelOpen = false;
	FVector PendingMoveLocation = FVector::ZeroVector;
	bool bPendingMoveValid = false;
	TWeakObjectPtr<AActor> PendingTarget;

	// Page builders
	void BuildSlotsForPage(ESFRadialWheelPage Page, TArray<FSFRadialWheelSlot>& OutSlots) const;
	void BuildRootSlots(TArray<FSFRadialWheelSlot>& OutSlots) const;
	void BuildStanceSlots(TArray<FSFRadialWheelSlot>& OutSlots) const;
	void BuildAggressionSlots(TArray<FSFRadialWheelSlot>& OutSlots) const;
	void BuildOrderSlots(TArray<FSFRadialWheelSlot>& OutSlots) const;
	void BuildAbilitySlots(TArray<FSFRadialWheelSlot>& OutSlots) const;

	void RefreshSlotsForCurrentPage();

	// Dispatch helpers
	UFUNCTION()
	void HandleSlotConfirmed(int32 SlotIndex, const FSFRadialWheelSlot& Slot);

	UFUNCTION()
	void HandleWheelClosed();

	void DispatchSlot(const FSFRadialWheelSlot& Slot);

	// Resolution helpers
	APlayerController* GetOwningPC() const;
	USFCompanionComponent* GetCompanionComponent() const;
	ASFCompanionCharacter* GetActiveCompanion() const;
};
