#include "Characters/Components/SFNPCStateComponent.h"

USFNPCStateComponent::USFNPCStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USFNPCStateComponent::AddStateTag(FGameplayTag Tag, bool bPersistent)
{
	if (!Tag.IsValid())
	{
		return;
	}

	FGameplayTagContainer& TargetContainer = bPersistent ? PersistentStateTags : RuntimeStateTags;
	if (!TargetContainer.HasTagExact(Tag))
	{
		TargetContainer.AddTag(Tag);
		OnStateTagChanged.Broadcast(Tag, true);
	}
}

void USFNPCStateComponent::RemoveStateTag(FGameplayTag Tag, bool bPersistent)
{
	if (!Tag.IsValid())
	{
		return;
	}

	FGameplayTagContainer& TargetContainer = bPersistent ? PersistentStateTags : RuntimeStateTags;
	if (TargetContainer.HasTagExact(Tag))
	{
		TargetContainer.RemoveTag(Tag);
		OnStateTagChanged.Broadcast(Tag, false);
	}
}

bool USFNPCStateComponent::HasTag(FGameplayTag Tag) const
{
	return GetCombinedStateTags().HasTag(Tag);
}

bool USFNPCStateComponent::HasAnyTags(const FGameplayTagContainer& Tags) const
{
	return GetCombinedStateTags().HasAny(Tags);
}

bool USFNPCStateComponent::HasAllTags(const FGameplayTagContainer& Tags) const
{
	return GetCombinedStateTags().HasAll(Tags);
}

FGameplayTagContainer USFNPCStateComponent::GetCombinedStateTags() const
{
	FGameplayTagContainer Combined = PersistentStateTags;
	Combined.AppendTags(RuntimeStateTags);
	return Combined;
}

void USFNPCStateComponent::ResetRuntimeState()
{
	RuntimeStateTags.Reset();
}