#include "AI/BehaviorTree/SFBTTask_LookAtLocation.h"

#include "AI/BehaviorTree/SFBTHelpers.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/SFCharacterBase.h"

USFBTTask_LookAtLocation::USFBTTask_LookAtLocation()
{
	NodeName = TEXT("SF Look At Location");
	bNotifyTick = true;
}

EBTNodeResult::Type USFBTTask_LookAtLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	AAIController* AI = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	ASFCharacterBase* Self = SFBTHelpers::GetControlledCharacter(AI);
	if (!AI || !BB || !Self)
	{
		return EBTNodeResult::Failed;
	}

	FVector Loc;
	if (!SFBTHelpers::GetBBVector(BB, BlackboardKey.SelectedKeyName, Loc))
	{
		return EBTNodeResult::Failed;
	}

	return EBTNodeResult::InProgress;
}

void USFBTTask_LookAtLocation::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/, float DeltaSeconds)
{
	AAIController* AI = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	ASFCharacterBase* Self = SFBTHelpers::GetControlledCharacter(AI);
	if (!AI || !BB || !Self)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FVector Loc;
	if (!SFBTHelpers::GetBBVector(BB, BlackboardKey.SelectedKeyName, Loc))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const FVector Delta = Loc - Self->GetActorLocation();
	const FRotator Desired(0.0f, Delta.Rotation().Yaw, 0.0f);
	const FRotator Current = Self->GetActorRotation();
	const float YawError = FMath::Abs(FRotator::NormalizeAxis(Desired.Yaw - Current.Yaw));

	if (YawError <= AcceptanceAngleDegrees)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	const FRotator NewRot = FMath::RInterpConstantTo(Current, Desired, DeltaSeconds, RotationRate);
	Self->SetActorRotation(NewRot);

	if (TimeoutSeconds > 0.0f && YawError > RotationRate * TimeoutSeconds)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
	}
}

FString USFBTTask_LookAtLocation::GetStaticDescription() const
{
	return FString::Printf(TEXT("Look at %s @ %.0f deg/s (<= %.1f deg)"),
		*BlackboardKey.SelectedKeyName.ToString(), RotationRate, AcceptanceAngleDegrees);
}
