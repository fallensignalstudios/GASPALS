#include "UI/SFCompanionCommandController.h"

#include "UI/SFCompanionRadialWheel.h"
#include "Companions/SFCompanionCharacter.h"
#include "Companions/SFCompanionComponent.h"
#include "Companions/SFCompanionTacticsComponent.h"
#include "Core/SFPlayerState.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

USFCompanionCommandController::USFCompanionCommandController()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USFCompanionCommandController::BeginPlay()
{
	Super::BeginPlay();
}

void USFCompanionCommandController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bWheelOpen)
	{
		CloseWheel();
	}
	if (WheelWidget)
	{
		WheelWidget->RemoveFromParent();
		WheelWidget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

APlayerController* USFCompanionCommandController::GetOwningPC() const
{
	return Cast<APlayerController>(GetOwner());
}

USFCompanionComponent* USFCompanionCommandController::GetCompanionComponent() const
{
	APlayerController* PC = GetOwningPC();
	if (!PC) return nullptr;
	ASFPlayerState* PS = PC->GetPlayerState<ASFPlayerState>();
	return PS ? PS->GetCompanionComponent() : nullptr;
}

ASFCompanionCharacter* USFCompanionCommandController::GetActiveCompanion() const
{
	const USFCompanionComponent* Comp = GetCompanionComponent();
	return Comp ? Comp->GetActiveCompanion() : nullptr;
}

// ---------------------------------------------------------------------------
// Open / close
// ---------------------------------------------------------------------------

void USFCompanionCommandController::OpenWheel()
{
	if (bWheelOpen) return;

	APlayerController* PC = GetOwningPC();
	if (!PC || !PC->IsLocalController()) return;

	if (!WheelWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("USFCompanionCommandController: WheelWidgetClass not set"));
		return;
	}

	// Slow-mo on the companion component (local time dilation; safe on client).
	if (USFCompanionComponent* Comp = GetCompanionComponent())
	{
		Comp->BeginCommandMode();
	}

	if (!WheelWidget)
	{
		WheelWidget = CreateWidget<USFCompanionRadialWheel>(PC, WheelWidgetClass);
		if (WheelWidget)
		{
			WheelWidget->OnSlotConfirmed.AddDynamic(this, &USFCompanionCommandController::HandleSlotConfirmed);
			WheelWidget->OnWheelClosed.AddDynamic(this, &USFCompanionCommandController::HandleWheelClosed);
		}
	}

	if (!WheelWidget) return;

	if (!WheelWidget->IsInViewport())
	{
		WheelWidget->AddToViewport(WheelZOrder);
	}

	bWheelOpen = true;
	CurrentPage = ESFRadialWheelPage::Root;
	PreviousPage = ESFRadialWheelPage::Root;
	PendingMoveLocation = FVector::ZeroVector;
	bPendingMoveValid = false;
	PendingTarget.Reset();

	RefreshSlotsForCurrentPage();
}

void USFCompanionCommandController::CloseWheel()
{
	if (!bWheelOpen) return;
	bWheelOpen = false;

	if (WheelWidget && WheelWidget->IsInViewport())
	{
		WheelWidget->RemoveFromParent();
	}

	if (USFCompanionComponent* Comp = GetCompanionComponent())
	{
		Comp->EndCommandMode();
	}
}

void USFCompanionCommandController::UpdateSelection(FVector2D Selection)
{
	if (!bWheelOpen || !WheelWidget) return;
	WheelWidget->UpdateSelectionFromVector(Selection);
}

void USFCompanionCommandController::ConfirmHovered()
{
	if (!bWheelOpen || !WheelWidget) return;
	WheelWidget->ConfirmHovered();
}

// ---------------------------------------------------------------------------
// Page nav
// ---------------------------------------------------------------------------

void USFCompanionCommandController::GoToPage(ESFRadialWheelPage NewPage)
{
	if (!bWheelOpen) return;
	PreviousPage = CurrentPage;
	CurrentPage = NewPage;
	RefreshSlotsForCurrentPage();
}

void USFCompanionCommandController::GoBack()
{
	if (!bWheelOpen) return;
	if (CurrentPage == ESFRadialWheelPage::Root)
	{
		CloseWheel();
		return;
	}
	const ESFRadialWheelPage Target = PreviousPage;
	PreviousPage = ESFRadialWheelPage::Root;
	CurrentPage = Target;
	RefreshSlotsForCurrentPage();
}

void USFCompanionCommandController::SetPendingMoveLocation(FVector InLocation, bool bInValid)
{
	PendingMoveLocation = InLocation;
	bPendingMoveValid = bInValid;
}

void USFCompanionCommandController::SetPendingTargetActor(AActor* InTarget)
{
	PendingTarget = InTarget;
}

// ---------------------------------------------------------------------------
// Slot dispatch
// ---------------------------------------------------------------------------

void USFCompanionCommandController::HandleSlotConfirmed(int32 SlotIndex, const FSFRadialWheelSlot& Slot)
{
	DispatchSlot(Slot);
}

void USFCompanionCommandController::HandleWheelClosed()
{
	CloseWheel();
}

void USFCompanionCommandController::DispatchSlot(const FSFRadialWheelSlot& Slot)
{
	switch (Slot.Action)
	{
	case ESFRadialWheelAction::Cancel:
		CloseWheel();
		return;

	case ESFRadialWheelAction::GoBack:
		GoBack();
		return;

	case ESFRadialWheelAction::OpenPage:
		GoToPage(Slot.TargetPage);
		return;

	case ESFRadialWheelAction::SetStance:
		if (USFCompanionComponent* Comp = GetCompanionComponent())
		{
			Comp->CommandSetStance(Slot.Stance);
		}
		CloseWheel();
		return;

	case ESFRadialWheelAction::SetAggression:
		if (USFCompanionComponent* Comp = GetCompanionComponent())
		{
			Comp->CommandSetAggression(Slot.Aggression);
		}
		CloseWheel();
		return;

	case ESFRadialWheelAction::IssueOrder:
		if (USFCompanionComponent* Comp = GetCompanionComponent())
		{
			switch (Slot.OrderType)
			{
			case ESFCompanionOrderType::Follow:
				Comp->CommandFollow();
				break;
			case ESFCompanionOrderType::HoldPosition:
				Comp->CommandHoldPosition();
				break;
			case ESFCompanionOrderType::MoveToLocation:
				if (bPendingMoveValid)
				{
					Comp->CommandMoveTo(PendingMoveLocation);
				}
				else
				{
					UE_LOG(LogTemp, Verbose, TEXT("Companion radial: MoveTo issued without a valid pending location"));
				}
				break;
			case ESFCompanionOrderType::AttackTarget:
				Comp->CommandAttackTarget(PendingTarget.Get());
				break;
			case ESFCompanionOrderType::FocusFire:
				Comp->CommandFocusFire(PendingTarget.Get());
				break;
			case ESFCompanionOrderType::RetreatToPlayer:
				Comp->CommandRetreatToPlayer();
				break;
			case ESFCompanionOrderType::UseAbility:
				Comp->CommandUseAbility(Slot.AbilityTag, PendingTarget.Get());
				break;
			default:
				break;
			}
		}
		CloseWheel();
		return;

	case ESFRadialWheelAction::UseAbility:
		if (USFCompanionComponent* Comp = GetCompanionComponent())
		{
			Comp->CommandUseAbility(Slot.AbilityTag, PendingTarget.Get());
		}
		CloseWheel();
		return;

	case ESFRadialWheelAction::None:
	default:
		return;
	}
}

// ---------------------------------------------------------------------------
// Slot population
// ---------------------------------------------------------------------------

void USFCompanionCommandController::RefreshSlotsForCurrentPage()
{
	if (!WheelWidget) return;
	TArray<FSFRadialWheelSlot> Out;
	BuildSlotsForPage(CurrentPage, Out);
	WheelWidget->SetSlots(Out);
}

void USFCompanionCommandController::BuildSlotsForPage(ESFRadialWheelPage Page, TArray<FSFRadialWheelSlot>& OutSlots) const
{
	OutSlots.Reset();
	switch (Page)
	{
	case ESFRadialWheelPage::Root:        BuildRootSlots(OutSlots); break;
	case ESFRadialWheelPage::Stance:      BuildStanceSlots(OutSlots); break;
	case ESFRadialWheelPage::Aggression:  BuildAggressionSlots(OutSlots); break;
	case ESFRadialWheelPage::Order:       BuildOrderSlots(OutSlots); break;
	case ESFRadialWheelPage::Ability:     BuildAbilitySlots(OutSlots); break;
	default: break;
	}
}

void USFCompanionCommandController::BuildRootSlots(TArray<FSFRadialWheelSlot>& OutSlots) const
{
	const bool bHasCompanion = (GetActiveCompanion() != nullptr);

	{
		FSFRadialWheelSlot S;
		S.Action = ESFRadialWheelAction::OpenPage;
		S.TargetPage = ESFRadialWheelPage::Order;
		S.Label = NSLOCTEXT("Radial", "Orders", "Orders");
		S.Description = NSLOCTEXT("Radial", "OrdersDesc", "Move, hold, attack, focus fire");
		S.bEnabled = bHasCompanion;
		OutSlots.Add(S);
	}
	{
		FSFRadialWheelSlot S;
		S.Action = ESFRadialWheelAction::OpenPage;
		S.TargetPage = ESFRadialWheelPage::Stance;
		S.Label = NSLOCTEXT("Radial", "Stance", "Stance");
		S.Description = NSLOCTEXT("Radial", "StanceDesc", "Tank / DPS / Healer");
		S.bEnabled = bHasCompanion;
		OutSlots.Add(S);
	}
	{
		FSFRadialWheelSlot S;
		S.Action = ESFRadialWheelAction::OpenPage;
		S.TargetPage = ESFRadialWheelPage::Ability;
		S.Label = NSLOCTEXT("Radial", "Ability", "Abilities");
		S.Description = NSLOCTEXT("Radial", "AbilityDesc", "Trigger a tagged ability");
		S.bEnabled = bHasCompanion && AbilitySlots.Num() > 0;
		OutSlots.Add(S);
	}
	{
		FSFRadialWheelSlot S;
		S.Action = ESFRadialWheelAction::OpenPage;
		S.TargetPage = ESFRadialWheelPage::Aggression;
		S.Label = NSLOCTEXT("Radial", "Aggression", "Aggression");
		S.Description = NSLOCTEXT("Radial", "AggressionDesc", "Passive / Defensive / Aggressive");
		S.bEnabled = bHasCompanion;
		OutSlots.Add(S);
	}
	{
		FSFRadialWheelSlot S;
		S.Action = ESFRadialWheelAction::Cancel;
		S.Label = NSLOCTEXT("Radial", "Cancel", "Close");
		OutSlots.Add(S);
	}
}

void USFCompanionCommandController::BuildStanceSlots(TArray<FSFRadialWheelSlot>& OutSlots) const
{
	ESFCompanionStance Current = ESFCompanionStance::DPS;
	if (const ASFCompanionCharacter* Comp = GetActiveCompanion())
	{
		if (const USFCompanionTacticsComponent* Tactics = Comp->GetTactics())
		{
			Current = Tactics->GetStance();
		}
	}

	auto Add = [&](ESFCompanionStance Stance, const FText& Label, const FText& Desc)
	{
		FSFRadialWheelSlot S;
		S.Action = ESFRadialWheelAction::SetStance;
		S.Stance = Stance;
		S.Label = Label;
		S.Description = Desc;
		S.bIsCurrent = (Stance == Current);
		OutSlots.Add(S);
	};

	Add(ESFCompanionStance::Tank,   NSLOCTEXT("Radial", "StanceTank",   "Tank"),
		NSLOCTEXT("Radial", "StanceTankDesc",   "Hold the line, taunt, peel"));
	Add(ESFCompanionStance::DPS,    NSLOCTEXT("Radial", "StanceDPS",    "DPS"),
		NSLOCTEXT("Radial", "StanceDPSDesc",    "Flank, focus highest threat"));
	Add(ESFCompanionStance::Healer, NSLOCTEXT("Radial", "StanceHealer", "Healer"),
		NSLOCTEXT("Radial", "StanceHealerDesc", "Stay near player, support"));

	{
		FSFRadialWheelSlot Back;
		Back.Action = ESFRadialWheelAction::GoBack;
		Back.Label = NSLOCTEXT("Radial", "Back", "Back");
		OutSlots.Add(Back);
	}
}

void USFCompanionCommandController::BuildAggressionSlots(TArray<FSFRadialWheelSlot>& OutSlots) const
{
	ESFCompanionAggression Current = ESFCompanionAggression::Defensive;
	if (const ASFCompanionCharacter* Comp = GetActiveCompanion())
	{
		if (const USFCompanionTacticsComponent* Tactics = Comp->GetTactics())
		{
			Current = Tactics->GetAggression();
		}
	}

	auto Add = [&](ESFCompanionAggression Agg, const FText& Label, const FText& Desc)
	{
		FSFRadialWheelSlot S;
		S.Action = ESFRadialWheelAction::SetAggression;
		S.Aggression = Agg;
		S.Label = Label;
		S.Description = Desc;
		S.bIsCurrent = (Agg == Current);
		OutSlots.Add(S);
	};

	Add(ESFCompanionAggression::Passive,    NSLOCTEXT("Radial", "AggPassive",    "Passive"),
		NSLOCTEXT("Radial", "AggPassiveDesc",    "Don't engage unless attacked"));
	Add(ESFCompanionAggression::Defensive,  NSLOCTEXT("Radial", "AggDefensive",  "Defensive"),
		NSLOCTEXT("Radial", "AggDefensiveDesc",  "Engage threats to player or self"));
	Add(ESFCompanionAggression::Aggressive, NSLOCTEXT("Radial", "AggAggressive", "Aggressive"),
		NSLOCTEXT("Radial", "AggAggressiveDesc", "Engage anything hostile in range"));

	{
		FSFRadialWheelSlot Back;
		Back.Action = ESFRadialWheelAction::GoBack;
		Back.Label = NSLOCTEXT("Radial", "Back", "Back");
		OutSlots.Add(Back);
	}
}

void USFCompanionCommandController::BuildOrderSlots(TArray<FSFRadialWheelSlot>& OutSlots) const
{
	auto AddOrder = [&](ESFCompanionOrderType Type, const FText& Label, const FText& Desc, bool bEnabled = true)
	{
		FSFRadialWheelSlot S;
		S.Action = ESFRadialWheelAction::IssueOrder;
		S.OrderType = Type;
		S.Label = Label;
		S.Description = Desc;
		S.bEnabled = bEnabled;
		OutSlots.Add(S);
	};

	const bool bHasTarget = PendingTarget.IsValid();

	AddOrder(ESFCompanionOrderType::Follow,
		NSLOCTEXT("Radial", "OrderFollow", "Follow"),
		NSLOCTEXT("Radial", "OrderFollowDesc", "Stay on me"));

	AddOrder(ESFCompanionOrderType::HoldPosition,
		NSLOCTEXT("Radial", "OrderHold", "Hold"),
		NSLOCTEXT("Radial", "OrderHoldDesc", "Anchor on this position"));

	AddOrder(ESFCompanionOrderType::MoveToLocation,
		NSLOCTEXT("Radial", "OrderMove", "Move To"),
		NSLOCTEXT("Radial", "OrderMoveDesc", "Move to the marked spot"),
		bPendingMoveValid);

	AddOrder(ESFCompanionOrderType::AttackTarget,
		NSLOCTEXT("Radial", "OrderAttack", "Attack"),
		NSLOCTEXT("Radial", "OrderAttackDesc", "Engage the marked target"),
		bHasTarget);

	AddOrder(ESFCompanionOrderType::FocusFire,
		NSLOCTEXT("Radial", "OrderFocus", "Focus Fire"),
		NSLOCTEXT("Radial", "OrderFocusDesc", "Lock onto target until down"),
		bHasTarget);

	AddOrder(ESFCompanionOrderType::RetreatToPlayer,
		NSLOCTEXT("Radial", "OrderRetreat", "Retreat"),
		NSLOCTEXT("Radial", "OrderRetreatDesc", "Fall back to me"));

	{
		FSFRadialWheelSlot Back;
		Back.Action = ESFRadialWheelAction::GoBack;
		Back.Label = NSLOCTEXT("Radial", "Back", "Back");
		OutSlots.Add(Back);
	}
}

void USFCompanionCommandController::BuildAbilitySlots(TArray<FSFRadialWheelSlot>& OutSlots) const
{
	const ASFCompanionCharacter* Comp = GetActiveCompanion();
	const USFCompanionTacticsComponent* Tactics = Comp ? Comp->GetTactics() : nullptr;
	const bool bCooldownReady = Tactics ? Tactics->CanCommandAbility() : true;

	for (const FSFRadialWheelSlot& Designer : AbilitySlots)
	{
		FSFRadialWheelSlot S = Designer;
		// Force action to UseAbility regardless of designer config to keep dispatch deterministic.
		S.Action = ESFRadialWheelAction::UseAbility;
		// Block confirmation while the cooldown is hot.
		if (!bCooldownReady)
		{
			S.bEnabled = false;
		}
		OutSlots.Add(S);
	}

	{
		FSFRadialWheelSlot Back;
		Back.Action = ESFRadialWheelAction::GoBack;
		Back.Label = NSLOCTEXT("Radial", "Back", "Back");
		OutSlots.Add(Back);
	}
}
