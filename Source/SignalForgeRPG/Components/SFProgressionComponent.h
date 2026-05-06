#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/SFLevelDefinition.h"
#include "SFProgressionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnXPChangedSignature, int32, CurrentXP, int32, CurrentLevel, int32, XPToNextLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLevelChangedSignature, int32, NewLevel, bool, bLeveledUp);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SIGNALFORGERPG_API USFProgressionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFProgressionComponent();

	UFUNCTION(BlueprintCallable, Category = "Progression")
	void AddXP(int32 XPAmount);

	UFUNCTION(BlueprintCallable, Category = "Progression")
	void SetLevel(int32 NewLevel, bool bApplyStats = true, bool bFullRefill = true);

	UFUNCTION(BlueprintCallable, Category = "Progression")
	void ApplyLevelStats(bool bFullRefill);

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetCurrentLevel() const { return CurrentLevel; }

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetCurrentXP() const { return CurrentXP; }

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetXPToNextLevel() const;

	UFUNCTION(BlueprintPure, Category = "Progression")
	bool IsAtMaxLevel() const { return CurrentLevel >= MaxLevel; }

	UPROPERTY(BlueprintAssignable, Category = "Progression")
	FOnXPChangedSignature OnXPChanged;

	UPROPERTY(BlueprintAssignable, Category = "Progression")
	FOnLevelChangedSignature OnLevelChanged;

	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetEnemyXPRewardForCurrentLevel() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TObjectPtr<class ASFCharacterBase> OwnerCharacter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression")
	TObjectPtr<class UDataTable> LevelProgressionTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression")
	int32 MaxLevel = 60;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Progression")
	int32 CurrentLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Progression")
	int32 CurrentXP = 0;

	const FSFLevelDefinition* GetLevelDefinition(int32 Level) const;

	void TryProcessLevelUps();
	void BroadcastXPState();
};