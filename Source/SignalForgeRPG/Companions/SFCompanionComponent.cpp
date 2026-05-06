#include "Companions/SFCompanionComponent.h"
#include "Companions/SFCompanionCharacter.h"
#include "Companions/SFCompanionTacticsComponent.h"
#include "Core/SFPlayerState.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Narrative/SFNarrativeComponent.h"
#include "Narrative/SFNarrativeTypes.h"
#include "Net/UnrealNetwork.h"

USFCompanionComponent::USFCompanionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void USFCompanionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (USFNarrativeComponent* Narrative = GetPlayerNarrative())
		{
			Narrative->OnLoadComplete.AddDynamic(this, &USFCompanionComponent::HandleNarrativeLoadComplete);
		}
	}
}

void USFCompanionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (USFNarrativeComponent* Narrative = GetPlayerNarrative())
	{
		Narrative->OnLoadComplete.RemoveDynamic(this, &USFCompanionComponent::HandleNarrativeLoadComplete);
	}

	Super::EndPlay(EndPlayReason);
}

USFNarrativeComponent* USFCompanionComponent::GetPlayerNarrative() const
{
	if (const ASFPlayerState* PS = Cast<ASFPlayerState>(GetOwner()))
	{
		return PS->GetNarrativeComponent();
	}
	return nullptr;
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

	FSFCompanionRosterEntry* Entry = FindRosterEntry(CompanionId);
	if (Entry)
	{
		if (Entry->bRecruited)
		{
			return; // already in roster
		}
		Entry->bRecruited = true;
		Entry->bAvailable = true;
	}
	else
	{
		FSFCompanionRosterEntry NewEntry;
		NewEntry.CompanionId = CompanionId;
		NewEntry.Approval = 0;
		NewEntry.bRecruited = true;
		NewEntry.bAvailable = true;
		Roster.Add(NewEntry);
		Entry = &Roster.Last();
	}

	WriteEntryFacts(*Entry);
	OnRosterChanged.Broadcast();
}

void USFCompanionComponent::RecruitCompanionFromPawn(ASFCompanionCharacter* CompanionPawn)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !CompanionPawn) return;
	const FName Id = CompanionPawn->GetCompanionId();
	if (Id.IsNone()) return;

	RecruitCompanion(Id);

	if (FSFCompanionRosterEntry* Entry = FindRosterEntry(Id))
	{
		Entry->PersonalQuestAssetId = CompanionPawn->GetPersonalQuestAssetId();
		Entry->PersonalQuestApprovalThreshold = CompanionPawn->GetPersonalQuestApprovalThreshold();
		Entry->bAutoStartPersonalQuest = CompanionPawn->ShouldAutoStartPersonalQuest();

		WriteEntryFacts(*Entry);
		OnRosterChanged.Broadcast();
	}
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

	if (ASFCompanionCharacter* Active = ActiveCompanion)
	{
		if (USFCompanionTacticsComponent* Tactics = Active->GetTactics())
		{
			// Cooldown gate — silently drops spam without surprising the player.
			if (!Tactics->CanCommandAbility())
			{
				return;
			}

			FSFCompanionOrder Order;
			Order.Type = ESFCompanionOrderType::UseAbility;
			Order.AbilityTag = AbilityTag;
			Order.TargetActor = OptionalTarget;
			Tactics->IssueOrder(Order);
			Tactics->NotifyAbilityCommanded();
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

	CheckLoyaltyUnlock(*Entry);
	WriteEntryFacts(*Entry);

	OnRosterChanged.Broadcast();
}

int32 USFCompanionComponent::GetApproval(FName CompanionId) const
{
	const int32 Index = FindRosterIndex(CompanionId);
	return Index != INDEX_NONE ? Roster[Index].Approval : 0;
}

bool USFCompanionComponent::IsLoyaltyUnlocked(FName CompanionId) const
{
	const int32 Index = FindRosterIndex(CompanionId);
	return Index != INDEX_NONE && Roster[Index].bLoyaltyUnlocked;
}

// -----------------------------------------------------------------------------
// Loyalty unlock + fact persistence
// -----------------------------------------------------------------------------

void USFCompanionComponent::CheckLoyaltyUnlock(FSFCompanionRosterEntry& Entry)
{
	if (Entry.bLoyaltyUnlocked) return;
	if (Entry.Approval < Entry.PersonalQuestApprovalThreshold) return;

	Entry.bLoyaltyUnlocked = true;

	USFNarrativeComponent* Narrative = GetPlayerNarrative();
	if (!Narrative) return;

	// Set the loyalty-unlocked fact (ContextId = CompanionId).
	static const FGameplayTag LoyaltyTag = FGameplayTag::RequestGameplayTag(
		FName(TEXT("Fact.Companion.LoyaltyUnlocked")), /*ErrorIfNotFound*/ false);
	if (LoyaltyTag.IsValid())
	{
		FSFWorldFactKey Key;
		Key.FactTag = LoyaltyTag;
		Key.ContextId = Entry.CompanionId;
		Narrative->SetWorldFact(Key, FSFWorldFactValue::MakeBool(true));
	}

	// Optional auto-start of the personal quest.
	if (Entry.bAutoStartPersonalQuest && Entry.PersonalQuestAssetId.IsValid())
	{
		Narrative->StartQuestByAssetId(Entry.PersonalQuestAssetId);
	}
}

void USFCompanionComponent::WriteEntryFacts(const FSFCompanionRosterEntry& Entry)
{
	USFNarrativeComponent* Narrative = GetPlayerNarrative();
	if (!Narrative) return;

	static const FGameplayTag RecruitedTag = FGameplayTag::RequestGameplayTag(
		FName(TEXT("Fact.Companion.Recruited")), /*ErrorIfNotFound*/ false);
	static const FGameplayTag ApprovalTag = FGameplayTag::RequestGameplayTag(
		FName(TEXT("Fact.Companion.Approval")), /*ErrorIfNotFound*/ false);
	static const FGameplayTag LoyaltyTag = FGameplayTag::RequestGameplayTag(
		FName(TEXT("Fact.Companion.LoyaltyUnlocked")), /*ErrorIfNotFound*/ false);

	if (RecruitedTag.IsValid())
	{
		FSFWorldFactKey Key; Key.FactTag = RecruitedTag; Key.ContextId = Entry.CompanionId;
		Narrative->SetWorldFact(Key, FSFWorldFactValue::MakeBool(Entry.bRecruited));
	}
	if (ApprovalTag.IsValid())
	{
		FSFWorldFactKey Key; Key.FactTag = ApprovalTag; Key.ContextId = Entry.CompanionId;
		Narrative->SetWorldFact(Key, FSFWorldFactValue::MakeInt(Entry.Approval));
	}
	if (LoyaltyTag.IsValid())
	{
		FSFWorldFactKey Key; Key.FactTag = LoyaltyTag; Key.ContextId = Entry.CompanionId;
		Narrative->SetWorldFact(Key, FSFWorldFactValue::MakeBool(Entry.bLoyaltyUnlocked));
	}
}

void USFCompanionComponent::PersistRosterToNarrative()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	for (const FSFCompanionRosterEntry& Entry : Roster)
	{
		WriteEntryFacts(Entry);
	}
}

void USFCompanionComponent::RestoreRosterFromNarrative()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	USFNarrativeComponent* Narrative = GetPlayerNarrative();
	if (!Narrative) return;

	// We don't enumerate facts from outside the subsystem here; instead we
	// walk the roster we already have (recruits create entries up-front via
	// RecruitCompanion at narrative trigger time) and pull each entry's
	// values back out of the fact store. If the save loaded BEFORE any
	// roster was constructed in memory, designers should drive recruitment
	// from the same narrative deltas the save replays.
	static const FGameplayTag RecruitedTag = FGameplayTag::RequestGameplayTag(
		FName(TEXT("Fact.Companion.Recruited")), /*ErrorIfNotFound*/ false);
	static const FGameplayTag ApprovalTag = FGameplayTag::RequestGameplayTag(
		FName(TEXT("Fact.Companion.Approval")), /*ErrorIfNotFound*/ false);
	static const FGameplayTag LoyaltyTag = FGameplayTag::RequestGameplayTag(
		FName(TEXT("Fact.Companion.LoyaltyUnlocked")), /*ErrorIfNotFound*/ false);

	for (FSFCompanionRosterEntry& Entry : Roster)
	{
		FSFWorldFactValue OutValue;

		if (RecruitedTag.IsValid())
		{
			FSFWorldFactKey Key; Key.FactTag = RecruitedTag; Key.ContextId = Entry.CompanionId;
			if (Narrative->GetWorldFactValue(Key, OutValue))
			{
				Entry.bRecruited = OutValue.BoolValue;
			}
		}
		if (ApprovalTag.IsValid())
		{
			FSFWorldFactKey Key; Key.FactTag = ApprovalTag; Key.ContextId = Entry.CompanionId;
			if (Narrative->GetWorldFactValue(Key, OutValue))
			{
				Entry.Approval = OutValue.IntValue;
			}
		}
		if (LoyaltyTag.IsValid())
		{
			FSFWorldFactKey Key; Key.FactTag = LoyaltyTag; Key.ContextId = Entry.CompanionId;
			if (Narrative->GetWorldFactValue(Key, OutValue))
			{
				Entry.bLoyaltyUnlocked = OutValue.BoolValue;
			}
		}
	}

	OnRosterChanged.Broadcast();
}

void USFCompanionComponent::HandleNarrativeLoadComplete(const FString& /*SlotName*/)
{
	RestoreRosterFromNarrative();
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
