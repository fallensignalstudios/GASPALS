#include "Companions/SFCompanionBarkComponent.h"

#include "Companions/SFCompanionCharacter.h"
#include "Companions/SFCompanionComponent.h"
#include "Companions/SFCompanionTacticsComponent.h"
#include "Core/SFPlayerState.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Narrative/SFNarrativeComponent.h"
#include "Narrative/SFNarrativeTypes.h"
#include "TimerManager.h"

USFCompanionBarkComponent::USFCompanionBarkComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false); // selection happens server-side; broadcast is local
}

void USFCompanionBarkComponent::BeginPlay()
{
	Super::BeginPlay();

	CompanionOwner = Cast<ASFCompanionCharacter>(GetOwner());
	if (CompanionOwner)
	{
		Tactics = CompanionOwner->GetTactics();
	}

	BindTacticsHooks();
	TryResolvePlayerNarrative();
	BindNarrativeHooks();

	// PlayerController/PlayerState may not be ready at BeginPlay in PIE or
	// after seamless travel. Retry once shortly after for the narrative bind.
	if (!CachedPlayerNarrative)
	{
		if (UWorld* World = GetWorld())
		{
			FTimerHandle Handle;
			World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				if (!CachedPlayerNarrative)
				{
					TryResolvePlayerNarrative();
					BindNarrativeHooks();
				}
			}), 0.5f, /*bLoop=*/false);
		}
	}
}

void USFCompanionBarkComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindNarrativeHooks();
	UnbindTacticsHooks();
	Super::EndPlay(EndPlayReason);
}

// -----------------------------------------------------------------------------
// Binding helpers
// -----------------------------------------------------------------------------

void USFCompanionBarkComponent::BindTacticsHooks()
{
	if (!Tactics) return;

	Tactics->OnStanceChanged.AddDynamic(this, &USFCompanionBarkComponent::HandleStanceChanged);
	Tactics->OnOrderIssued.AddDynamic(this, &USFCompanionBarkComponent::HandleOrderIssued);
	Tactics->OnThresholdCrossed.AddDynamic(this, &USFCompanionBarkComponent::HandleThresholdCrossed);
}

void USFCompanionBarkComponent::UnbindTacticsHooks()
{
	if (!Tactics) return;

	Tactics->OnStanceChanged.RemoveDynamic(this, &USFCompanionBarkComponent::HandleStanceChanged);
	Tactics->OnOrderIssued.RemoveDynamic(this, &USFCompanionBarkComponent::HandleOrderIssued);
	Tactics->OnThresholdCrossed.RemoveDynamic(this, &USFCompanionBarkComponent::HandleThresholdCrossed);
}

void USFCompanionBarkComponent::TryResolvePlayerNarrative()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	CachedPlayerState = Cast<ASFPlayerState>(PC->PlayerState);
	if (CachedPlayerState)
	{
		CachedPlayerNarrative = CachedPlayerState->GetNarrativeComponent();
	}
}

void USFCompanionBarkComponent::BindNarrativeHooks()
{
	if (!CachedPlayerNarrative) return;

	CachedPlayerNarrative->OnWorldFactChanged.AddDynamic(this, &USFCompanionBarkComponent::HandleWorldFactChanged);
	CachedPlayerNarrative->OnFactionStandingChanged.AddDynamic(this, &USFCompanionBarkComponent::HandleFactionStandingChanged);
}

void USFCompanionBarkComponent::UnbindNarrativeHooks()
{
	if (!CachedPlayerNarrative) return;

	CachedPlayerNarrative->OnWorldFactChanged.RemoveDynamic(this, &USFCompanionBarkComponent::HandleWorldFactChanged);
	CachedPlayerNarrative->OnFactionStandingChanged.RemoveDynamic(this, &USFCompanionBarkComponent::HandleFactionStandingChanged);
}

// -----------------------------------------------------------------------------
// Selection
// -----------------------------------------------------------------------------

bool USFCompanionBarkComponent::LineEligible(const FSFCompanionBarkLine& Line) const
{
	// Approval gate
	if (CachedPlayerState && CompanionOwner)
	{
		if (USFCompanionComponent* CompComp = CachedPlayerState->GetCompanionComponent())
		{
			const int32 Approval = CompComp->GetApproval(CompanionOwner->GetCompanionId());
			if (Approval < Line.MinApproval)
			{
				return false;
			}
		}
	}

	// Required-fact gate
	if (Line.RequiredFactTags.Num() > 0 && CachedPlayerNarrative)
	{
		for (const FGameplayTag& FactTag : Line.RequiredFactTags)
		{
			FSFWorldFactKey Key;
			Key.FactTag = FactTag;
			Key.ContextId = NAME_None;
			if (!CachedPlayerNarrative->HasWorldFact(Key))
			{
				return false;
			}
		}
	}

	return true;
}

bool USFCompanionBarkComponent::TryPlayBark(FGameplayTag BarkTag)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !BarkTag.IsValid())
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (!World) return false;

	const double Now = World->GetTimeSeconds();
	if (const double* LastFired = LastFiredByTag.Find(BarkTag))
	{
		if ((Now - *LastFired) < DefaultBarkCooldown)
		{
			return false; // on cooldown
		}
	}

	// Collect candidates (exact match OR tag matches a parent rule).
	TArray<const FSFCompanionBarkLine*> Candidates;
	float TotalWeight = 0.f;
	for (const FSFCompanionBarkLine& Line : BarkLines)
	{
		if (!Line.BarkTag.IsValid()) continue;
		if (!BarkTag.MatchesTag(Line.BarkTag)) continue; // BarkTag IS-A Line.BarkTag
		if (!LineEligible(Line)) continue;
		Candidates.Add(&Line);
		TotalWeight += Line.Weight;
	}

	if (Candidates.Num() == 0 || TotalWeight <= 0.f)
	{
		return false;
	}

	// Weighted pick
	float Roll = FMath::FRandRange(0.f, TotalWeight);
	const FSFCompanionBarkLine* Picked = Candidates.Last();
	for (const FSFCompanionBarkLine* Line : Candidates)
	{
		Roll -= Line->Weight;
		if (Roll <= 0.f)
		{
			Picked = Line;
			break;
		}
	}

	LastFiredByTag.Add(BarkTag, Now);
	OnBarkPlayed.Broadcast(BarkTag, *Picked);
	return true;
}

// -----------------------------------------------------------------------------
// Tactics-event -> bark mapping
// -----------------------------------------------------------------------------

void USFCompanionBarkComponent::HandleStanceChanged(ESFCompanionStance /*OldStance*/, ESFCompanionStance NewStance)
{
	const TCHAR* TagName = nullptr;
	switch (NewStance)
	{
	case ESFCompanionStance::Tank:   TagName = TEXT("Bark.Stance.Tank");   break;
	case ESFCompanionStance::DPS:    TagName = TEXT("Bark.Stance.DPS");    break;
	case ESFCompanionStance::Healer: TagName = TEXT("Bark.Stance.Healer"); break;
	default: return;
	}

	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(TagName), /*ErrorIfNotFound*/ false);
	if (Tag.IsValid())
	{
		TryPlayBark(Tag);
	}
}

void USFCompanionBarkComponent::HandleOrderIssued(const FSFCompanionOrder& NewOrder)
{
	const TCHAR* TagName = nullptr;
	switch (NewOrder.Type)
	{
	case ESFCompanionOrderType::Follow:          TagName = TEXT("Bark.Order.Follow");        break;
	case ESFCompanionOrderType::HoldPosition:    TagName = TEXT("Bark.Order.HoldPosition");  break;
	case ESFCompanionOrderType::MoveToLocation:  TagName = TEXT("Bark.Order.MoveTo");        break;
	case ESFCompanionOrderType::AttackTarget:    TagName = TEXT("Bark.Order.Attack");        break;
	case ESFCompanionOrderType::FocusFire:       TagName = TEXT("Bark.Order.FocusFire");     break;
	case ESFCompanionOrderType::UseAbility:      TagName = TEXT("Bark.Order.UseAbility");    break;
	case ESFCompanionOrderType::RetreatToPlayer: TagName = TEXT("Bark.Order.Retreat");       break;
	default: return;
	}

	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(TagName), /*ErrorIfNotFound*/ false);
	if (Tag.IsValid())
	{
		TryPlayBark(Tag);
	}
}

void USFCompanionBarkComponent::HandleThresholdCrossed(FGameplayTag ThresholdTag, bool bNowActive)
{
	if (!bNowActive || !ThresholdTag.IsValid()) return;

	// Reuse the threshold tag space directly under Bark.* by remapping the leaf.
	// Companion.Threat.PlayerLowHealth -> Bark.Combat.PlayerLowHealth
	const FString Source = ThresholdTag.ToString();
	FString Leaf;
	if (!Source.Split(TEXT("."), nullptr, &Leaf, ESearchCase::CaseSensitive, ESearchDir::FromEnd))
	{
		return;
	}

	const FString Mapped = FString::Printf(TEXT("Bark.Combat.%s"), *Leaf);
	const FGameplayTag BarkTag = FGameplayTag::RequestGameplayTag(FName(*Mapped), /*ErrorIfNotFound*/ false);
	if (BarkTag.IsValid())
	{
		TryPlayBark(BarkTag);
	}
}

// -----------------------------------------------------------------------------
// Narrative-event -> bark mapping
// -----------------------------------------------------------------------------

void USFCompanionBarkComponent::HandleWorldFactChanged(FGameplayTag FactTag, FName /*ContextId*/, const FSFWorldFactValue& /*NewValue*/)
{
	if (!FactTag.IsValid()) return;

	// Generic interjection: companions react to any world fact under
	// Fact.NPC.Killed, Fact.Quest.Completed, etc. with a tag-shaped bark.
	// Rule: Fact.<Domain>.<Leaf>  ->  Bark.React.<Domain>
	const FString Source = FactTag.ToString();
	TArray<FString> Parts;
	Source.ParseIntoArray(Parts, TEXT("."), true);
	if (Parts.Num() < 2) return;

	const FString Mapped = FString::Printf(TEXT("Bark.React.%s"), *Parts[1]);
	const FGameplayTag BarkTag = FGameplayTag::RequestGameplayTag(FName(*Mapped), /*ErrorIfNotFound*/ false);
	if (BarkTag.IsValid())
	{
		TryPlayBark(BarkTag);
	}
}

void USFCompanionBarkComponent::HandleFactionStandingChanged(FGameplayTag FactionTag, float OldValue, float NewValue)
{
	if (!FactionTag.IsValid()) return;

	// Coarse direction: significant up / down moves get distinct barks.
	const float Delta = NewValue - OldValue;
	if (FMath::Abs(Delta) < 1.0f) return;

	const TCHAR* TagName = (Delta > 0.f) ? TEXT("Bark.React.FactionUp") : TEXT("Bark.React.FactionDown");
	const FGameplayTag BarkTag = FGameplayTag::RequestGameplayTag(FName(TagName), /*ErrorIfNotFound*/ false);
	if (BarkTag.IsValid())
	{
		TryPlayBark(BarkTag);
	}
}
