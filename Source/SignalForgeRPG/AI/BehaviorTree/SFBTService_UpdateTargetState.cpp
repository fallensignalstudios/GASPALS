#include "AI/BehaviorTree/SFBTService_UpdateTargetState.h"

#include "AI/BehaviorTree/SFBTHelpers.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Characters/SFCharacterBase.h"

USFBTService_UpdateTargetState::USFBTService_UpdateTargetState()
{
	NodeName = TEXT("SF Update Target State");
	Interval = 0.2f;
	RandomDeviation = 0.05f;
	bNotifyTick = true;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(USFBTService_UpdateTargetState, TargetActorKey), AActor::StaticClass());
	DistanceKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(USFBTService_UpdateTargetState, DistanceKey));
	HasLineOfSightKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(USFBTService_UpdateTargetState, HasLineOfSightKey));
	IsHostileKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(USFBTService_UpdateTargetState, IsHostileKey));
	IsDeadKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(USFBTService_UpdateTargetState, IsDeadKey));
	LastKnownLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(USFBTService_UpdateTargetState, LastKnownLocationKey));
}

void USFBTService_UpdateTargetState::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBAsset);
		DistanceKey.ResolveSelectedKey(*BBAsset);
		HasLineOfSightKey.ResolveSelectedKey(*BBAsset);
		IsHostileKey.ResolveSelectedKey(*BBAsset);
		IsDeadKey.ResolveSelectedKey(*BBAsset);
		LastKnownLocationKey.ResolveSelectedKey(*BBAsset);
	}
}

void USFBTService_UpdateTargetState::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AI = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	ASFCharacterBase* Self = SFBTHelpers::GetControlledCharacter(AI);
	if (!AI || !BB || !Self)
	{
		return;
	}

	AActor* Target = nullptr;
	if (TargetActorKey.SelectedKeyName != NAME_None)
	{
		Target = Cast<AActor>(BB->GetValueAsObject(TargetActorKey.SelectedKeyName));
	}
	if (!Target)
	{
		// Zero out derived keys so old data doesn't lie around.
		if (DistanceKey.SelectedKeyName != NAME_None) BB->SetValueAsFloat(DistanceKey.SelectedKeyName, 0.0f);
		if (HasLineOfSightKey.SelectedKeyName != NAME_None) BB->SetValueAsBool(HasLineOfSightKey.SelectedKeyName, false);
		if (IsHostileKey.SelectedKeyName != NAME_None) BB->SetValueAsBool(IsHostileKey.SelectedKeyName, false);
		if (IsDeadKey.SelectedKeyName != NAME_None) BB->SetValueAsBool(IsDeadKey.SelectedKeyName, false);
		return;
	}

	const float Dist = FVector::Dist(Self->GetActorLocation(), Target->GetActorLocation());
	const bool bHostile = SFBTHelpers::IsHostile(Self, Target);
	const bool bLOS = SFBTHelpers::HasLineOfSight(Self, Target);
	bool bDead = false;
	if (const ASFCharacterBase* TargetChar = Cast<ASFCharacterBase>(Target))
	{
		bDead = TargetChar->IsDead();
	}

	if (DistanceKey.SelectedKeyName != NAME_None) BB->SetValueAsFloat(DistanceKey.SelectedKeyName, Dist);
	if (HasLineOfSightKey.SelectedKeyName != NAME_None) BB->SetValueAsBool(HasLineOfSightKey.SelectedKeyName, bLOS);
	if (IsHostileKey.SelectedKeyName != NAME_None) BB->SetValueAsBool(IsHostileKey.SelectedKeyName, bHostile);
	if (IsDeadKey.SelectedKeyName != NAME_None) BB->SetValueAsBool(IsDeadKey.SelectedKeyName, bDead);

	if (bLOS && LastKnownLocationKey.SelectedKeyName != NAME_None)
	{
		BB->SetValueAsVector(LastKnownLocationKey.SelectedKeyName, Target->GetActorLocation());
	}

	const bool bShouldDrop =
		(bClearOnDead && bDead)
		|| (bClearOnNonHostile && !bHostile)
		|| (LeashDistance > 0.0f && Dist > LeashDistance);

	if (bShouldDrop && TargetActorKey.SelectedKeyName != NAME_None)
	{
		BB->ClearValue(TargetActorKey.SelectedKeyName);
	}
}

FString USFBTService_UpdateTargetState::GetStaticDescription() const
{
	return FString::Printf(TEXT("Update target state every %.2fs (leash %.0f)"), Interval, LeashDistance);
}
