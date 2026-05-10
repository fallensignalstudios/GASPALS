// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/PrimaryAssetId.h"
#include "Narrative/SFNarrativeTypes.h"
#include "SFQuestDisplayTypes.generated.h"

class USFQuestDefinition;
class USFQuestInstance;

/**
 * UI-friendly view of a single objective (quest task) within a quest's
 * current state. Built by USFQuestLogWidgetController from runtime data.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFQuestObjectiveDisplayEntry
{
	GENERATED_BODY()

public:
	/** Stable id of the underlying FSFQuestTaskDefinition. */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	FName TaskId = NAME_None;

	/** Semantic verb tag (Kill, Collect, Travel, etc.). Useful for icons. */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	FGameplayTag TaskTag;

	/** Optional context id for disambiguation (target/location/etc.). */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	FName ContextId = NAME_None;

	/** Human-readable text for this objective row. */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	FText DisplayText;

	/** Current progress count (0..RequiredQuantity). */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	int32 CurrentProgress = 0;

	/** Required count to complete the task (>=1). */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	int32 RequiredQuantity = 1;

	/** True when CurrentProgress >= RequiredQuantity. */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	bool bIsComplete = false;

	/** Normalized progress in [0,1]. */
	float GetProgressFraction() const
	{
		const int32 Required = FMath::Max(1, RequiredQuantity);
		return FMath::Clamp(static_cast<float>(CurrentProgress) / static_cast<float>(Required), 0.0f, 1.0f);
	}
};

/**
 * UI-friendly view of a single quest. Carries enough information for a
 * collapsible/expandable list row plus its objectives.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFQuestDisplayEntry
{
	GENERATED_BODY()

public:
	/** Stable id used by the widget controller to address this quest. */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	FName QuestId = NAME_None;

	/** PrimaryAssetId of the quest definition (preferred routing key). */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	FPrimaryAssetId QuestAssetId;

	/** Cached pointer to the underlying definition asset (may be null after stream-out). */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	TObjectPtr<const USFQuestDefinition> Definition = nullptr;

	/** Quest title suitable for headers. */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	FText DisplayName;

	/** Long form quest description. */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	FText Description;

	/** Display name of the active state (e.g. "Reach the relay"). */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	FText CurrentStateDisplayName;

	/** Authoring id of the active state (used for diffing on updates). */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	FName CurrentStateId = NAME_None;

	/** High-level lifecycle (NotStarted/InProgress/Succeeded/Failed/Abandoned). */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	ESFQuestCompletionState CompletionState = ESFQuestCompletionState::NotStarted;

	/** Classification tags from the definition (Quest.Type.Main, etc.). */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	FGameplayTagContainer QuestTags;

	/** Per-objective rows for the active state. */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	TArray<FSFQuestObjectiveDisplayEntry> Objectives;

	/** True when all objectives in the active state are complete. */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	bool bAllObjectivesComplete = false;

	/** True if this quest was tracked by the user (pinned to HUD/top of list). */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	bool bIsTracked = false;

	/** Convenience: in progress and not finished. */
	bool IsActive() const
	{
		return CompletionState == ESFQuestCompletionState::InProgress;
	}

	/** Convenience: terminal state (succeeded, failed, or abandoned). */
	bool IsFinished() const
	{
		return CompletionState == ESFQuestCompletionState::Succeeded
			|| CompletionState == ESFQuestCompletionState::Failed
			|| CompletionState == ESFQuestCompletionState::Abandoned;
	}

	/** Average completion of objectives in the current state, in [0,1]. */
	float GetAverageObjectiveProgress() const
	{
		if (Objectives.Num() == 0)
		{
			return bAllObjectivesComplete ? 1.0f : 0.0f;
		}
		float Sum = 0.0f;
		for (const FSFQuestObjectiveDisplayEntry& Obj : Objectives)
		{
			Sum += Obj.GetProgressFraction();
		}
		return Sum / static_cast<float>(Objectives.Num());
	}
};
