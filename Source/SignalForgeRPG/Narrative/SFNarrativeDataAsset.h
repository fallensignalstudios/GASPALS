// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "SFNarrativeTypes.h"
#include "SFQuestDefinition.h"
#include "SFNarrativeDataAsset.generated.h"

class UDataTable;
class UPrimaryDataAsset;
class UCurveFloat;
class USoundBase;
class UTexture2D;
class UMaterialInterface;

/**
 * Global configuration and content references for the narrative system.
 *
 * This asset is intended to be created once per project/game and assigned
 * in project settings or on a high-level narrative subsystem.
 */
UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFNarrativeDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    //
    // General settings
    //

    /** Human-readable name for this narrative configuration (e.g. Episode 1). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Config")
    FText DisplayName;

    /** Version number for migrations / compatibility checks. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Config")
    int32 NarrativeVersion = 1;

    /** Gameplay tag used as the root for all narrative-related tags. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Tags")
    FGameplayTag NarrativeRootTag;

    //
    // Quest content
    //

    /**
     * All quest definition assets that should be registered at startup.
     * Typically these are USFQuestDefinition primary assets.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Quests")
    TArray<TSoftObjectPtr<USFQuestDefinition>> QuestDefinitions;

    /** Optional data table listing quest metadata (for menus/codex). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Quests")
    TSoftObjectPtr<UDataTable> QuestMetadataTable;

    //
    // Faction / identity / ending catalogues
    //

    /** Faction catalog table describing each faction’s display name, icon, etc. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Factions")
    TSoftObjectPtr<UDataTable> FactionTable;

    /** Identity axis catalog table (id ? display name, description, icon). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Identity")
    TSoftObjectPtr<UDataTable> IdentityAxisTable;

    /** Ending catalog table (tag ? name, description, requirements). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Endings")
    TSoftObjectPtr<UDataTable> EndingTable;

    //
    // World facts / triggers / events
    //

    /** Optional table describing known world facts and how they are surfaced in UI. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Facts")
    TSoftObjectPtr<UDataTable> WorldFactCatalog;

    /** Optional table of narrative triggers / scripted events. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Events")
    TSoftObjectPtr<UDataTable> NarrativeEventTable;

    //
    // Tuning curves
    //

    /** Curve mapping faction aggregate score ? standing band thresholds (debug/tuning). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Curves")
    TSoftObjectPtr<UCurveFloat> FactionStandingCurve;

    /** Curve mapping identity axis raw value ? normalized 0–1 for UI bars. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Curves")
    TSoftObjectPtr<UCurveFloat> IdentityAxisNormalizationCurve;

    //
    // Audio / presentation
    //

    /** Optional default sound to play for important narrative beats. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Presentation")
    TSoftObjectPtr<USoundBase> ImportantBeatSound;

    /** Optional sound for quest update / objective complete. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Presentation")
    TSoftObjectPtr<USoundBase> QuestUpdateSound;

    /** Optional icon atlas / material used by the narrative HUD. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Presentation")
    TSoftObjectPtr<UMaterialInterface> NarrativeIconMaterial;

    /** Optional global background texture for narrative UI (codex, log, etc.). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Presentation")
    TSoftObjectPtr<UTexture2D> NarrativeBackgroundTexture;

    //
    // Utility
    //

    /** Returns true if the given quest asset id is part of this configuration. */
    UFUNCTION(BlueprintPure, Category = "Narrative|Config")
    bool ContainsQuestDefinition(const FPrimaryAssetId& QuestAssetId) const
    {
        for (const TSoftObjectPtr<USFQuestDefinition>& QuestPtr : QuestDefinitions)
        {
            if (QuestPtr.IsValid())
            {
                if (QuestPtr.Get()->GetPrimaryAssetId() == QuestAssetId)
                {
                    return true;
                }
            }
            else if (FName(*QuestPtr.ToSoftObjectPath().GetAssetPath().ToString()) == QuestAssetId.PrimaryAssetName)
            {
                // Fallback comparison if asset not loaded yet.
                return true;
            }
        }

        return false;
    }
};

// Copyright Fallen Signal Studios LLC. All Rights Reserved.
