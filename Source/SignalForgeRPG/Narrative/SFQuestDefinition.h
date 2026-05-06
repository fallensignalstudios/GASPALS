// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
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

    /**
     * Classification tags used for quest filtering / queries by USFQuestDatabase
     * (e.g. Quest.Type.Main, Quest.Type.Side, Quest.Region.Sector7).
     * Leave empty if your project filters quests another way.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Quest")
    FGameplayTagContainer QuestTags;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Quest")
    TArray<FSFQuestStateDefinition> States;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
