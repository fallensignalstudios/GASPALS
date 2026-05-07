#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BlackboardComponent.h"

class UBlackboardComponent;

/**
 * Canonical blackboard key names for the Companion BT layer.
 *
 * These match the keys the BT asset (BB_Companion) is expected to expose.
 * Designers can rename keys per-instance in the BT, but every native
 * service/task that ships with the project defaults to these names so a
 * fresh BT works out of the box.
 *
 * Key types (see paste.txt design doc):
 *   PlayerActor              Object<Actor>     player pawn we're escorting
 *   FocusTarget              Object<Actor>     current combat target
 *
 *   OrderType                Enum<ESFCompanionOrderType>
 *   OrderSequence            Int               last-handled sequence number
 *   OrderTargetActor         Object<Actor>
 *   OrderTargetLocation      Vector
 *   OrderAbilityTag          Name              FGameplayTag stringified
 *   bHasValidOrder           Bool              OrderType != None
 *
 *   Stance                   Enum<ESFCompanionStance>
 *   Aggression               Enum<ESFCompanionAggression>
 *   EngagementRange          Enum<ESFCompanionEngagementRange>
 *   StanceTag                Name              first stance gameplay tag
 *
 *   DesiredPosition          Vector            BT-computed formation slot
 *   DesiredCombatDistance    Float             stance/range-derived spacing
 *   HomeAnchorLocation       Vector            HoldPosition / fallback anchor
 *
 *   bSelfLowHealth           Bool              tactics threshold
 *   bPlayerLowHealth         Bool              tactics threshold
 *   bCanUseCommandedAbility  Bool              cooldown gate
 *   bHasLineOfSight          Bool              perception-derived (optional)
 */
namespace SFCompanionBlackboardKeys
{
	// Helper: read an enum-stored byte regardless of whether the BB key is
	// configured as Enum (UE only resolves enum entries with this) or Int
	// (designers fall back to Int when the editor can't see the C++ UENUM).
	// Returns 0 if neither read succeeds.
	inline uint8 GetEnumOrInt(const UBlackboardComponent* BB, FName KeyName)
	{
		if (!BB) { return 0; }
		const uint8 EnumVal = BB->GetValueAsEnum(KeyName);
		if (EnumVal != 0) { return EnumVal; }
		return static_cast<uint8>(BB->GetValueAsInt(KeyName));
	}

	// Helper: write a byte to an enum-style key supporting both Enum and Int
	// configurations. Both writes are no-ops on the wrong key type so this is
	// safe regardless of which the designer picked.
	inline void SetEnumOrInt(UBlackboardComponent* BB, FName KeyName, uint8 Value)
	{
		if (!BB) { return; }
		BB->SetValueAsEnum(KeyName, Value);
		BB->SetValueAsInt(KeyName, static_cast<int32>(Value));
	}

	// --- Actors / focus -----------------------------------------------------
	inline const FName PlayerActor             = TEXT("PlayerActor");
	inline const FName FocusTarget             = TEXT("FocusTarget");

	// --- Active order (mirror of tactics component) ------------------------
	inline const FName OrderType               = TEXT("OrderType");
	inline const FName OrderSequence           = TEXT("OrderSequence");
	inline const FName OrderTargetActor        = TEXT("OrderTargetActor");
	inline const FName OrderTargetLocation     = TEXT("OrderTargetLocation");
	// Legacy short name kept for SFCompanionAIController back-compat.
	inline const FName OrderTargetLoc          = TEXT("OrderTargetLoc");
	inline const FName OrderAbilityTag         = TEXT("OrderAbilityTag");
	inline const FName bHasValidOrder          = TEXT("bHasValidOrder");

	// --- Stance / aggression / range ---------------------------------------
	inline const FName Stance                  = TEXT("Stance");
	inline const FName Aggression              = TEXT("Aggression");
	inline const FName EngagementRange         = TEXT("EngagementRange");
	inline const FName StanceTag               = TEXT("StanceTag");

	// --- Movement / positioning --------------------------------------------
	inline const FName DesiredPosition         = TEXT("DesiredPosition");
	inline const FName DesiredCombatDistance   = TEXT("DesiredCombatDistance");
	inline const FName HomeAnchorLocation      = TEXT("HomeAnchorLocation");

	// --- Threshold / threat flags ------------------------------------------
	inline const FName bSelfLowHealth          = TEXT("bSelfLowHealth");
	inline const FName bPlayerLowHealth        = TEXT("bPlayerLowHealth");
	inline const FName bCanUseCommandedAbility = TEXT("bCanUseCommandedAbility");
	inline const FName bHasLineOfSight         = TEXT("bHasLineOfSight");
}
