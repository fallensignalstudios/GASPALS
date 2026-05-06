#include "Companions/SFCompanionComponent.h"
#include "Companions/SFCompanionCharacter.h"
#include "Companions/SFCompanionTacticsComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

USFCompanionComponent::USFCompanionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void USFCompanionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USFCompanionComponent, Roster);
	DOREPLIFETIME(USFCompanionComponent, ActiveCompanion);
}

// -------------------------------------------------------------------------
// Roster / recruitment
// -------------------------------------------------------------------------

int32 USFCompanionComponent::FindRosterIndex(FName CompanionId) const
{
	for (int32 i = 0; i < Roster.Num(); ++i)
	{
		if (Roster[i].CompanionId == CompanionId)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

FSFCompanionRosterEntry* USFCompanionComponent::FindRosterEntry(FName CompanionId)
{
	const int32 Index = FindRosterIndex(CompanionId);
	return Index != INDEX_NONE ? &Roster[Index] : nullptr;
}

void USFCompanionComponent::RecruitCompanion(FName CompanionId)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || CompanionId.IsNone())
	{
		return;
	}

	if (FSFCompanionRosterEntry* Existing = FindRosterEntry(CompanionId))
	{
		if (Existing->bRecruited)
		{
			return; // already in roster
		}
		Existing->bRecruited = true;
		Existing->bAvailable = true;
	}
	else
	{
		FSFCompanionRosterEntry Entry;
		Entry.CompanionId = CompanionId;
		Entry.Approval = 0;
		Entry.bRecruited = true;
		Entry.bAvailable = true;
		Roster.Add(Entry);
	}

	OnRosterChanged.Broadcast();
}

void USFCompanionComponent::DismissCompanion(FName CompanionId)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || CompanionId.IsNone())
	{
		return;
	}

	FSFCompanionRosterEntry* Entry = FindRosterEntry(CompanionId);
	if (!Entry || !Entry->bRecruited)
	{
		return;
	}

	Entry->bRecruited = false;

	// If the dismissed companion is currently active, clear active.
	if (ASFCompanionCharacter* Active = ActiveCompanion)
	{
		if (Active->GetCompanionId() == CompanionId)
		{
			SetActiveCompanion(nullptr);
		}
	}

	OnRosterChanged.Broadcast();
}

bool USFCompanionComponent::IsRecruited(FName CompanionId) const
{
	const int32 Index = FindRosterIndex(CompanionId);
	return Index != INDEX_NONE && Roster[Index].bRecruited;
}

// -------------------------------------------------------------------------
// Active companion
// -------------------------------------------------------------------------

void USFCompanionComponent::SetActiveCompanion(ASFCompanionCharacter* NewActive)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	ASFCompanionCharacter* OldActive = ActiveCompanion;
	if (OldActive == NewActive)
	{
		return;
	}

	ActiveCompanion = NewActive;
	OnActiveCompanionChanged.Broadcast(OldActive, NewActive);
}

// -------------------------------------------------------------------------
// Player command surface — forward to active companion's tactics component
// -------------------------------------------------------------------------

void USFCompanionComponent::CommandSetStance(ESFCompanionStance NewStance)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	if (ASFCompanionCharacter* Active = ActiveCompanion.Get())
	{
		if (USFCompanionTacticsComponent* Tactics = Active->GetTactics())
		{
			Tactics->SetStance(NewStance);
		}
	}
}

void USFCompanionComponent::CommandSetAggression(ESFCompanionAggression NewAggression)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	if (ASFCompanionCharacter* Active = ActiveCompanion.Get())
	{
		if (USFCompanionTacticsComponent* Tactics = Active->GetTactics())
		{
			Tactics->SetAggression(NewAggression);
		}
	}
}

void USFCompanionComponent::CommandFollow()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	if (ASFCompanionCharacter* Active = ActiveCompanion.Get())
	{
		if (USFCompanionTacticsComponent* Tactics = Active->GetTactics())
		{
			FSFCompanionOrder Order;
			Order.Type = ESFCompanionOrderType::Follow;
			Tactics->IssueOrder(Order);
		}
	}
}

void USFCompanionComponent::CommandHoldPosition()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	if (ASFCompanionCharacter* Active = ActiveCompanion.Get())
	{
		if (USFCompanionTacticsComponent* Tactics = Active->GetTactics())
		{
			FSFCompanionOrder Order;
			Order.Type = ESFCompanionOrderType::HoldPosition;
			Order.TargetLocation = Active->GetActorLocation();
			Tactics->IssueOrder(Order);
		}
	}
}

void USFCompanionComponent::CommandMoveTo(FVector Location)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	if (ASFCompanionCharacter* Active = ActiveCompanion.Get())
	{
		if (USFCompanionTacticsComponent* Tactics = Active->GetTactics())
		{
			FSFCompanionOrder Order;
			Order.Type = ESFCompanionOrderType::MoveToLocation;
			Order.TargetLocation = Location;
			Tactics->IssueOrder(Order);
		}
	}
}

void USFCompanionComponent::CommandAttackTarget(AActor* Target)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Target) return;

	if (ASFCompanionCharacter* Active = ActiveCompanion.Get())
	{
		if (USFCompanionTacticsComponent* Tactics = Active->GetTactics())
		{
			FSFCompanionOrder Order;
			Order.Type = ESFCompanionOrderType::AttackTarget;
			Order.TargetActor = Target;
			Tactics->IssueOrder(Order);
		}
	}
}

void USFCompanionComponent::CommandFocusFire(AActor* Target)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Target) return;

	if (ASFCompanionCharacter* Active = ActiveCompanion.Get())
	{
		if (USFCompanionTacticsComponent* Tactics = Active->GetTactics())
		{
			FSFCompanionOrder Order;
			Order.Type = ESFCompanionOrderType::FocusFire;
			Order.TargetActor = Target;
			Tactics->IssueOrder(Order);
		}
	}
}

void USFCompanionComponent::CommandUseAbility(FGameplayTag AbilityTag, AActor* OptionalTarget)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !AbilityTag.IsValid()) return;

	if (ASFCompanionCharacter* Active = ActiveCompanion.Get())
	{
		if (USFCompanionTacticsComponent* Tactics = Active->GetTactics())
		{
			FSFCompanionOrder Order;
			Order.Type = ESFCompanionOrderType::UseAbility;
			Order.AbilityTag = AbilityTag;
			Order.TargetActor = OptionalTarget;
			Tactics->IssueOrder(Order);
		}
	}
}

void USFCompanionComponent::CommandRetreatToPlayer()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	if (ASFCompanionCharacter* Active = ActiveCompanion.Get())
	{
		if (USFCompanionTacticsComponent* Tactics = Active->GetTactics())
		{
			FSFCompanionOrder Order;
			Order.Type = ESFCompanionOrderType::RetreatToPlayer;
			Tactics->IssueOrder(Order);
		}
	}
}

// -------------------------------------------------------------------------
// Radial wheel — slow-mo enter/exit (local-only by design)
// -------------------------------------------------------------------------

void USFCompanionComponent::BeginCommandMode()
{
	if (bInCommandMode) return;

	bInCommandMode = true;

	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, CommandModeTimeDilation);
	}
}

void USFCompanionComponent::EndCommandMode()
{
	if (!bInCommandMode) return;

	bInCommandMode = false;

	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, 1.0f);
	}
}

// -------------------------------------------------------------------------
// Approval
// -------------------------------------------------------------------------

void USFCompanionComponent::ChangeApproval(FName CompanionId, int32 Delta)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || CompanionId.IsNone() || Delta == 0)
	{
		return;
	}

	FSFCompanionRosterEntry* Entry = FindRosterEntry(CompanionId);
	if (!Entry)
	{
		// First approval change for an unknown companion — implicitly create
		// a roster entry in the unrecruited state so we can remember the
		// sentiment until they're recruited later.
		FSFCompanionRosterEntry NewEntry;
		NewEntry.CompanionId = CompanionId;
		NewEntry.Approval = 0;
		NewEntry.bRecruited = false;
		NewEntry.bAvailable = true;
		Roster.Add(NewEntry);
		Entry = &Roster.Last();
	}

	const int32 OldApproval = Entry->Approval;
	Entry->Approval = OldApproval + Delta;

	OnApprovalChanged.Broadcast(CompanionId, OldApproval, Entry->Approval);
	OnRosterChanged.Broadcast();
}

int32 USFCompanionComponent::GetApproval(FName CompanionId) const
{
	const int32 Index = FindRosterIndex(CompanionId);
	return Index != INDEX_NONE ? Roster[Index].Approval : 0;
}

// -------------------------------------------------------------------------
// OnRep
// -------------------------------------------------------------------------

void USFCompanionComponent::OnRep_Roster()
{
	OnRosterChanged.Broadcast();
}

void USFCompanionComponent::OnRep_ActiveCompanion(ASFCompanionCharacter* OldActive)
{
	OnActiveCompanionChanged.Broadcast(OldActive, ActiveCompanion);
}
