#include "Characters/Components/SFNPCGoalComponent.h"

USFNPCGoalComponent::USFNPCGoalComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USFNPCGoalComponent::AddGoal(const FSFNPCGoal& Goal)
{
	Goals.RemoveAll([&Goal](const FSFNPCGoal& ExistingGoal)
		{
			return ExistingGoal.GoalId == Goal.GoalId && Goal.GoalId != NAME_None;
		});

	Goals.Add(Goal);

	FSFNPCGoal ActiveGoal;
	if (TryGetHighestPriorityGoal(ActiveGoal))
	{
		OnActiveGoalChanged.Broadcast(ActiveGoal);
	}
}

void USFNPCGoalComponent::RemoveGoalById(FName GoalId)
{
	Goals.RemoveAll([&GoalId](const FSFNPCGoal& ExistingGoal)
		{
			return ExistingGoal.GoalId == GoalId;
		});

	FSFNPCGoal ActiveGoal;
	if (TryGetHighestPriorityGoal(ActiveGoal))
	{
		OnActiveGoalChanged.Broadcast(ActiveGoal);
	}
}

bool USFNPCGoalComponent::TryGetHighestPriorityGoal(FSFNPCGoal& OutGoal) const
{
	if (Goals.Num() == 0)
	{
		return false;
	}

	const FSFNPCGoal* BestGoal = &Goals[0];
	for (const FSFNPCGoal& Goal : Goals)
	{
		if (Goal.Priority > BestGoal->Priority)
		{
			BestGoal = &Goal;
		}
	}

	OutGoal = *BestGoal;
	return true;
}