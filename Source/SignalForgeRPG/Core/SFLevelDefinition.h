#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "SFLevelDefinition.generated.h"

USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFLevelDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	int32 Level = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	int32 XPRequiredToReachThisLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Stats")
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Stats")
	float MaxEcho = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Stats")
	float MaxShields = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player Stats")
	float MaxStamina = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Scaling")
	float EnemyHealthMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Scaling")
	float EnemyShieldsMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Scaling")
	float EnemyDamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Scaling")
	int32 EnemyXPReward = 10;
};