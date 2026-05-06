#pragma once

#include "CoreMinimal.h"
#include "Companions/SFCompanionTypes.h"
#include "GameplayTagContainer.h"
#include "Engine/Texture2D.h"
#include "SFRadialWheelTypes.generated.h"

/**
 * What a wheel slot does when confirmed. The command controller
 * dispatches based on this enum + the slot's payload fields.
 */
UENUM(BlueprintType)
enum class ESFRadialWheelAction : uint8
{
	None         UMETA(DisplayName = "None"),
	SetStance    UMETA(DisplayName = "Set Stance"),
	SetAggression UMETA(DisplayName = "Set Aggression"),
	IssueOrder   UMETA(DisplayName = "Issue Order"),
	UseAbility   UMETA(DisplayName = "Use Ability"),
	OpenPage     UMETA(DisplayName = "Open Page"),  // Navigate to another page (PageId)
	GoBack       UMETA(DisplayName = "Go Back"),    // Return to previous page
	Cancel       UMETA(DisplayName = "Cancel"),     // Close wheel without action
};

/**
 * Identifies which page of the wheel is currently shown. Page nav is
 * pure data — the same wheel widget swaps slots when the page changes.
 */
UENUM(BlueprintType)
enum class ESFRadialWheelPage : uint8
{
	Root        UMETA(DisplayName = "Root"),       // Stance / Order / Ability / Cancel
	Stance      UMETA(DisplayName = "Stance"),     // Tank / DPS / Healer / Custom
	Aggression  UMETA(DisplayName = "Aggression"), // Passive / Defensive / Aggressive
	Order       UMETA(DisplayName = "Order"),      // Follow / Hold / MoveTo / Attack / FocusFire / Retreat
	Ability     UMETA(DisplayName = "Ability"),    // Companion-specific commandable abilities
};

/**
 * One slot on the wheel. POD-ish; built up by USFCompanionCommandController
 * and pushed into the wheel widget. Designers can extend visuals via the
 * BP-only RadialWheelEntry subclass, but the data here is what drives
 * dispatch.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFRadialWheelSlot
{
	GENERATED_BODY()

	/** Action taken when this slot is confirmed. */
	UPROPERTY(BlueprintReadWrite, Category = "Radial")
	ESFRadialWheelAction Action = ESFRadialWheelAction::None;

	/** Display label, e.g. "Tank Stance", "Hold Position", "Overcharge". */
	UPROPERTY(BlueprintReadWrite, Category = "Radial")
	FText Label;

	/** Tooltip / sub-label shown on hover, e.g. ability cost or current value. */
	UPROPERTY(BlueprintReadWrite, Category = "Radial")
	FText Description;

	/** Optional icon for the slot. Soft-loaded so we don't pay GC cost when wheel is hidden. */
	UPROPERTY(BlueprintReadWrite, Category = "Radial")
	TSoftObjectPtr<UTexture2D> Icon;

	/** When false, the slot is shown but cannot be confirmed (e.g. ability on cooldown, requirement missing). */
	UPROPERTY(BlueprintReadWrite, Category = "Radial")
	bool bEnabled = true;

	/** When true, the slot is rendered with a "current" highlight (e.g. the active stance). */
	UPROPERTY(BlueprintReadWrite, Category = "Radial")
	bool bIsCurrent = false;

	// ---- Action payload (only the fields relevant to Action are read) ----

	UPROPERTY(BlueprintReadWrite, Category = "Radial|Payload")
	ESFCompanionStance Stance = ESFCompanionStance::DPS;

	UPROPERTY(BlueprintReadWrite, Category = "Radial|Payload")
	ESFCompanionAggression Aggression = ESFCompanionAggression::Defensive;

	UPROPERTY(BlueprintReadWrite, Category = "Radial|Payload")
	ESFCompanionOrderType OrderType = ESFCompanionOrderType::None;

	/** For UseAbility action and IssueOrder/UseAbility orders. */
	UPROPERTY(BlueprintReadWrite, Category = "Radial|Payload")
	FGameplayTag AbilityTag;

	/** For OpenPage action — the page to navigate to. */
	UPROPERTY(BlueprintReadWrite, Category = "Radial|Payload")
	ESFRadialWheelPage TargetPage = ESFRadialWheelPage::Root;
};
