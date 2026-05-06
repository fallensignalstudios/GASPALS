#include "Characters/SFNPCBase.h"

#include "Dialogue/Data/SFConversationDataAsset.h"

ASFNPCBase::ASFNPCBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

ESFInteractionAvailability ASFNPCBase::GetInteractionAvailability_Implementation(
	const FSFInteractionContext& InteractionContext) const
{
	if (!bCanInteract)
	{
		return ESFInteractionAvailability::Disabled;
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

	UE_LOG(LogTemp, Log, TEXT("%s interacted with %s"),
		*GetNameSafe(InteractionContext.InteractingActor),
		*GetName());

	return FSFInteractionExecutionResult::Success(OptionId);
}

void ASFNPCBase::BeginInteractionFocus_Implementation(
	const FSFInteractionContext& InteractionContext)
{
	// Optional hook: outline, widget marker, audio cue, etc.
}

void ASFNPCBase::EndInteractionFocus_Implementation(
	const FSFInteractionContext& InteractionContext)
{
	// Optional hook: clear outline, hide marker, etc.
}

FVector ASFNPCBase::GetInteractionLocation_Implementation(
	const FSFInteractionContext& InteractionContext) const
{
	return GetActorLocation();
}

float ASFNPCBase::GetInteractionRange_Implementation(
	const FSFInteractionContext& InteractionContext) const
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
	return DialogueSpeakerId != NAME_None ? DialogueSpeakerId : FName(TEXT("NPC"));
}