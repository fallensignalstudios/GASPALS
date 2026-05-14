// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "UI/SFQuestNotificationToastWidget.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Core/SFPlayerState.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Narrative/SFNarrativeComponent.h"
#include "Narrative/SFQuestDefinition.h"
#include "Narrative/SFQuestInstance.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogSFQuestToast, Log, All);

#define LOCTEXT_NAMESPACE "SFQuestNotificationToastWidget"

// =============================================================================
// Construction / destruction
// =============================================================================

void USFQuestNotificationToastWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (HeaderLabel.IsEmpty())
	{
		HeaderLabel = LOCTEXT("DefaultHeader", "New Quest");
	}

	// Start hidden; we only show on actual events.
	if (RootContainer)
	{
		RootContainer->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Try to find the local player's narrative component immediately. If not
	// ready (PlayerState hasn't replicated, etc.), start a low-frequency retry.
	if (!TryAutoResolveNarrativeComponent())
	{
		StartAutoResolveRetryTimer();
	}
}

void USFQuestNotificationToastWidget::NativeDestruct()
{
	StopAutoResolveRetryTimer();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ToastDurationHandle);
		World->GetTimerManager().ClearTimer(InterToastGapHandle);
	}

	UnbindFromNarrativeComponent();

	Super::NativeDestruct();
}

// =============================================================================
// Public API
// =============================================================================

void USFQuestNotificationToastWidget::InitializeToast(USFNarrativeComponent* InNarrativeComponent)
{
	UnbindFromNarrativeComponent();
	NarrativeComponent = InNarrativeComponent;
	BindToNarrativeComponent();

	if (NarrativeComponent)
	{
		StopAutoResolveRetryTimer();
	}
}

void USFQuestNotificationToastWidget::DeinitializeToast()
{
	StopAutoResolveRetryTimer();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ToastDurationHandle);
		World->GetTimerManager().ClearTimer(InterToastGapHandle);
	}
	PendingQueue.Reset();
	bIsShowingToast = false;
	UnbindFromNarrativeComponent();
}

void USFQuestNotificationToastWidget::ShowToastForEntry(const FSFQuestDisplayEntry& Entry)
{
	if (bIsShowingToast)
	{
		PendingQueue.Add(Entry);
		return;
	}
	ShowEntry(Entry);
}

void USFQuestNotificationToastWidget::DismissAndClearQueue()
{
	PendingQueue.Reset();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ToastDurationHandle);
		World->GetTimerManager().ClearTimer(InterToastGapHandle);
	}

	if (bIsShowingToast)
	{
		DismissCurrent();
	}
}

// =============================================================================
// Auto-resolve
// =============================================================================

bool USFQuestNotificationToastWidget::TryAutoResolveNarrativeComponent()
{
	if (NarrativeComponent)
	{
		return true;
	}

	APlayerController* PC = GetOwningPlayer();
	APlayerState* PS = PC ? PC->PlayerState : nullptr;
	ASFPlayerState* SFPS = Cast<ASFPlayerState>(PS);
	USFNarrativeComponent* NC = SFPS ? SFPS->GetNarrativeComponent() : nullptr;

	if (!NC)
	{
		return false;
	}

	NarrativeComponent = NC;
	BindToNarrativeComponent();

	UE_LOG(LogSFQuestToast, Log,
		TEXT("[QuestToast] Auto-resolved NarrativeComponent from %s."),
		*GetNameSafe(SFPS));
	return true;
}

void USFQuestNotificationToastWidget::StartAutoResolveRetryTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (World->GetTimerManager().IsTimerActive(AutoResolveRetryHandle))
	{
		return;
	}
	AutoResolveRetryElapsedSeconds = 0.0f;
	const float Period = FMath::Max(0.1f, AutoResolveRetryPeriodSeconds);
	World->GetTimerManager().SetTimer(
		AutoResolveRetryHandle,
		this,
		&USFQuestNotificationToastWidget::TickAutoResolveRetry,
		Period,
		/*bLoop=*/true);
}

void USFQuestNotificationToastWidget::StopAutoResolveRetryTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoResolveRetryHandle);
	}
	AutoResolveRetryElapsedSeconds = 0.0f;
}

void USFQuestNotificationToastWidget::TickAutoResolveRetry()
{
	if (TryAutoResolveNarrativeComponent())
	{
		StopAutoResolveRetryTimer();
		return;
	}

	AutoResolveRetryElapsedSeconds += FMath::Max(0.1f, AutoResolveRetryPeriodSeconds);
	if (AutoResolveRetryElapsedSeconds >= MaxAutoResolveRetrySeconds)
	{
		UE_LOG(LogSFQuestToast, Warning,
			TEXT("[QuestToast] Gave up auto-resolving NarrativeComponent after %.1fs. ")
			TEXT("Quest start toasts will not appear. Confirm GameMode->PlayerStateClass is ASFPlayerState (or a subclass) and that the player has a USFNarrativeComponent."),
			AutoResolveRetryElapsedSeconds);
		StopAutoResolveRetryTimer();
	}
}

// =============================================================================
// Binding
// =============================================================================

void USFQuestNotificationToastWidget::BindToNarrativeComponent()
{
	if (!NarrativeComponent)
	{
		return;
	}
	if (!NarrativeComponent->OnQuestStarted.IsAlreadyBound(this, &USFQuestNotificationToastWidget::HandleQuestStarted))
	{
		NarrativeComponent->OnQuestStarted.AddDynamic(this, &USFQuestNotificationToastWidget::HandleQuestStarted);
	}

	if (UWorld* World = GetWorld())
	{
		BindTimestampSeconds = World->GetTimeSeconds();
	}
}

void USFQuestNotificationToastWidget::UnbindFromNarrativeComponent()
{
	if (!NarrativeComponent)
	{
		return;
	}
	NarrativeComponent->OnQuestStarted.RemoveDynamic(this, &USFQuestNotificationToastWidget::HandleQuestStarted);
	NarrativeComponent = nullptr;
}

// =============================================================================
// Quest start handler
// =============================================================================

void USFQuestNotificationToastWidget::HandleQuestStarted(USFQuestInstance* QuestInstance)
{
	if (!QuestInstance)
	{
		return;
	}

	// Optional debounce: suppress toasts in the brief window after binding so
	// that "quest just replayed from save" doesn't spam a stack of toasts.
	if (SuppressInitialBroadcastsForSeconds > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			const double Now = World->GetTimeSeconds();
			if (Now - BindTimestampSeconds < SuppressInitialBroadcastsForSeconds)
			{
				return;
			}
		}
	}

	FSFQuestDisplayEntry Entry;
	if (!BuildEntryForInstance(QuestInstance, Entry))
	{
		return;
	}

	ShowToastForEntry(Entry);
}

// =============================================================================
// Display entry construction (lean version of the controller's builder)
// =============================================================================

bool USFQuestNotificationToastWidget::BuildEntryForInstance(USFQuestInstance* Instance, FSFQuestDisplayEntry& OutEntry) const
{
	if (!Instance)
	{
		return false;
	}
	const USFQuestDefinition* Def = Instance->GetDefinition();
	if (!Def)
	{
		return false;
	}

	OutEntry = FSFQuestDisplayEntry{};
	OutEntry.QuestId = Def->QuestId;
	OutEntry.QuestAssetId = Def->GetPrimaryAssetId();
	OutEntry.Definition = Def;
	OutEntry.DisplayName = Def->DisplayName.IsEmpty()
		? FText::FromName(Def->QuestId)
		: Def->DisplayName;
	OutEntry.Description = Def->Description;
	OutEntry.QuestTags = Def->QuestTags;
	OutEntry.CompletionState = Instance->GetCompletionState();
	OutEntry.CurrentStateId = Instance->GetCurrentStateId();

	for (const FSFQuestStateDefinition& State : Def->States)
	{
		if (State.StateId == OutEntry.CurrentStateId)
		{
			OutEntry.CurrentStateDisplayName = State.DisplayName.IsEmpty()
				? FText::FromName(State.StateId)
				: State.DisplayName;
			break;
		}
	}

	return true;
}

// =============================================================================
// Show / dismiss flow
// =============================================================================

void USFQuestNotificationToastWidget::TryShowNextQueued()
{
	if (PendingQueue.Num() == 0)
	{
		return;
	}
	const FSFQuestDisplayEntry Next = PendingQueue[0];
	PendingQueue.RemoveAt(0);
	ShowEntry(Next);
}

void USFQuestNotificationToastWidget::ShowEntry(const FSFQuestDisplayEntry& Entry)
{
	CurrentEntry = Entry;
	bIsShowingToast = true;

	if (HeaderText)
	{
		HeaderText->SetText(HeaderLabel);
	}
	if (QuestNameText)
	{
		QuestNameText->SetText(Entry.DisplayName);
	}
	if (QuestStateText)
	{
		QuestStateText->SetText(Entry.CurrentStateDisplayName);
	}
	if (QuestDescriptionText)
	{
		QuestDescriptionText->SetText(Entry.Description);
	}

	if (RootContainer)
	{
		RootContainer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	BP_OnNotificationShown(Entry);

	if (UWorld* World = GetWorld())
	{
		const float Duration = FMath::Max(0.5f, DisplayDurationSeconds);
		World->GetTimerManager().SetTimer(
			ToastDurationHandle,
			this,
			&USFQuestNotificationToastWidget::HandleToastDurationElapsed,
			Duration,
			/*bLoop=*/false);
	}
}

void USFQuestNotificationToastWidget::HandleToastDurationElapsed()
{
	DismissCurrent();
}

void USFQuestNotificationToastWidget::DismissCurrent()
{
	bIsShowingToast = false;
	CurrentEntry = FSFQuestDisplayEntry{};

	if (RootContainer)
	{
		RootContainer->SetVisibility(ESlateVisibility::Collapsed);
	}

	BP_OnNotificationDismissed();

	// If more toasts are queued, schedule the next one after the configured gap.
	if (PendingQueue.Num() == 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float Gap = FMath::Max(0.0f, InterToastGapSeconds);
	if (Gap <= KINDA_SMALL_NUMBER)
	{
		TryShowNextQueued();
		return;
	}

	World->GetTimerManager().SetTimer(
		InterToastGapHandle,
		this,
		&USFQuestNotificationToastWidget::HandleInterToastGapElapsed,
		Gap,
		/*bLoop=*/false);
}

void USFQuestNotificationToastWidget::HandleInterToastGapElapsed()
{
	TryShowNextQueued();
}

#undef LOCTEXT_NAMESPACE
