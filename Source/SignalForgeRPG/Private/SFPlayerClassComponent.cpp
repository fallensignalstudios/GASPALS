// SFPlayerClassComponent.cpp

#include "Classes/SFPlayerClassComponent.h"

#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/SFGameplayAbility.h"
#include "Characters/SFCharacterBase.h"
#include "GameplayEffect.h"

DEFINE_LOG_CATEGORY_STATIC(LogSFClassComponent, Log, All);

// -----------------------------------------------------------------------------
// Construction
// -----------------------------------------------------------------------------

USFPlayerClassComponent::USFPlayerClassComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// -----------------------------------------------------------------------------
// Internal helpers
// -----------------------------------------------------------------------------

USFAbilitySystemComponent* USFPlayerClassComponent::GetASC() const
{
	if (const ASFCharacterBase* Character = GetOwnerCharacter())
	{
		return Character->GetSFAbilitySystemComponent();
	}
	return nullptr;
}

ASFCharacterBase* USFPlayerClassComponent::GetOwnerCharacter() const
{
	return Cast<ASFCharacterBase>(GetOwner());
}

bool USFPlayerClassComponent::IsInCombat() const
{
	// Hook this into your combat/state tag system when ready.
	if (const USFAbilitySystemComponent* ASC = GetASC())
	{
		// Example when you have a combat tag:
		// return ASC->HasMatchingGameplayTag(FSignalForgeGameplayTags::Get().State_InCombat);
	}
	return false;
}

bool USFPlayerClassComponent::MeetsLevelRequirement() const
{
	if (!CurrentClass)
	{
		return false;
	}

	// Hook into your progression component when ready.
	return true;
}

FSFDisciplineProgress* USFPlayerClassComponent::FindOrCreateProgress(
	USFDisciplineDefinition* Discipline)
{
	for (FSFDisciplineProgress& Progress : DisciplineProgressList)
	{
		if (Progress.Discipline == Discipline)
		{
			return &Progress;
		}
	}

	FSFDisciplineProgress& NewProgress = DisciplineProgressList.AddDefaulted_GetRef();
	NewProgress.Discipline = Discipline;
	return &NewProgress;
}

FSFDisciplineProgress* USFPlayerClassComponent::FindProgress(
	USFDisciplineDefinition* Discipline)
{
	for (FSFDisciplineProgress& Progress : DisciplineProgressList)
	{
		if (Progress.Discipline == Discipline)
		{
			return &Progress;
		}
	}
	return nullptr;
}

const FSFDisciplineProgress* USFPlayerClassComponent::FindProgress(
	USFDisciplineDefinition* Discipline) const
{
	for (const FSFDisciplineProgress& Progress : DisciplineProgressList)
	{
		if (Progress.Discipline == Discipline)
		{
			return &Progress;
		}
	}
	return nullptr;
}

const FSFDisciplineSkillNode* USFPlayerClassComponent::FindNode(
	USFDisciplineDefinition* Discipline,
	FName NodeID) const
{
	if (!Discipline)
	{
		return nullptr;
	}

	for (const FSFDisciplineSkillNode& Node : Discipline->SkillNodes)
	{
		if (Node.NodeID == NodeID)
		{
			return &Node;
		}
	}
	return nullptr;
}

// -----------------------------------------------------------------------------
// Class assignment
// -----------------------------------------------------------------------------

void USFPlayerClassComponent::AssignClass(
	USFClassDefinition* NewClass,
	bool bIsFirstTimeSetup)
{
	if (!NewClass)
	{
		UE_LOG(LogSFClassComponent, Warning,
			TEXT("AssignClass called with null class on %s."), *GetNameSafe(GetOwner()));
		return;
	}

	if (CurrentClass)
	{
		RemoveCurrentClass();
	}

	CurrentClass = NewClass;

	GrantClassAbilities();
	ApplyClassAttributeModifier();

	// Assign the default discipline automatically
	if (NewClass->DefaultDiscipline)
	{
		AssignDisciplineInternal(NewClass->DefaultDiscipline);
	}

	// Apply starting equipment only on a fresh save
	if (bIsFirstTimeSetup)
	{
		ASFCharacterBase* Character = GetOwnerCharacter();
		if (Character && Character->GetInventoryComponent())
		{
			for (const FSFStartingEquipmentEntry& Entry : NewClass->StartingEquipment)
			{
				if (Entry.ItemDefinition && Entry.Quantity > 0)
				{
					// Hook into your inventory add method here.
					// Character->GetInventoryComponent()->AddItem(Entry.ItemDefinition, Entry.Quantity);
					/*UE_LOG(LogSFClassComponent, Log,
						TEXT("Would grant starting item: %s x%d"),
						*GetNameSafe(Entry.ItemDefinition), Entry.Quantity);*/
				}
			}
		}
	}

	UE_LOG(LogSFClassComponent, Log,
		TEXT("%s: Class assigned — %s"),
		*GetNameSafe(GetOwner()),
		*NewClass->ClassDisplayName.ToString());

	OnClassAssigned.Broadcast(CurrentClass);
}

void USFPlayerClassComponent::RemoveCurrentClass()
{
	if (!CurrentClass)
	{
		return;
	}

	// Remove discipline first
	if (CurrentDiscipline)
	{
		RemoveDisciplinePassive();

		USFAbilitySystemComponent* ASC = GetASC();
		if (ASC && DisciplineAttributeModifierHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(DisciplineAttributeModifierHandle);
			DisciplineAttributeModifierHandle.Invalidate();
		}

		CurrentDiscipline = nullptr;
	}

	RemoveClassAbilities();
	RemoveClassAttributeModifier();

	DisciplineProgressList.Reset();
	CurrentClass = nullptr;

	UE_LOG(LogSFClassComponent, Log,
		TEXT("%s: Class removed."), *GetNameSafe(GetOwner()));
}

void USFPlayerClassComponent::GrantClassAbilities()
{
	ASFCharacterBase* Character = GetOwnerCharacter();
	if (!Character || !CurrentClass)
	{
		return;
	}

	ClassAbilityHandles.Reset();

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : CurrentClass->ActiveAbilities)
	{
		if (!AbilityClass)
		{
			continue;
		}

		FGameplayAbilitySpecHandle Handle = Character->GrantCharacterAbility(AbilityClass, 1);
		if (Handle.IsValid())
		{
			ClassAbilityHandles.Add(Handle);
		}
	}

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : CurrentClass->PassiveAbilities)
	{
		if (!AbilityClass)
		{
			continue;
		}

		FGameplayAbilitySpecHandle Handle = Character->GrantCharacterAbility(AbilityClass, 1);
		if (Handle.IsValid())
		{
			ClassAbilityHandles.Add(Handle);
		}
	}

	UE_LOG(LogSFClassComponent, Log,
		TEXT("%s: Granted %d class abilities."),
		*GetNameSafe(GetOwner()), ClassAbilityHandles.Num());
}

void USFPlayerClassComponent::RemoveClassAbilities()
{
	USFAbilitySystemComponent* ASC = GetASC();
	if (!ASC)
	{
		return;
	}

	for (const FGameplayAbilitySpecHandle& Handle : ClassAbilityHandles)
	{
		if (Handle.IsValid())
		{
			ASC->ClearAbility(Handle);
		}
	}

	ClassAbilityHandles.Reset();
}

void USFPlayerClassComponent::ApplyClassAttributeModifier()
{
	USFAbilitySystemComponent* ASC = GetASC();
	if (!ASC || !CurrentClass || !CurrentClass->ClassAttributeModifierEffect)
	{
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(GetOwner());

	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(
		CurrentClass->ClassAttributeModifierEffect, 1.0f, Context);

	if (Spec.IsValid())
	{
		ClassAttributeModifierHandle =
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

void USFPlayerClassComponent::RemoveClassAttributeModifier()
{
	USFAbilitySystemComponent* ASC = GetASC();
	if (!ASC)
	{
		return;
	}

	if (ClassAttributeModifierHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(ClassAttributeModifierHandle);
		ClassAttributeModifierHandle.Invalidate();
	}
}

// -----------------------------------------------------------------------------
// Discipline assignment
// -----------------------------------------------------------------------------

bool USFPlayerClassComponent::TrySwitchDiscipline(USFDisciplineDefinition* NewDiscipline)
{
	if (!NewDiscipline)
	{
		UE_LOG(LogSFClassComponent, Warning,
			TEXT("TrySwitchDiscipline called with null discipline."));
		return false;
	}

	if (!CurrentClass)
	{
		UE_LOG(LogSFClassComponent, Warning,
			TEXT("TrySwitchDiscipline called with no class assigned."));
		return false;
	}

	if (!CurrentClass->AvailableDisciplines.Contains(NewDiscipline))
	{
		UE_LOG(LogSFClassComponent, Warning,
			TEXT("Discipline %s is not available for class %s."),
			*NewDiscipline->DisciplineDisplayName.ToString(),
			*CurrentClass->ClassDisplayName.ToString());
		return false;
	}

	if (NewDiscipline == CurrentDiscipline)
	{
		return false;
	}

	if (IsInCombat())
	{
		UE_LOG(LogSFClassComponent, Log,
			TEXT("Cannot switch discipline during combat."));
		return false;
	}

	if (!MeetsLevelRequirement())
	{
		UE_LOG(LogSFClassComponent, Log,
			TEXT("Player does not meet level requirement to switch discipline."));
		return false;
	}

	AssignDisciplineInternal(NewDiscipline);
	return true;
}

void USFPlayerClassComponent::AssignDisciplineInternal(USFDisciplineDefinition* NewDiscipline)
{
	USFAbilitySystemComponent* ASC = GetASC();

	// Remove previous discipline modifier and passive
	if (CurrentDiscipline)
	{
		RemoveDisciplinePassive();

		if (ASC && DisciplineAttributeModifierHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(DisciplineAttributeModifierHandle);
			DisciplineAttributeModifierHandle.Invalidate();
		}

		// Remove node abilities from previous discipline
		if (FSFDisciplineProgress* OldProgress = FindProgress(CurrentDiscipline))
		{
			RemoveDisciplineNodeAbilities(*OldProgress);
		}
	}

	CurrentDiscipline = NewDiscipline;

	if (!CurrentDiscipline)
	{
		return;
	}

	// Apply discipline attribute modifier
	if (ASC && CurrentDiscipline->DisciplineAttributeModifierEffect)
	{
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(GetOwner());

		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(
			CurrentDiscipline->DisciplineAttributeModifierEffect, 1.0f, Context);

		if (Spec.IsValid())
		{
			DisciplineAttributeModifierHandle =
				ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	// Grant signature passive
	GrantDisciplinePassive(CurrentDiscipline);

	// Re-grant any previously purchased node abilities for this discipline
	FSFDisciplineProgress* Progress = FindOrCreateProgress(CurrentDiscipline);
	ASFCharacterBase* Character = GetOwnerCharacter();

	if (Character && Progress)
	{
		Progress->GrantedAbilityHandles.Reset();

		for (const FName& NodeID : Progress->PurchasedNodeIDs)
		{
			const FSFDisciplineSkillNode* Node = FindNode(CurrentDiscipline, NodeID);
			if (Node && Node->GrantedAbility)
			{
				FGameplayAbilitySpecHandle Handle =
					Character->GrantCharacterAbility(Node->GrantedAbility, 1);

				if (Handle.IsValid())
				{
					Progress->GrantedAbilityHandles.Add(Handle);
				}
			}
		}
	}

	UE_LOG(LogSFClassComponent, Log,
		TEXT("%s: Discipline assigned — %s"),
		*GetNameSafe(GetOwner()),
		*CurrentDiscipline->DisciplineDisplayName.ToString());

	OnDisciplineAssigned.Broadcast(CurrentDiscipline);
}

void USFPlayerClassComponent::GrantDisciplinePassive(USFDisciplineDefinition* Discipline)
{
	if (!Discipline || !Discipline->SignaturePassiveAbility)
	{
		return;
	}

	ASFCharacterBase* Character = GetOwnerCharacter();
	if (!Character)
	{
		return;
	}

	DisciplinePassiveHandle = Character->GrantCharacterAbility(
		Discipline->SignaturePassiveAbility, 1);
}

void USFPlayerClassComponent::RemoveDisciplinePassive()
{
	USFAbilitySystemComponent* ASC = GetASC();
	if (!ASC)
	{
		return;
	}

	if (DisciplinePassiveHandle.IsValid())
	{
		ASC->ClearAbility(DisciplinePassiveHandle);
		DisciplinePassiveHandle = FGameplayAbilitySpecHandle();
	}
}

void USFPlayerClassComponent::RemoveDisciplineNodeAbilities(FSFDisciplineProgress& Progress)
{
	USFAbilitySystemComponent* ASC = GetASC();
	if (!ASC)
	{
		return;
	}

	for (const FGameplayAbilitySpecHandle& Handle : Progress.GrantedAbilityHandles)
	{
		if (Handle.IsValid())
		{
			ASC->ClearAbility(Handle);
		}
	}

	Progress.GrantedAbilityHandles.Reset();
}

bool USFPlayerClassComponent::CanSwitchDiscipline() const
{
	return !IsInCombat() && MeetsLevelRequirement() && CurrentClass != nullptr;
}

// -----------------------------------------------------------------------------
// Skill node purchasing
// -----------------------------------------------------------------------------

bool USFPlayerClassComponent::TryPurchaseSkillNode(FName NodeID)
{
	if (!CurrentDiscipline)
	{
		UE_LOG(LogSFClassComponent, Warning,
			TEXT("TryPurchaseSkillNode: No discipline assigned."));
		return false;
	}

	const FSFDisciplineSkillNode* Node = FindNode(CurrentDiscipline, NodeID);
	if (!Node)
	{
		UE_LOG(LogSFClassComponent, Warning,
			TEXT("TryPurchaseSkillNode: Node %s not found."), *NodeID.ToString());
		return false;
	}

	FSFDisciplineProgress* Progress = FindOrCreateProgress(CurrentDiscipline);
	if (!Progress)
	{
		return false;
	}

	// Already purchased
	if (Progress->PurchasedNodeIDs.Contains(NodeID))
	{
		UE_LOG(LogSFClassComponent, Log,
			TEXT("Node %s already purchased."), *NodeID.ToString());
		return false;
	}

	// Tier unlock check
	if (Node->Tier == ESFDisciplineTier::Tier2 &&
		!CurrentDiscipline->AreTier1NodesComplete(Progress->PurchasedNodeIDs))
	{
		UE_LOG(LogSFClassComponent, Log,
			TEXT("Cannot purchase Tier 2 node — Tier 1 not complete."));
		return false;
	}

	if (Node->Tier == ESFDisciplineTier::Tier3 &&
		!CurrentDiscipline->AreTier2NodesComplete(Progress->PurchasedNodeIDs))
	{
		UE_LOG(LogSFClassComponent, Log,
			TEXT("Cannot purchase Tier 3 node — Tier 2 not complete."));
		return false;
	}

	// Point cost check
	if (!SpendDisciplinePoints(Node->DisciplinePointCost))
	{
		UE_LOG(LogSFClassComponent, Log,
			TEXT("Cannot purchase node %s — insufficient Discipline Points."),
			*NodeID.ToString());
		return false;
	}

	// Remove replaced ability if set
	if (Node->ReplacesAbility)
	{
		USFAbilitySystemComponent* ASC = GetASC();
		if (ASC)
		{
			FGameplayAbilitySpec* Spec =
				ASC->FindAbilitySpecFromClass(Node->ReplacesAbility);
			if (Spec)
			{
				ASC->ClearAbility(Spec->Handle);
			}
		}
	}

	// Grant the new ability
	ASFCharacterBase* Character = GetOwnerCharacter();
	if (Character && Node->GrantedAbility)
	{
		FGameplayAbilitySpecHandle Handle =
			Character->GrantCharacterAbility(Node->GrantedAbility, 1);

		if (Handle.IsValid())
		{
			Progress->GrantedAbilityHandles.Add(Handle);
		}
	}

	Progress->PurchasedNodeIDs.Add(NodeID);

	UE_LOG(LogSFClassComponent, Log,
		TEXT("%s: Purchased node %s in discipline %s."),
		*GetNameSafe(GetOwner()),
		*NodeID.ToString(),
		*CurrentDiscipline->DisciplineDisplayName.ToString());

	OnSkillNodePurchased.Broadcast(CurrentDiscipline, NodeID);
	return true;
}

bool USFPlayerClassComponent::IsNodePurchased(FName NodeID) const
{
	const FSFDisciplineProgress* Progress = FindProgress(CurrentDiscipline);
	if (!Progress)
	{
		return false;
	}
	return Progress->PurchasedNodeIDs.Contains(NodeID);
}

bool USFPlayerClassComponent::IsNodeAvailable(FName NodeID) const
{
	if (!CurrentDiscipline)
	{
		return false;
	}

	const FSFDisciplineSkillNode* Node = FindNode(CurrentDiscipline, NodeID);
	if (!Node)
	{
		return false;
	}

	if (IsNodePurchased(NodeID))
	{
		return false;
	}

	const FSFDisciplineProgress* Progress = FindProgress(CurrentDiscipline);
	const TArray<FName> EmptyList;
	const TArray<FName>& PurchasedIDs = Progress ? Progress->PurchasedNodeIDs : EmptyList;

	switch (Node->Tier)
	{
	case ESFDisciplineTier::Tier1:
		return true;
	case ESFDisciplineTier::Tier2:
		return CurrentDiscipline->AreTier1NodesComplete(PurchasedIDs);
	case ESFDisciplineTier::Tier3:
		return CurrentDiscipline->AreTier2NodesComplete(PurchasedIDs);
	default:
		return false;
	}
}

TArray<FName> USFPlayerClassComponent::GetPurchasedNodes() const
{
	const FSFDisciplineProgress* Progress = FindProgress(CurrentDiscipline);
	if (!Progress)
	{
		return TArray<FName>();
	}
	return Progress->PurchasedNodeIDs;
}

// -----------------------------------------------------------------------------
// Discipline Points
// -----------------------------------------------------------------------------

void USFPlayerClassComponent::AddDisciplinePoints(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	DisciplinePoints += Amount;

	UE_LOG(LogSFClassComponent, Log,
		TEXT("%s: Added %d Discipline Points. Total: %d"),
		*GetNameSafe(GetOwner()), Amount, DisciplinePoints);

	OnDisciplinePointsChanged.Broadcast(DisciplinePoints);
}

bool USFPlayerClassComponent::SpendDisciplinePoints(int32 Amount)
{
	if (Amount <= 0)
	{
		return true;
	}

	if (DisciplinePoints < Amount)
	{
		return false;
	}

	DisciplinePoints -= Amount;

	UE_LOG(LogSFClassComponent, Log,
		TEXT("%s: Spent %d Discipline Points. Remaining: %d"),
		*GetNameSafe(GetOwner()), Amount, DisciplinePoints);

	OnDisciplinePointsChanged.Broadcast(DisciplinePoints);
	return true;
}

void USFPlayerClassComponent::SetDisciplinePoints(int32 Amount)
{
	DisciplinePoints = FMath::Max(0, Amount);
	UE_LOG(LogSFClassComponent, Log,
		TEXT("%s: Discipline Points set to %d"),
		*GetNameSafe(GetOwner()), DisciplinePoints);

	OnDisciplinePointsChanged.Broadcast(DisciplinePoints);
}

// -----------------------------------------------------------------------------
// Save/Load helpers
// -----------------------------------------------------------------------------

void USFPlayerClassComponent::RestoreDisciplineProgress(
	const TArray<FSFDisciplineProgress>& SavedProgress)
{
	// Remove existing runtime handles
	for (FSFDisciplineProgress& Progress : DisciplineProgressList)
	{
		RemoveDisciplineNodeAbilities(Progress);
	}
	DisciplineProgressList.Reset();

	// Copy progress data by value
	DisciplineProgressList = SavedProgress;

	// Clear handles (they will be re‑granted)
	for (FSFDisciplineProgress& Progress : DisciplineProgressList)
	{
		Progress.GrantedAbilityHandles.Reset();
	}

	if (CurrentDiscipline)
	{
		AssignDisciplineInternal(CurrentDiscipline);
	}
}