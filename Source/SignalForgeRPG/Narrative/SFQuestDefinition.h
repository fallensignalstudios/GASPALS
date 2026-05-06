#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UObject/PrimaryAssetId.h"
#include "SFNarrativeTypes.h"
#include "SFQuestDefinition.generated.h"

UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFQuestDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Quest")
    FName QuestId = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Quest")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Quest")
    FText Description;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Quest")
    FName InitialStateId = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Quest")
    TArray<FSFQuestStateDefinition> States;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};