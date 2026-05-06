#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SFDialogueGraphChoiceEntry.generated.h"

USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFDialogueGraphChoiceEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FText ChoiceText;

	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FGameplayTagContainer RequiredTags;

	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FGameplayTagContainer BlockedTags;
};