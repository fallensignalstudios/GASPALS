#include "AI/BehaviorTree/SFBTTask_FaceTarget.h"

#include "AI/BehaviorTree/SFBTHelpers.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/SFCharacterBase.h"
#include "GameFramework/Pawn.h"

USFBTTask_FaceTarget::USFBTTask_FaceTarget()
{
	NodeName = TEXT("SF Face Target");
	bNotifyTick = true;
}

EBTNodeResult::Type USFBTTask_FaceTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/)
{
	AAIController* AI = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	ASFCharacterBase* Self = SFBTHelpers::GetControlledCharacter(AI);
	if (!AI || !BB || !Self)
	{
		return EBTNodeResult::Failed;
	}

	AActor* Target = SFBTHelpers::GetBBActor(BB, BlackboardKey.SelectedKeyName);
	if (!Target)
	{
		return EBTNodeResult::Failed;
	}
	if (const ASFCharacterBase* TargetChar = Cast<ASFCharacterBase>(Target))
	{
		if (TargetChar->IsDead())
		{
			return EBTNodeResult::Failed;
		}
	}

	return EBTNodeResult::InProgress;
}

void USFBTTask_FaceTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/, float DeltaSeconds)
{
	AAIController* AI = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	ASFCharacterBase* Self = SFBTHelpers::GetControlledCharacter(AI);
	if (!AI || !BB || !Self)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	AActor* Target = SFBTHelpers::GetBBActor(BB, BlackboardKey.SelectedKeyName);
	if (!Target)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const FVector Delta = Target->GetActorLocation() - Self->GetActorLocation();
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

	// Track elapsed time via the component's instanced timer instead of NodeMemory to keep this task non-instanced.
	// Simple guard: if YawError is still huge after one tick of TimeoutSeconds worth of rotation, give up.
	// (RotationRate * TimeoutSeconds gives the max degrees we could ever cover.)
	if (TimeoutSeconds > 0.0f && YawError > RotationRate * TimeoutSeconds)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
	}
}

FString USFBTTask_FaceTarget::GetStaticDescription() const
{
	return FString::Printf(TEXT("Face %s @ %.0f deg/s (<= %.1f deg)"),
		*BlackboardKey.SelectedKeyName.ToString(), RotationRate, AcceptanceAngleDegrees);
}
