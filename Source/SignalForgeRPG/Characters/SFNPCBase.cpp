#include "Characters/SFNPCBase.h"

#include "AI/SFNPCAIController.h"
#include "Characters/SFNPCNarrativeIdentityComponent.h"
#include "Core/SFPlayerState.h"
#include "Dialogue/Data/SFConversationDataAsset.h"
#include "GameFramework/PlayerState.h"
#include "Narrative/SFNarrativeComponent.h"
#include "Narrative/SFNarrativeTypes.h"

ASFNPCBase::ASFNPCBase()
{
	PrimaryActorTick.bCanEverTick = false;

	NarrativeIdentity = CreateDefaultSubobject<USFNPCNarrativeIdentityComponent>(TEXT("NarrativeIdentity"));

	AIControllerClass = ASFNPCAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ASFNPCBase::BeginPlay()
{
	Super::BeginPlay();
}

void ASFNPCBase::HandleDeath()
{
	if (bIsDead)
	{
		return;
	}

	// Essential NPCs are kept alive by design — refuse to enter the death path.
	if (NarrativeIdentity && NarrativeIdentity->IsEssential())
	{
		return;
	}

	Super::HandleDeath();

	if (NarrativeIdentity)
	{
		NarrativeIdentity->SetDisposition(ESFNPCDisposition::Dead);
	}

	// Record an idempotent `Fact.NPC.Killed` bool fact keyed by this NPC's
	// ContextId so quests, dialogue conditions, and faction reactions can
	// branch on it. The fact is set on every connected player's narrative
	// component; designers can additionally fire FSFNarrativeDelta-based
	// outcomes from death blueprints if they want richer reactions
	// (FSFNarrativeDelta::ApplyOutcome takes a primary asset id, not a tag).
	if (HasAuthority() && NarrativeIdentity && !NarrativeIdentity->GetNPCContextId().IsNone())
	{
		static const FName KilledTagName(TEXT("Fact.NPC.Killed"));
		FSFWorldFactKey Key;
		Key.FactTag = FGameplayTag::RequestGameplayTag(KilledTagName, /*ErrorIfNotFound*/ false);
		Key.ContextId = NarrativeIdentity->GetNPCContextId();

		if (Key.FactTag.IsValid())
		{
			if (UWorld* World = GetWorld())
			{
				for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
				{
					if (APlayerController* PC = It->Get())
					{
						if (ASFPlayerState* PS = PC->GetPlayerState<ASFPlayerState>())
						{
							if (USFNarrativeComponent* N = PS->GetNarrativeComponent())
							{
								N->SetWorldFact(Key, FSFWorldFactValue::MakeBool(true));
							}
						}
					}
				}
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Interaction
// -----------------------------------------------------------------------------

USFNarrativeComponent* ASFNPCBase::ResolveInstigatorNarrative(const FSFInteractionContext& InteractionContext) const
{
	if (!IsValid(InteractionContext.InteractingActor))
	{
		return nullptr;
	}

	// InteractingActor may be the pawn; the narrative component lives on PlayerState.
	APawn* AsPawn = Cast<APawn>(InteractionContext.InteractingActor);
	APlayerState* InstigatorPlayerState = AsPawn ? AsPawn->GetPlayerState() : nullptr;

	if (ASFPlayerState* SFPlayerState = Cast<ASFPlayerState>(InstigatorPlayerState))
	{
		return SFPlayerState->GetNarrativeComponent();
	}

	// Fallback: actor itself owns a narrative component (rare — debug/cinematic only).
	return InteractionContext.InteractingActor->FindComponentByClass<USFNarrativeComponent>();
}

bool ASFNPCBase::PassesFactGate(USFNarrativeComponent* InstigatorNarrative) const
{
	if (RequiredFactTags.IsEmpty() && BlockedFactTags.IsEmpty())
	{
		return true;
	}

	if (!InstigatorNarrative)
	{
		// If we have gating requirements but no narrative component, treat as
		// an unconfigured project rather than a designer error: refuse.
		return false;
	}

	for (const FGameplayTag& Required : RequiredFactTags)
	{
		FSFWorldFactKey Key;
		Key.FactTag = Required;
		Key.ContextId = NAME_None;
		FSFWorldFactValue Value;
		const bool bHas = InstigatorNarrative->GetWorldFactValue(Key, Value);
		const bool bTrue = bHas && Value.ValueType == ESFNarrativeFactValueType::Bool && Value.BoolValue;
		if (!bTrue)
		{
			return false;
		}
	}

	for (const FGameplayTag& Blocked : BlockedFactTags)
	{
		FSFWorldFactKey Key;
		Key.FactTag = Blocked;
		Key.ContextId = NAME_None;
		FSFWorldFactValue Value;
		if (InstigatorNarrative->GetWorldFactValue(Key, Value))
		{
			if (Value.ValueType == ESFNarrativeFactValueType::Bool && Value.BoolValue)
			{
				return false;
			}
		}
	}

	return true;
}

ESFInteractionAvailability ASFNPCBase::GetInteractionAvailability_Implementation(
	const FSFInteractionContext& InteractionContext) const
{
	if (bIsDead)
	{
		return ESFInteractionAvailability::Disabled;
	}

	if (!bCanInteract)
	{
		return ESFInteractionAvailability::Disabled;
	}

	if (NarrativeIdentity)
	{
		const ESFNPCDisposition Disp = NarrativeIdentity->GetDisposition();
		if (Disp == ESFNPCDisposition::Dead)
		{
			return ESFInteractionAvailability::Disabled;
		}
		if (bRefuseDialogueWhenHostile && Disp == ESFNPCDisposition::Hostile)
		{
			return ESFInteractionAvailability::Disabled;
		}
	}

	if (!IsValid(InteractionContext.InteractingActor))
	{
		return ESFInteractionAvailability::Invalid;
	}

	const FVector InteractionLocation = GetInteractionLocation_Implementation(InteractionContext);
	const float AllowedRange = GetInteractionRange_Implementation(InteractionContext);

	const float DistSquared = FVector::DistSquared(
		InteractionContext.InteractingActor->GetActorLocation(),
		InteractionLocation);

	if (DistSquared > FMath::Square(AllowedRange))
	{
		return ESFInteractionAvailability::OutOfRange;
	}

	if (!PassesFactGate(ResolveInstigatorNarrative(InteractionContext)))
	{
		return ESFInteractionAvailability::Disabled;
	}

	return ESFInteractionAvailability::Available;
}

FSFInteractionOption ASFNPCBase::GetPrimaryInteractionOption_Implementation(
	const FSFInteractionContext& InteractionContext) const
{
	FSFInteractionOption Option;
	Option.OptionId = PrimaryInteractionOptionId;
	Option.PromptText = InteractionPromptText;
	Option.SubPromptText = InteractionSubPromptText;
	Option.Availability = GetInteractionAvailability_Implementation(InteractionContext);
	Option.TriggerType = InteractionTriggerType;
	Option.HoldDuration = InteractionHoldDuration;
	Option.bShowPromptWhenUnavailable = bShowPromptWhenUnavailable;
	return Option;
}

TArray<FSFInteractionOption> ASFNPCBase::GetInteractionOptions_Implementation(
	const FSFInteractionContext& InteractionContext) const
{
	TArray<FSFInteractionOption> Options;
	Options.Add(GetPrimaryInteractionOption_Implementation(InteractionContext));
	return Options;
}

FSFInteractionExecutionResult ASFNPCBase::Interact_Implementation(
	const FSFInteractionContext& InteractionContext)
{
	return InteractWithOption_Implementation(InteractionContext, PrimaryInteractionOptionId);
}

FSFInteractionExecutionResult ASFNPCBase::InteractWithOption_Implementation(
	const FSFInteractionContext& InteractionContext,
	FName OptionId)
{
	const ESFInteractionAvailability Availability =
		GetInteractionAvailability_Implementation(InteractionContext);

	if (Availability != ESFInteractionAvailability::Available)
	{
		return FSFInteractionExecutionResult::Failure(
			Availability,
			FText::FromString(TEXT("Interaction is unavailable.")),
			OptionId);
	}

	if (OptionId != PrimaryInteractionOptionId)
	{
		return FSFInteractionExecutionResult::Failure(
			ESFInteractionAvailability::Invalid,
			FText::FromString(TEXT("Unsupported interaction option.")),
			OptionId);
	}

	USFNarrativeComponent* InstigatorNarrative = ResolveInstigatorNarrative(InteractionContext);

	// Auto-record the "met" fact on first conversation. Cheap, idempotent,
	// and saves designers from sprinkling it across every greeting node.
	if (bAutoSetMetFactOnFirstConversation && InstigatorNarrative && NarrativeIdentity)
	{
		const FName ContextId = NarrativeIdentity->GetNPCContextId();
		if (!ContextId.IsNone())
		{
			static const FName MetTagName(TEXT("Fact.NPC.Met"));
			FSFWorldFactKey Key;
			Key.FactTag = FGameplayTag::RequestGameplayTag(MetTagName, /*ErrorIfNotFound*/ false);
			Key.ContextId = ContextId;

			if (Key.FactTag.IsValid())
			{
				FSFWorldFactValue Existing;
				const bool bAlreadyMet =
					InstigatorNarrative->GetWorldFactValue(Key, Existing) &&
					Existing.ValueType == ESFNarrativeFactValueType::Bool &&
					Existing.BoolValue;

				if (!bAlreadyMet && InstigatorNarrative->GetOwner() && InstigatorNarrative->GetOwner()->HasAuthority())
				{
					InstigatorNarrative->SetWorldFact(Key, FSFWorldFactValue::MakeBool(true));
				}
			}
		}
	}

	// Kick off the conversation through the narrative component so the
	// dialogue/quest flow stays unified. Server-authoritative.
	if (InstigatorNarrative && ConversationAsset && InstigatorNarrative->GetOwner() && InstigatorNarrative->GetOwner()->HasAuthority())
	{
		InstigatorNarrative->BeginConversation(ConversationAsset, this);
	}

	return FSFInteractionExecutionResult::Success(OptionId);
}

void ASFNPCBase::BeginInteractionFocus_Implementation(
	const FSFInteractionContext& /*InteractionContext*/)
{
	// Optional hook: outline, widget marker, audio cue, etc.
}

void ASFNPCBase::EndInteractionFocus_Implementation(
	const FSFInteractionContext& /*InteractionContext*/)
{
	// Optional hook: clear outline, hide marker, etc.
}

FVector ASFNPCBase::GetInteractionLocation_Implementation(
	const FSFInteractionContext& /*InteractionContext*/) const
{
	return GetActorLocation();
}

float ASFNPCBase::GetInteractionRange_Implementation(
	const FSFInteractionContext& /*InteractionContext*/) const
{
	return InteractionRange;
}

bool ASFNPCBase::CanInteract_Implementation(
	const FSFInteractionContext& InteractionContext) const
{
	return GetInteractionAvailability_Implementation(InteractionContext) ==
		ESFInteractionAvailability::Available;
}

FText ASFNPCBase::GetInteractionPromptText_Implementation(
	const FSFInteractionContext& InteractionContext) const
{
	return GetPrimaryInteractionOption_Implementation(InteractionContext).PromptText;
}

USFConversationDataAsset* ASFNPCBase::GetConversationAsset_Implementation() const
{
	return ConversationAsset;
}

FName ASFNPCBase::GetDialogueSpeakerId_Implementation() const
{
	if (DialogueSpeakerId != NAME_None)
	{
		return DialogueSpeakerId;
	}
	if (NarrativeIdentity && !NarrativeIdentity->GetNPCContextId().IsNone())
	{
		return NarrativeIdentity->GetNPCContextId();
	}
	return FName(TEXT("NPC"));
}
