#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SFAbilitySlotUIData.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct FSFAbilitySlotUIData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	FGameplayTag InputTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	bool bHasAbility = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	bool bIsReady = false;

	// New: for precise cooldown updates
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	FGameplayTag CooldownTag;
};
