#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "SFEnemyAIController.generated.h"

class ASFEnemyCharacter;
class UBlackboardComponent;
class UBehaviorTreeComponent;

UCLASS()
class SIGNALFORGERPG_API ASFEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	ASFEnemyAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UFUNCTION(BlueprintPure, Category = "AI")
	ASFEnemyCharacter* GetControlledEnemy() const { return ControlledEnemy; }

	UFUNCTION(BlueprintPure, Category = "AI")
	UBlackboardComponent* GetBlackboardComponent() const { return BlackboardComponent; }

protected:
	UPROPERTY(Transient)
	TObjectPtr<ASFEnemyCharacter> ControlledEnemy = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBlackboardComponent> BlackboardComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBehaviorTreeComponent> BehaviorTreeComponent = nullptr;
};