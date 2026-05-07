#include "AI/Companion/SFCompanionStateBase.h"

#include "AI/Companion/SFCompanionStateMachine.h"
#include "AI/SFCompanionAIController.h"
#include "Companions/SFCompanionCharacter.h"
#include "Companions/SFCompanionTacticsComponent.h"

ASFCompanionAIController* FSFCompanionStateBase::GetController(USFCompanionStateMachine& Machine) const
{
	return Machine.GetController();
}

ASFCompanionCharacter* FSFCompanionStateBase::GetCompanion(USFCompanionStateMachine& Machine) const
{
	return Machine.GetCompanion();
}

USFCompanionTacticsComponent* FSFCompanionStateBase::GetTactics(USFCompanionStateMachine& Machine) const
{
	return Machine.GetTactics();
}
