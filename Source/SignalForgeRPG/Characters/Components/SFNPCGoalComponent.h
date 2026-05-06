#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "SFNPCGoalComponent.generated.h"

USTRUCT(BlueprintType)
struct FSFNPCGoal
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Goal")
	FName GoalId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Goal")
	FGameplayTag GoalTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Goal")
	FGameplayTagContainer GoalContextTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Goal")
	int32 Priority = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Goal")
	bool bCanBeInterrupted = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Goal")
	TObjectPtr<AActor> TargetActor = nullptr;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FSFNPCGoalChangedSignature,
	const FSFNPCGoal&, Goal);

UCLASS(ClassGroup = (SignalForge), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class SIGNALFORGERPG_API USFNPCGoalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFNPCGoalComponent();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Goal")
	TArray<FSFNPCGoal> Goals;

public:
	UPROPERTY(BlueprintAssignable, Category = "NPC|Goal")
	FSFNPCGoalChangedSignature OnActiveGoalChanged;

	UFUNCTION(BlueprintCallable, Category = "NPC|Goal")
	void AddGoal(const FSFNPCGoal& Goal);

	UFUNCTION(BlueprintCallable, Category = "NPC|Goal")
	void RemoveGoalById(FName GoalId);

	UFUNCTION(BlueprintCallable, Category = "NPC|Goal")
	bool TryGetHighestPriorityGoal(FSFNPCGoal& OutGoal) const;

	UFUNCTION(BlueprintCallable, Category = "NPC|Goal")
	const TArray<FSFNPCGoal>& GetGoals() const { return Goals; }
};