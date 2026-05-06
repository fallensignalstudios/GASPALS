#include "Companions/SFCompanionTacticsComponent.h"

#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

USFCompanionTacticsComponent::USFCompanionTacticsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void USFCompanionTacticsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USFCompanionTacticsComponent, Stance);
	DOREPLIFETIME(USFCompanionTacticsComponent, Aggression);
	DOREPLIFETIME(USFCompanionTacticsComponent, EngagementRange);
	DOREPLIFETIME(USFCompanionTacticsComponent, ActiveOrder);
}

// -----------------------------------------------------------------------------
// Stance / aggression / engagement
// -----------------------------------------------------------------------------

void USFCompanionTacticsComponent::SetStance(ESFCompanionStance NewStance)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (NewStance == Stance)
	{
		return;
	}

	const ESFCompanionStance OldStance = Stance;
	Stance = NewStance;

	RefreshStanceTags();
	ApplyStanceTagsToAbilitySystem();

	OnStanceChanged.Broadcast(OldStance, NewStance);
}

void USFCompanionTacticsComponent::SetAggression(ESFCompanionAggression NewAggression)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	Aggression = NewAggression;
}

void USFCompanionTacticsComponent::SetEngagementRange(ESFCompanionEngagementRange NewRange)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	EngagementRange = NewRange;
}

// -----------------------------------------------------------------------------
// Orders
// -----------------------------------------------------------------------------

void USFCompanionTacticsComponent::IssueOrder(const FSFCompanionOrder& NewOrder)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	ActiveOrder = NewOrder;
	ActiveOrder.Sequence = NextOrderSequence++;
	LastObservedOrderSequence = ActiveOrder.Sequence;

	OnOrderIssued.Broadcast(ActiveOrder);
}

void USFCompanionTacticsComponent::ClearOrder()
{
	FSFCompanionOrder Cleared;
	Cleared.Type = ESFCompanionOrderType::None;
	IssueOrder(Cleared);
}

// -----------------------------------------------------------------------------
// Threshold gating
// -----------------------------------------------------------------------------

bool USFCompanionTacticsComponent::CanCommandAbility() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return true;
	}
	const double Now = World->GetTimeSeconds();
	return (Now - LastCommandedAbilityTime) >= Thresholds.AbilityCommandCooldown;
}

void USFCompanionTacticsComponent::NotifyAbilityCommanded()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (const UWorld* World = GetWorld())
	{
		LastCommandedAbilityTime = World->GetTimeSeconds();
	}
}

void USFCompanionTacticsComponent::EvaluateHealthThresholds(float SelfPct, float PlayerPct)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	const bool bNewSelfLow = SelfPct >= 0.f && SelfPct <= Thresholds.SelfLowHealthPct;
	const bool bNewPlayerLow = PlayerPct >= 0.f && PlayerPct <= Thresholds.PlayerLowHealthPct;

	if (bNewSelfLow != bSelfLowHealth)
	{
		bSelfLowHealth = bNewSelfLow;
		static const FGameplayTag SelfTag = FGameplayTag::RequestGameplayTag(
			FName(TEXT("Companion.Threat.SelfLowHealth")), /*ErrorIfNotFound*/ false);
		if (SelfTag.IsValid())
		{
			OnThresholdCrossed.Broadcast(SelfTag, bSelfLowHealth);
		}
	}

	if (bNewPlayerLow != bPlayerLowHealth)
	{
		bPlayerLowHealth = bNewPlayerLow;
		static const FGameplayTag PlayerTag = FGameplayTag::RequestGameplayTag(
			FName(TEXT("Companion.Threat.PlayerLowHealth")), /*ErrorIfNotFound*/ false);
		if (PlayerTag.IsValid())
		{
			OnThresholdCrossed.Broadcast(PlayerTag, bPlayerLowHealth);
		}
	}
}

// -----------------------------------------------------------------------------
// OnReps
// -----------------------------------------------------------------------------

void USFCompanionTacticsComponent::OnRep_Stance(ESFCompanionStance OldStance)
{
	RefreshStanceTags();
	ApplyStanceTagsToAbilitySystem();
	OnStanceChanged.Broadcast(OldStance, Stance);
}

void USFCompanionTacticsComponent::OnRep_ActiveOrder()
{
	if (ActiveOrder.Sequence != LastObservedOrderSequence)
	{
		LastObservedOrderSequence = ActiveOrder.Sequence;
		OnOrderIssued.Broadcast(ActiveOrder);
	}
}

// -----------------------------------------------------------------------------
// Internals
// -----------------------------------------------------------------------------

void USFCompanionTacticsComponent::RefreshStanceTags()
{
	switch (Stance)
	{
	case ESFCompanionStance::Tank:   ActiveStanceTags = TankStanceTags;   break;
	case ESFCompanionStance::DPS:    ActiveStanceTags = DPSStanceTags;    break;
	case ESFCompanionStance::Healer: ActiveStanceTags = HealerStanceTags; break;
	case ESFCompanionStance::Custom: ActiveStanceTags = CustomStanceTags; break;
	default: ActiveStanceTags.Reset(); break;
	}
}

void USFCompanionTacticsComponent::ApplyStanceTagsToAbilitySystem()
{
	IAbilitySystemInterface* AsASI = Cast<IAbilitySystemInterface>(GetOwner());
	if (!AsASI)
	{
		return;
	}

	UAbilitySystemComponent* ASC = AsASI->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	// Strip prior stance tags (any tag matching Companion.Stance) by clearing
	// the loose tags we own and re-adding the active set. Loose tags don't
	// replicate but stance is replicated separately, so each peer reapplies
	// after OnRep.
	static const FGameplayTag StanceRoot = FGameplayTag::RequestGameplayTag(
		FName(TEXT("Companion.Stance")), /*ErrorIfNotFound*/ false);

	if (StanceRoot.IsValid())
	{
		FGameplayTagContainer Owned;
		ASC->GetOwnedGameplayTags(Owned);
		for (const FGameplayTag& Existing : Owned)
		{
			if (Existing.MatchesTag(StanceRoot))
			{
				ASC->SetLooseGameplayTagCount(Existing, 0);
			}
		}
	}

	for (const FGameplayTag& Tag : ActiveStanceTags)
	{
		ASC->AddLooseGameplayTag(Tag);
	}
}
