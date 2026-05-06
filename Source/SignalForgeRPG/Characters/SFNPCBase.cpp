#include "Characters/SFNPCBase.h"

#include "Dialogue/Data/SFConversationDataAsset.h"
#include "Characters/Components/SFNPCStateComponent.h"
#include "Characters/Components/SFNPCGoalComponent.h"
#include "Characters/Components/SFNPCNarrativeComponent.h"

ASFNPCBase::ASFNPCBase()
{
	PrimaryActorTick.bCanEverTick = false;

	StateComponent = CreateDefaultSubobject<USFNPCStateComponent>(TEXT("StateComponent"));
	GoalComponent = CreateDefaultSubobject<USFNPCGoalComponent>(TEXT("GoalComponent"));
	NarrativeComponent = CreateDefaultSubobject<USFNPCNarrativeComponent>(TEXT("NarrativeComponent"));
}

void ASFNPCBase::BeginPlay()
{
	Super::BeginPlay();
}

USFConversationDataAsset* ASFNPCBase::ResolveConversationAsset_Implementation(
	const FSFInteractionContext& InteractionContext) const
{
	return DefaultConversationAsset;
}

bool ASFNPCBase::CanInteractByState_Implementation(
	const FSFInteractionContext& InteractionContext) const
{
	if (!StateComponent)
	{
		return true;
	}

	if (BlockedInteractionTags.Num() > 0 && StateComponent->HasAnyTags(BlockedInteractionTags))
	{
		return false;
	}

	if (RequiredInteractionTags.Num() > 0 && !StateComponent->HasAllTags(RequiredInteractionTags))
	{
		return false;
	}

	if (AutoDisableInteractionTags.Num() > 0 && StateComponent->HasAnyTags(AutoDisableInteractionTags))
	{
		return false;
	}

	return true;
}

TArray<FSFInteractionOption> ASFNPCBase::BuildInteractionOptions_Implementation(
	const FSFInteractionContext& InteractionContext) const
{
	TArray<FSFInteractionOption> Options;

	FSFInteractionOption TalkOption;
	TalkOption.OptionId = PrimaryInteractionOptionId;
	TalkOption.PromptText = InteractionPromptText;
	TalkOption.SubPromptText = InteractionSubPromptText;
	TalkOption.Availability = GetInteractionAvailability_Implementation(InteractionContext);
	TalkOption.TriggerType = InteractionTriggerType;
	TalkOption.HoldDuration = InteractionHoldDuration;
	TalkOption.bShowPromptWhenUnavailable = bShowPromptWhenUnavailable;

	Options.Add(TalkOption);
	return Options;
}

bool ASFNPCBase::HasStateTag(FGameplayTag Tag) const
{
	return StateComponent ? StateComponent->HasTag(Tag) : false;
}

bool ASFNPCBase::HasAnyStateTags(const FGameplayTagContainer& Tags) const
{
	return StateComponent ? StateComponent->HasAnyTags(Tags) : false;
}

bool ASFNPCBase::HasAllStateTags(const FGameplayTagContainer& Tags) const
{
	return StateComponent ? StateComponent->HasAllTags(Tags) : false;
}

void ASFNPCBase::ApplyNarrativeEvent(const FSFNarrativeEventPayload& Payload)
{
	if (NarrativeComponent)
	{
		NarrativeComponent->ApplyNarrativeEvent(Payload);
	}

	OnNarrativeEventApplied(Payload);
}

ESFInteractionAvailability ASFNPCBase::GetInteractionAvailability_Implementation(
	const FSFInteractionContext& InteractionContext) const
{
	if (!IsValid(InteractionContext.InteractingActor))
	{
		return ESFInteractionAvailability::Invalid;
	}

	if (!CanInteractByState_Implementation(InteractionContext))
	{
		return ESFInteractionAvailability::Disabled;
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

	return ESFInteractionAvailability::Available;
}

FSFInteractionOption ASFNPCBase::GetPrimaryInteractionOption_Implementation(
	const FSFInteractionContext& InteractionContext) const
{
	const TArray<FSFInteractionOption> Options = BuildInteractionOptions_Implementation(InteractionContext);
	return Options.Num() > 0 ? Options[0] : FSFInteractionOption();
}

TArray<FSFInteractionOption> ASFNPCBase::GetInteractionOptions_Implementation(
	const FSFInteractionContext& InteractionContext) const
{
	return BuildInteractionOptions_Implementation(InteractionContext);
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
	const TArray<FSFInteractionOption> Options = GetInteractionOptions_Implementation(InteractionContext);
	const FSFInteractionOption* SelectedOption = Options.FindByPredicate(
		[&OptionId](const FSFInteractionOption& Option)
		{
			return Option.OptionId == OptionId;
		});

	if (!SelectedOption)
	{
		return FSFInteractionExecutionResult::Failure(
			ESFInteractionAvailability::Invalid,
			FText::FromString(TEXT("Unsupported interaction option.")),
			OptionId);
	}

	if (SelectedOption->Availability != ESFInteractionAvailability::Available)
	{
		return FSFInteractionExecutionResult::Failure(
			SelectedOption->Availability,
			FText::FromString(TEXT("Interaction is unavailable.")),
			OptionId);
	}

	UE_LOG(LogTemp, Log, TEXT("%s interacted with %s using option %s"),
		*GetNameSafe(InteractionContext.InteractingActor),
		*GetName(),
		*OptionId.ToString());

	OnInteractionSucceeded(InteractionContext, OptionId);
	return FSFInteractionExecutionResult::Success(OptionId);
}

void ASFNPCBase::BeginInteractionFocus_Implementation(
	const FSFInteractionContext& InteractionContext)
{
}

void ASFNPCBase::EndInteractionFocus_Implementation(
	const FSFInteractionContext& InteractionContext)
{
}

FVector ASFNPCBase::GetInteractionLocation_Implementation(
	const FSFInteractionContext& InteractionContext) const
{
	return GetActorLocation();
}

float ASFNPCBase::GetInteractionRange_Implementation(
	const FSFInteractionContext& InteractionContext) const
{
	return BaseInteractionRange;
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
	FSFInteractionContext DummyContext;
	return ResolveConversationAsset_Implementation(DummyContext);
}

FName ASFNPCBase::GetDialogueSpeakerId_Implementation() const
{
	return DialogueSpeakerId != NAME_None ? DialogueSpeakerId : FName(TEXT("NPC"));
}