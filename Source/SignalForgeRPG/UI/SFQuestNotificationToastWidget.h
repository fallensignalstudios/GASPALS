// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Narrative/SFQuestDisplayTypes.h"
#include "UI/SFUserWidgetBase.h"
#include "SFQuestNotificationToastWidget.generated.h"

class UTextBlock;
class UPanelWidget;
class USFNarrativeComponent;
class USFQuestInstance;

/**
 * HUD-side "New Quest Added" toast.
 *
 * Drop one instance of WBP_QuestNotificationToast (reparented to this class)
 * into the player HUD, anchored where you'd like it shown (e.g. top-left).
 * The widget self-resolves the local player's USFNarrativeComponent, listens
 * to OnQuestStarted, and shows a short-lived notification with the quest's
 * display name and current state.
 *
 * The visual reveal/hide is entirely up to the designer: hook
 * BP_OnNotificationShown / BP_OnNotificationDismissed to drive a widget
 * animation. The container starts collapsed; we only call SetVisibility on
 * the named RootContainer when one is bound — if it isn't bound, designers
 * can rely solely on the BP hooks.
 *
 * Notifications are queued, so multiple quests starting on the same frame
 * are shown one after the other instead of being dropped.
 */
UCLASS(Blueprintable)
class SIGNALFORGERPG_API USFQuestNotificationToastWidget : public USFUserWidgetBase
{
	GENERATED_BODY()

public:
	/** Manually point the toast at a narrative component. Optional; auto-resolve handles the common case. */
	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI")
	void InitializeToast(USFNarrativeComponent* InNarrativeComponent);

	/** Tear down bindings and stop any active timers. */
	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI")
	void DeinitializeToast();

	/** Force a notification with the supplied quest entry. Useful for previews / blueprint tests. */
	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI")
	void ShowToastForEntry(const FSFQuestDisplayEntry& Entry);

	/** Immediately hide and clear any queued notifications. */
	UFUNCTION(BlueprintCallable, Category = "Narrative|Quest|UI")
	void DismissAndClearQueue();

	UFUNCTION(BlueprintPure, Category = "Narrative|Quest|UI")
	USFNarrativeComponent* GetNarrativeComponent() const { return NarrativeComponent.Get(); }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// --- BindWidgetOptional UMG bindings ---

	/** Root container we toggle visibility on. Designer-bound; can be null when an animation hides the widget instead. */
	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> RootContainer = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HeaderText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuestNameText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuestStateText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuestDescriptionText = nullptr;

	// --- Designer-tunable behavior ---

	/** Header text shown above the quest name (e.g. "New Quest"). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Quest|UI")
	FText HeaderLabel;

	/** How long each notification stays on screen before auto-dismissing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (ClampMin = "0.5", ClampMax = "30.0"))
	float DisplayDurationSeconds = 5.0f;

	/** Brief pause between consecutive queued notifications so they don't snap-cut. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float InterToastGapSeconds = 0.25f;

	/** Period between auto-resolve attempts when the narrative component isn't ready yet. */
	UPROPERTY(EditDefaultsOnly, Category = "Narrative|Quest|UI", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float AutoResolveRetryPeriodSeconds = 0.5f;

	/** Total time we'll keep retrying auto-resolve before giving up. */
	UPROPERTY(EditDefaultsOnly, Category = "Narrative|Quest|UI", meta = (ClampMin = "0.5", ClampMax = "60.0"))
	float MaxAutoResolveRetrySeconds = 10.0f;

	/**
	 * Suppress the toast for the very first OnQuestStarted broadcast that
	 * happens within this many seconds of binding. Stops "all your existing
	 * quests just spawned!" floods immediately after a load when the
	 * narrative component replays its state.
	 *
	 * Set to 0 to fire toasts for every started quest, even on initial bind.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Narrative|Quest|UI", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float SuppressInitialBroadcastsForSeconds = 0.5f;

	// --- Internals ---

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	TObjectPtr<USFNarrativeComponent> NarrativeComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	TArray<FSFQuestDisplayEntry> PendingQueue;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	FSFQuestDisplayEntry CurrentEntry;

	UPROPERTY(BlueprintReadOnly, Category = "Narrative|Quest|UI")
	bool bIsShowingToast = false;

	UFUNCTION()
	void HandleQuestStarted(USFQuestInstance* QuestInstance);

	UFUNCTION()
	void TickAutoResolveRetry();

	UFUNCTION()
	void HandleToastDurationElapsed();

	UFUNCTION()
	void HandleInterToastGapElapsed();

	bool TryAutoResolveNarrativeComponent();
	void BindToNarrativeComponent();
	void UnbindFromNarrativeComponent();

	void StartAutoResolveRetryTimer();
	void StopAutoResolveRetryTimer();

	/** Convert a quest runtime instance into a UI-friendly display entry. */
	bool BuildEntryForInstance(USFQuestInstance* Instance, FSFQuestDisplayEntry& OutEntry) const;

	/** Dequeue and show the next pending entry, or no-op if queue empty. */
	void TryShowNextQueued();

	/** Apply text bindings + toggle visibility + fire BP hook. */
	void ShowEntry(const FSFQuestDisplayEntry& Entry);

	/** Hide + fire BP hook + start gap timer to flush queue. */
	void DismissCurrent();

	/** Designer-side animation hooks. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Narrative|Quest|UI")
	void BP_OnNotificationShown(const FSFQuestDisplayEntry& Entry);

	UFUNCTION(BlueprintImplementableEvent, Category = "Narrative|Quest|UI")
	void BP_OnNotificationDismissed();

	// --- Timer / state ---

	FTimerHandle AutoResolveRetryHandle;
	float AutoResolveRetryElapsedSeconds = 0.0f;

	FTimerHandle ToastDurationHandle;
	FTimerHandle InterToastGapHandle;

	/** Time at which we bound to the narrative component. Used by SuppressInitialBroadcastsForSeconds. */
	double BindTimestampSeconds = 0.0;
};
