#include "AI/Companion/SFCompanionStates.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/SFGameplayAbility.h"
#include "AI/Companion/SFCompanionStateMachine.h"
#include "AI/SFCompanionAIController.h"
#include "Characters/SFCharacterBase.h"
#include "Companions/SFCompanionCharacter.h"
#include "Companions/SFCompanionTacticsComponent.h"
#include "Companions/SFCompanionTypes.h"
#include "Core/SignalForgeGameplayTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"

// =========================================================================
// Tunables — tweak per project, not per state. Designer-exposable later if
// needed (move to a UDataAsset or component).
// =========================================================================
namespace SFCompanionStateTuning
{
	// Follow / formation
	constexpr float FollowDesiredDistance     = 350.f;  // Target spacing from player
	constexpr float FollowMaxDistance         = 800.f;  // Beyond this, force a repath
	constexpr float FollowRepathInterval      = 0.5f;   // Seconds between repaths
	constexpr float FollowAcceptanceRadius    = 150.f;

	// Hold position
	constexpr float HoldEngageRadius          = 1500.f; // Threats inside this engage; outside: ignore

	// Move-to-location
	constexpr float MoveAcceptanceRadius      = 100.f;

	// Attack
	constexpr float AttackDefaultRange        = 800.f;
	constexpr float AttackInterval            = 1.0f;   // Seconds between commanded attacks
	constexpr float AttackChaseAcceptance     = 200.f;

	// Combat (autonomous)
	constexpr float CombatTargetScanInterval  = 0.5f;
	constexpr float CombatAggressiveScanRange = 2000.f;
	constexpr float CombatDefensiveScanRange  = 1200.f;

	// Retreat
	constexpr float RetreatTargetDistance     = 250.f;
	constexpr float RetreatAcceptanceRadius   = 300.f;
	constexpr float RetreatRepathInterval     = 0.5f;
}

// =========================================================================
// Local helpers
// =========================================================================
namespace
{
	APawn* GetPlayerPawnForMachine(USFCompanionStateMachine& Machine)
	{
		const ASFCompanionAIController* Controller = Machine.GetController();
		if (!Controller) { return nullptr; }
		return UGameplayStatics::GetPlayerPawn(Controller, 0);
	}

	bool IsActorAliveHostile(AActor* Actor)
	{
		if (!IsValid(Actor)) { return false; }
		if (const ASFCharacterBase* Char = Cast<ASFCharacterBase>(Actor))
		{
			return !Char->IsDead();
		}
		return true;
	}

	/** Issue a MoveToLocation through the AI controller if movement is possible. */
	void RequestMoveTo(ASFCompanionAIController* Controller, const FVector& Goal, float Acceptance)
	{
		if (!Controller) { return; }
		FAIMoveRequest Req;
		Req.SetGoalLocation(Goal);
		Req.SetAcceptanceRadius(Acceptance);
		Req.SetUsePathfinding(true);
		Req.SetAllowPartialPath(true);
		Controller->MoveTo(Req);
	}

	void StopMovement(ASFCompanionAIController* Controller)
	{
		if (!Controller) { return; }
		Controller->StopMovement();
	}

	/**
	 * Try to fire a commanded ability by gameplay tag. Returns true if the
	 * ASC actually activated something. Authority-only path; safe to call
	 * from server FSM tick.
	 */
	bool TryFireAbilityByTag(ASFCompanionCharacter* Companion, const FGameplayTag& Tag)
	{
		if (!Companion || !Tag.IsValid()) { return false; }

		UAbilitySystemComponent* ASC = Companion->GetAbilitySystemComponent();
		if (!ASC) { return false; }

		FScopedAbilityListLock Lock(*ASC);
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (!Spec.Ability) { continue; }
			const USFGameplayAbility* AsSF = Cast<USFGameplayAbility>(Spec.Ability);
			if (!AsSF) { continue; }
			if (AsSF->GetAssetTags().HasTagExact(Tag))
			{
				return ASC->TryActivateAbility(Spec.Handle);
			}
		}
		return false;
	}

}

// =========================================================================
// Idle
// =========================================================================
void FSFCompanionState_Idle::Enter(USFCompanionStateMachine& Machine)
{
	StopMovement(GetController(Machine));
}

void FSFCompanionState_Idle::Tick(USFCompanionStateMachine& /*Machine*/, float /*DeltaSeconds*/)
{
	// Idle is passive — transitions out are handled by the machine's
	// per-tick EvaluateDesiredState() call. Nothing to do here.
}

// =========================================================================
// Follow
// =========================================================================
void FSFCompanionState_Follow::Enter(USFCompanionStateMachine& Machine)
{
	RepathTimer = 0.f;
}

void FSFCompanionState_Follow::Tick(USFCompanionStateMachine& Machine, float DeltaSeconds)
{
	using namespace SFCompanionStateTuning;

	ASFCompanionAIController* Controller = GetController(Machine);
	ASFCompanionCharacter*    Companion  = GetCompanion(Machine);
	if (!Controller || !Companion) { return; }

	APawn* Player = GetPlayerPawnForMachine(Machine);
	if (!Player) { StopMovement(Controller); return; }

	const FVector CompanionLoc = Companion->GetActorLocation();
	const FVector PlayerLoc    = Player->GetActorLocation();
	const float   Distance     = FVector::Dist2D(CompanionLoc, PlayerLoc);

	RepathTimer -= DeltaSeconds;

	// Stay close: if we're inside the desired band, don't repath every frame.
	if (Distance <= FollowDesiredDistance + FollowAcceptanceRadius)
	{
		// In the comfort zone — issue a stop so we settle into idle anim.
		if (Controller->GetMoveStatus() == EPathFollowingStatus::Moving)
		{
			StopMovement(Controller);
		}
		return;
	}

	// Repath periodically or whenever we exceed the hard ceiling.
	if (RepathTimer <= 0.f || Distance > FollowMaxDistance)
	{
		// Aim a little inside the desired distance so we don't overshoot.
		const FVector ToCompanion = (CompanionLoc - PlayerLoc).GetSafeNormal2D();
		const FVector Goal = PlayerLoc + ToCompanion * FollowDesiredDistance;
		RequestMoveTo(Controller, Goal, FollowAcceptanceRadius);
		RepathTimer = FollowRepathInterval;
	}
}

void FSFCompanionState_Follow::Exit(USFCompanionStateMachine& Machine)
{
	// Cancel any in-flight follow path before the next state starts its own.
	StopMovement(GetController(Machine));
}

// =========================================================================
// HoldPosition (persistent)
// =========================================================================
void FSFCompanionState_HoldPosition::Enter(USFCompanionStateMachine& Machine)
{
	bAnchorValid = false;
	HoldAnchor = FVector::ZeroVector;

	if (USFCompanionTacticsComponent* Tactics = GetTactics(Machine))
	{
		const FSFCompanionOrder& Order = Tactics->GetActiveOrder();
		if (Order.Type == ESFCompanionOrderType::HoldPosition)
		{
			// Use the order location if the player painted one; otherwise
			// snap to wherever the companion is right now.
			if (!Order.TargetLocation.IsNearlyZero())
			{
				HoldAnchor = Order.TargetLocation;
				bAnchorValid = true;
			}
		}
	}

	if (!bAnchorValid)
	{
		if (ASFCompanionCharacter* Companion = GetCompanion(Machine))
		{
			HoldAnchor = Companion->GetActorLocation();
			bAnchorValid = true;
		}
	}

	if (bAnchorValid)
	{
		RequestMoveTo(GetController(Machine), HoldAnchor, SFCompanionStateTuning::MoveAcceptanceRadius);
	}
}

void FSFCompanionState_HoldPosition::Tick(USFCompanionStateMachine& Machine, float /*DeltaSeconds*/)
{
	using namespace SFCompanionStateTuning;

	if (!bAnchorValid) { return; }

	ASFCompanionAIController* Controller = GetController(Machine);
	ASFCompanionCharacter*    Companion  = GetCompanion(Machine);
	if (!Controller || !Companion) { return; }

	// If the companion has wandered (e.g. after a brief combat detour),
	// pull it back to the anchor.
	const float DistToAnchor = FVector::Dist2D(Companion->GetActorLocation(), HoldAnchor);
	if (DistToAnchor > HoldEngageRadius * 0.25f &&
		Controller->GetMoveStatus() != EPathFollowingStatus::Moving)
	{
		RequestMoveTo(Controller, HoldAnchor, MoveAcceptanceRadius);
	}

	// HoldPosition is persistent — the FSM never auto-clears the order.
	// The player must issue Follow / cancel to leave this state.
}

void FSFCompanionState_HoldPosition::Exit(USFCompanionStateMachine& Machine)
{
	StopMovement(GetController(Machine));
}

// =========================================================================
// MoveToLocation (consumed)
// =========================================================================
void FSFCompanionState_MoveToLocation::Enter(USFCompanionStateMachine& Machine)
{
	Destination = FVector::ZeroVector;
	OrderSequence = 0;

	if (USFCompanionTacticsComponent* Tactics = GetTactics(Machine))
	{
		const FSFCompanionOrder& Order = Tactics->GetActiveOrder();
		Destination = Order.TargetLocation;
		OrderSequence = Order.Sequence;
		Machine.SetLastHandledOrderSequence(OrderSequence);
	}

	RequestMoveTo(GetController(Machine), Destination, SFCompanionStateTuning::MoveAcceptanceRadius);
}

void FSFCompanionState_MoveToLocation::Tick(USFCompanionStateMachine& Machine, float /*DeltaSeconds*/)
{
	using namespace SFCompanionStateTuning;

	ASFCompanionAIController* Controller = GetController(Machine);
	ASFCompanionCharacter*    Companion  = GetCompanion(Machine);
	if (!Controller || !Companion) { return; }

	const float Dist = FVector::Dist2D(Companion->GetActorLocation(), Destination);
	if (Dist <= MoveAcceptanceRadius)
	{
		// Arrived — clear the order. Authority-only setter is safe here
		// because the FSM only ticks on the server.
		if (USFCompanionTacticsComponent* Tactics = GetTactics(Machine))
		{
			if (Companion->HasAuthority() &&
				Tactics->GetActiveOrder().Type == ESFCompanionOrderType::MoveToLocation)
			{
				Tactics->ClearOrder();
			}
		}
		return;
	}

	// Repath if path-follow stopped without reaching the goal.
	if (Controller->GetMoveStatus() != EPathFollowingStatus::Moving)
	{
		RequestMoveTo(Controller, Destination, MoveAcceptanceRadius);
	}
}

void FSFCompanionState_MoveToLocation::Exit(USFCompanionStateMachine& Machine)
{
	StopMovement(GetController(Machine));
}

// =========================================================================
// AttackTarget (consumed when target dies)
// =========================================================================
void FSFCompanionState_AttackTarget::Enter(USFCompanionStateMachine& Machine)
{
	Target = nullptr;
	AttackTimer = 0.f;
	OrderSequence = 0;

	if (USFCompanionTacticsComponent* Tactics = GetTactics(Machine))
	{
		const FSFCompanionOrder& Order = Tactics->GetActiveOrder();
		Target = Order.TargetActor;
		OrderSequence = Order.Sequence;
		Machine.SetLastHandledOrderSequence(OrderSequence);
	}
}

void FSFCompanionState_AttackTarget::Tick(USFCompanionStateMachine& Machine, float DeltaSeconds)
{
	using namespace SFCompanionStateTuning;

	ASFCompanionAIController* Controller = GetController(Machine);
	ASFCompanionCharacter*    Companion  = GetCompanion(Machine);
	USFCompanionTacticsComponent* Tactics = GetTactics(Machine);
	if (!Controller || !Companion || !Tactics) { return; }

	AActor* T = Target.Get();
	if (!IsActorAliveHostile(T))
	{
		// Target gone — clear the order so we fall back to Combat/Follow.
		if (Companion->HasAuthority() &&
			Tactics->GetActiveOrder().Type == ESFCompanionOrderType::AttackTarget)
		{
			Tactics->ClearOrder();
		}
		return;
	}

	const float Dist = FVector::Dist2D(Companion->GetActorLocation(), T->GetActorLocation());
	if (Dist > AttackDefaultRange)
	{
		// Chase.
		if (Controller->GetMoveStatus() != EPathFollowingStatus::Moving)
		{
			RequestMoveTo(Controller, T->GetActorLocation(), AttackChaseAcceptance);
		}
	}
	else
	{
		StopMovement(Controller);

		AttackTimer -= DeltaSeconds;
		if (AttackTimer <= 0.f)
		{
			Controller->SetFocus(T);
			TryFireAbilityByTag(Companion, FSignalForgeGameplayTags::Get().Ability_Attack_Light);
			AttackTimer = AttackInterval;
		}
	}
}

void FSFCompanionState_AttackTarget::Exit(USFCompanionStateMachine& Machine)
{
	if (ASFCompanionAIController* Controller = GetController(Machine))
	{
		Controller->ClearFocus(EAIFocusPriority::Gameplay);
		Controller->StopMovement();
	}
}

// =========================================================================
// FocusFire (persistent until target dies)
// =========================================================================
void FSFCompanionState_FocusFire::Enter(USFCompanionStateMachine& Machine)
{
	Target = nullptr;
	AttackTimer = 0.f;

	if (USFCompanionTacticsComponent* Tactics = GetTactics(Machine))
	{
		const FSFCompanionOrder& Order = Tactics->GetActiveOrder();
		Target = Order.TargetActor;
		Machine.SetLastHandledOrderSequence(Order.Sequence);
	}
}

void FSFCompanionState_FocusFire::Tick(USFCompanionStateMachine& Machine, float DeltaSeconds)
{
	using namespace SFCompanionStateTuning;

	ASFCompanionAIController* Controller = GetController(Machine);
	ASFCompanionCharacter*    Companion  = GetCompanion(Machine);
	USFCompanionTacticsComponent* Tactics = GetTactics(Machine);
	if (!Controller || !Companion || !Tactics) { return; }

	AActor* T = Target.Get();
	if (!IsActorAliveHostile(T))
	{
		// Target died — only now do we clear the persistent order.
		if (Companion->HasAuthority() &&
			Tactics->GetActiveOrder().Type == ESFCompanionOrderType::FocusFire)
		{
			Tactics->ClearOrder();
		}
		return;
	}

	const float Dist = FVector::Dist2D(Companion->GetActorLocation(), T->GetActorLocation());
	if (Dist > AttackDefaultRange)
	{
		if (Controller->GetMoveStatus() != EPathFollowingStatus::Moving)
		{
			RequestMoveTo(Controller, T->GetActorLocation(), AttackChaseAcceptance);
		}
	}
	else
	{
		StopMovement(Controller);

		AttackTimer -= DeltaSeconds;
		if (AttackTimer <= 0.f)
		{
			Controller->SetFocus(T);
			TryFireAbilityByTag(Companion, FSignalForgeGameplayTags::Get().Ability_Attack_Light);
			AttackTimer = AttackInterval;
		}
	}
}

void FSFCompanionState_FocusFire::Exit(USFCompanionStateMachine& Machine)
{
	if (ASFCompanionAIController* Controller = GetController(Machine))
	{
		Controller->ClearFocus(EAIFocusPriority::Gameplay);
		Controller->StopMovement();
	}
}

// =========================================================================
// UseAbility (one-shot, consumed)
// =========================================================================
void FSFCompanionState_UseAbility::Enter(USFCompanionStateMachine& Machine)
{
	ASFCompanionCharacter*        Companion = GetCompanion(Machine);
	USFCompanionTacticsComponent* Tactics   = GetTactics(Machine);
	if (!Companion || !Tactics) { return; }

	const FSFCompanionOrder& Order = Tactics->GetActiveOrder();
	Machine.SetLastHandledOrderSequence(Order.Sequence);

	const bool bAuthority = Companion->HasAuthority();
	const bool bGated     = !Tactics->CanCommandAbility();

	if (bGated)
	{
		// Cooldown still active — silently drop the order so we don't spin.
		if (bAuthority) { Tactics->ClearOrder(); }
		return;
	}

	const bool bFired = TryFireAbilityByTag(Companion, Order.AbilityTag);

	if (bAuthority)
	{
		if (bFired)
		{
			Tactics->NotifyAbilityCommanded();
		}
		// Always clear — failed activation shouldn't stick the FSM here.
		Tactics->ClearOrder();
	}
}

void FSFCompanionState_UseAbility::Tick(USFCompanionStateMachine& /*Machine*/, float /*DeltaSeconds*/)
{
	// No body — Enter() does the one-shot work and the order is cleared,
	// so the next EvaluateDesiredState() will move us out automatically.
}

// =========================================================================
// Combat (autonomous, no order)
// =========================================================================
void FSFCompanionState_Combat::Enter(USFCompanionStateMachine& Machine)
{
	CurrentTarget = nullptr;
	TargetScanTimer = 0.f;
	AttackTimer = 0.f;
}

void FSFCompanionState_Combat::Tick(USFCompanionStateMachine& Machine, float DeltaSeconds)
{
	using namespace SFCompanionStateTuning;

	ASFCompanionAIController* Controller = GetController(Machine);
	ASFCompanionCharacter*    Companion  = GetCompanion(Machine);
	USFCompanionTacticsComponent* Tactics = GetTactics(Machine);
	if (!Controller || !Companion || !Tactics) { return; }

	// Re-scan for a target periodically. This is the autonomous combat
	// path — perception integration plugs in here later.
	TargetScanTimer -= DeltaSeconds;
	if (TargetScanTimer <= 0.f || !IsActorAliveHostile(CurrentTarget.Get()))
	{
		TargetScanTimer = CombatTargetScanInterval;
		// Without perception wired up yet, we don't auto-acquire. The
		// machine will fall back to Follow/Idle on the next tick when
		// CurrentTarget stays null — that's the intended behavior until
		// perception lands.
	}

	AActor* T = CurrentTarget.Get();
	if (!IsActorAliveHostile(T))
	{
		return;
	}

	const float Dist = FVector::Dist2D(Companion->GetActorLocation(), T->GetActorLocation());
	if (Dist > AttackDefaultRange)
	{
		if (Controller->GetMoveStatus() != EPathFollowingStatus::Moving)
		{
			RequestMoveTo(Controller, T->GetActorLocation(), AttackChaseAcceptance);
		}
	}
	else
	{
		StopMovement(Controller);

		AttackTimer -= DeltaSeconds;
		if (AttackTimer <= 0.f)
		{
			Controller->SetFocus(T);
			TryFireAbilityByTag(Companion, FSignalForgeGameplayTags::Get().Ability_Attack_Light);
			AttackTimer = AttackInterval;
		}
	}
}

void FSFCompanionState_Combat::Exit(USFCompanionStateMachine& Machine)
{
	if (ASFCompanionAIController* Controller = GetController(Machine))
	{
		Controller->ClearFocus(EAIFocusPriority::Gameplay);
		Controller->StopMovement();
	}
}

// =========================================================================
// RetreatToPlayer
// =========================================================================
void FSFCompanionState_RetreatToPlayer::Enter(USFCompanionStateMachine& Machine)
{
	RepathTimer = 0.f;
}

void FSFCompanionState_RetreatToPlayer::Tick(USFCompanionStateMachine& Machine, float DeltaSeconds)
{
	using namespace SFCompanionStateTuning;

	ASFCompanionAIController* Controller = GetController(Machine);
	ASFCompanionCharacter*    Companion  = GetCompanion(Machine);
	USFCompanionTacticsComponent* Tactics = GetTactics(Machine);
	if (!Controller || !Companion || !Tactics) { return; }

	APawn* Player = GetPlayerPawnForMachine(Machine);
	if (!Player) { return; }

	RepathTimer -= DeltaSeconds;

	const FVector PlayerLoc    = Player->GetActorLocation();
	const FVector CompanionLoc = Companion->GetActorLocation();
	const float   Dist         = FVector::Dist2D(CompanionLoc, PlayerLoc);

	if (Dist <= RetreatAcceptanceRadius)
	{
		// Made it back. If this state was triggered by an order, clear it.
		if (Companion->HasAuthority() &&
			Tactics->GetActiveOrder().Type == ESFCompanionOrderType::RetreatToPlayer)
		{
			Tactics->ClearOrder();
		}
		// Threshold-driven retreats clear when bSelfLowHealth flips false;
		// the machine handles that on the next tick.
		return;
	}

	if (RepathTimer <= 0.f)
	{
		const FVector ToCompanion = (CompanionLoc - PlayerLoc).GetSafeNormal2D();
		const FVector Goal = PlayerLoc + ToCompanion * RetreatTargetDistance;
		RequestMoveTo(Controller, Goal, RetreatAcceptanceRadius);
		RepathTimer = RetreatRepathInterval;
	}
}

void FSFCompanionState_RetreatToPlayer::Exit(USFCompanionStateMachine& Machine)
{
	StopMovement(GetController(Machine));
}

// =========================================================================
// Stunned
// =========================================================================
void FSFCompanionState_Stunned::Enter(USFCompanionStateMachine& Machine)
{
	StopMovement(GetController(Machine));
	// Disable movement input on the character while stunned.
	if (ASFCompanionCharacter* Companion = GetCompanion(Machine))
	{
		if (UCharacterMovementComponent* Move = Companion->GetCharacterMovement())
		{
			Move->DisableMovement();
		}
	}
}

void FSFCompanionState_Stunned::Exit(USFCompanionStateMachine& Machine)
{
	if (ASFCompanionCharacter* Companion = GetCompanion(Machine))
	{
		if (UCharacterMovementComponent* Move = Companion->GetCharacterMovement())
		{
			Move->SetMovementMode(MOVE_Walking);
		}
	}
}

// =========================================================================
// Dead (terminal)
// =========================================================================
void FSFCompanionState_Dead::Enter(USFCompanionStateMachine& Machine)
{
	StopMovement(GetController(Machine));
	if (ASFCompanionAIController* Controller = GetController(Machine))
	{
		Controller->ClearFocus(EAIFocusPriority::Gameplay);
	}
}
