#pragma once

#include "CoreMinimal.h"
#include "SFCompanionAIState.generated.h"

/**
 * High-level AI state of a companion. Replicated from the controller so
 * client AnimBPs / UI can read it without round-tripping through the
 * tactics component.
 *
 * Order maps roughly 1:1 onto ESFCompanionOrderType, plus three states
 * the FSM owns autonomously (Idle, Combat, Stunned, Dead).
 */
UENUM(BlueprintType)
enum class ESFCompanionAIState : uint8
{
	Idle             UMETA(DisplayName = "Idle"),
	Follow           UMETA(DisplayName = "Follow"),
	HoldPosition     UMETA(DisplayName = "Hold Position"),
	MoveToLocation   UMETA(DisplayName = "Move To Location"),
	AttackTarget     UMETA(DisplayName = "Attack Target"),
	FocusFire        UMETA(DisplayName = "Focus Fire"),
	UseAbility       UMETA(DisplayName = "Use Ability"),
	Combat           UMETA(DisplayName = "Combat"),         // Autonomous combat with no order
	RetreatToPlayer  UMETA(DisplayName = "Retreat To Player"),
	Stunned          UMETA(DisplayName = "Stunned"),
	Dead             UMETA(DisplayName = "Dead"),
};
