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

	// Drive the controller's focal point at the target so GetActorEyesViewPoint()
	// returns an eye-to-eye direction (with pitch), not a yaw-only horizontal
	// vector. This is what makes ranged shots actually land on the player when
	// they are at a different height -- without it, AIs fire perfectly
	// horizontally from their eye height and bullets pass over/under the player.
	AI->SetFocus(Target, EAIFocusPriority::Gameplay);

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

	// Body yaw: keep the pawn upright (pitch=0) so the spine doesn't tilt; only
	// rotate the body around yaw. This matches the player rig -- pitch is
	// expressed via the aim-offset overlay reading control rotation, not by
	// pitching the actor.
	const FVector SelfLoc = Self->GetActorLocation();
	const FVector TargetLoc = Target->GetActorLocation();
	const FVector PlanarDelta(TargetLoc.X - SelfLoc.X, TargetLoc.Y - SelfLoc.Y, 0.0f);
	const FRotator Desired(0.0f, PlanarDelta.Rotation().Yaw, 0.0f);
	const FRotator Current = Self->GetActorRotation();
	const float YawError = FMath::Abs(FRotator::NormalizeAxis(Desired.Yaw - Current.Yaw));

	// Compute eye-to-eye pitch up front so we can apply it on both the
	// in-progress-rotation path and the already-facing-target success path.
	FVector SelfEyeLocStartup; FRotator SelfEyeRotStartup;
	Self->GetActorEyesViewPoint(SelfEyeLocStartup, SelfEyeRotStartup);
	FVector TargetEyeLocStartup = TargetLoc;
	if (const APawn* TargetPawnStartup = Cast<APawn>(Target))
	{
		FRotator UnusedStartup;
		TargetPawnStartup->GetActorEyesViewPoint(TargetEyeLocStartup, UnusedStartup);
	}
	const FRotator EyeAimRot = (TargetEyeLocStartup - SelfEyeLocStartup).Rotation();

	if (YawError <= AcceptanceAngleDegrees)
	{
		// Already facing the target -- still need to update pitch every tick so
		// the aim trace tracks vertically.
		AI->SetControlRotation(FRotator(EyeAimRot.Pitch, Current.Yaw, 0.0f));
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	const FRotator NewRot = FMath::RInterpConstantTo(Current, Desired, DeltaSeconds, RotationRate);
	Self->SetActorRotation(NewRot);

	// Drive the AIController's ControlRotation with BOTH yaw and pitch so anything
	// that reads GetControlRotation() (hitscan eye trace / GetActorEyesViewPoint /
	// aim-offset overlay / WeaponFire trace) gets an eye-to-eye direction. We
	// compute pitch from eye-to-eye, not from actor origins, so vertical aim is
	// accurate regardless of capsule height. Without the pitch component the AI
	// fires perfectly horizontally and shots miss any target at a different
	// elevation.
	AI->SetControlRotation(FRotator(EyeAimRot.Pitch, NewRot.Yaw, 0.0f));

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
