#include "Characters/Components/SFNPCNarrativeComponent.h"

#include "GameFramework/Actor.h"
#include "Characters/Components/SFNPCStateComponent.h"
#include "Characters/Components/SFNPCGoalComponent.h"

USFNPCNarrativeComponent::USFNPCNarrativeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USFNPCNarrativeComponent::ApplyNarrativeEvent(const FSFNarrativeEventPayload& Payload)
{
	if (!Payload.EventTag.IsValid())
	{
		return;
	}

	KnownNarrativeTags.AddTag(Payload.EventTag);
	KnownNarrativeTags.AppendTags(Payload.EventContextTags);

	AActor* OwnerActor = GetOwner();
	if (ASFNPCBase* NPC = Cast<ASFNPCBase>(OwnerActor))
	{
		if (USFNPCStateComponent* StateComp = const_cast<USFNPCStateComponent*>(NPC->GetStateComponent()))
		{
			StateComp->AddStateTag(Payload.EventTag, true);
		}

		if (USFNPCGoalComponent* GoalComp = const_cast<USFNPCGoalComponent*>(NPC->GetGoalComponent()))
		{
			if (Payload.EventContextTags.HasTag(FGameplayTag::RequestGameplayTag(TEXT("Trigger.Combat.Enter"))))
			{
				FSFNPCGoal CombatGoal;
				CombatGoal.GoalId = TEXT("NarrativeCombatResponse");
				CombatGoal.GoalTag = FGameplayTag::RequestGameplayTag(TEXT("Goal.Combat.Engage"));
				CombatGoal.Priority = 100;
				CombatGoal.bCanBeInterrupted = false;
				CombatGoal.TargetActor = Payload.Subject;
				GoalComp->AddGoal(CombatGoal);
			}
		}
	}

	OnNarrativeEventApplied.Broadcast(Payload);
}

bool USFNPCNarrativeComponent::KnowsNarrativeTag(FGameplayTag Tag) const
{
	return KnownNarrativeTags.HasTag(Tag);
}