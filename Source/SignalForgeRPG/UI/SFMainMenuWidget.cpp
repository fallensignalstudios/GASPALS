#include "UI/SFMainMenuWidget.h"

#include "UI/SFSaveSlotListWidget.h"
#include "Save/SFPlayerSaveService.h"
#include "Core/SignalForgeGameInstance.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/App.h"

#define LOCTEXT_NAMESPACE "SFMainMenu"

DEFINE_LOG_CATEGORY_STATIC(LogSFMainMenu, Log, All);

// =============================================================================
// Construct / Destruct
// =============================================================================

void USFMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindButtons();
	BindToService();

	// Default visibility -- show the main column, hide the load-game subpanel.
	if (bStartWithLoadPanelHidden)
	{
		if (LoadGamePanel)
		{
			LoadGamePanel->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (MainButtonColumn)
		{
			MainButtonColumn->SetVisibility(ESlateVisibility::Visible);
		}
	}

	// Default version string -- BP can override the text manually after construct.
	if (VersionText)
	{
		VersionText->SetText(FText::FromString(FApp::GetProjectName() + FString(TEXT(" ")) + FApp::GetBuildVersion()));
	}

	if (bAutoRefreshOnConstruct)
	{
		RefreshContinueState();
	}

	BP_OnMainMenuShown();
}

void USFMainMenuWidget::NativeDestruct()
{
	UnbindButtons();
	UnbindFromService();
	Super::NativeDestruct();
}

// =============================================================================
// Service accessor
// =============================================================================

USFPlayerSaveService* USFMainMenuWidget::GetSaveService() const
{
	if (CachedService)
	{
		return CachedService;
	}
	if (UGameInstance* GI = GetGameInstance())
	{
		const_cast<USFMainMenuWidget*>(this)->CachedService = GI->GetSubsystem<USFPlayerSaveService>();
	}
	return CachedService;
}

// =============================================================================
// Continue / save query
// =============================================================================

void USFMainMenuWidget::RefreshContinueState()
{
	bHasAnyValidSave = false;
	MostRecentSlotName.Reset();
	MostRecentSlotInfo = FSFPlayerSaveSlotInfo{};

	USFPlayerSaveService* Service = GetSaveService();
	if (!Service)
	{
		UpdateStatus(LOCTEXT("NoService", "Save service unavailable."));
	}
	else
	{
		TArray<FSFPlayerSaveSlotInfo> Infos = Service->GetAllSlotInfos(UserIndex);

		// Newest first, valid slots only.
		const FSFPlayerSaveSlotInfo* Best = nullptr;
		for (const FSFPlayerSaveSlotInfo& Info : Infos)
		{
			if (!Info.bIsValid)
			{
				continue;
			}
			if (!Best || Info.SaveTimestamp > Best->SaveTimestamp)
			{
				Best = &Info;
			}
		}

		if (Best)
		{
			bHasAnyValidSave = true;
			MostRecentSlotInfo = *Best;
			MostRecentSlotName = Best->SlotName;
		}
	}

	if (ContinueButton)
	{
		ContinueButton->SetIsEnabled(bHasAnyValidSave);
	}

	if (MostRecentSaveText)
	{
		if (bHasAnyValidSave)
		{
			const FText Label = MostRecentSlotInfo.FriendlyName.IsEmpty()
				? FText::FromString(MostRecentSlotInfo.SlotName)
				: FText::FromString(MostRecentSlotInfo.FriendlyName);
			MostRecentSaveText->SetText(FText::Format(LOCTEXT("MostRecentFmt", "Continue: {0}"), Label));
			MostRecentSaveText->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			MostRecentSaveText->SetText(LOCTEXT("NoSavesYet", "No saves yet"));
			MostRecentSaveText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	BP_OnContinueStateChanged(bHasAnyValidSave, MostRecentSlotInfo);
}

// =============================================================================
// Public actions
// =============================================================================

bool USFMainMenuWidget::ContinueFromMostRecentSave()
{
	USFPlayerSaveService* Service = GetSaveService();
	if (!Service)
	{
		UpdateStatus(LOCTEXT("NoServiceContinue", "Cannot continue: save service unavailable."));
		return false;
	}

	if (!bHasAnyValidSave || MostRecentSlotName.IsEmpty())
	{
		UpdateStatus(LOCTEXT("NoSavesToContinue", "No saved games to continue."));
		return false;
	}

	UpdateStatus(FText::Format(LOCTEXT("ContinuingFmt", "Continuing {0}..."), FText::FromString(MostRecentSlotName)));

	BP_OnContinueClicked();

	const bool bStarted = Service->BeginLoadFromSlot(MostRecentSlotName, UserIndex);
	if (!bStarted)
	{
		UE_LOG(LogSFMainMenu, Warning, TEXT("ContinueFromMostRecentSave[%s]: BeginLoadFromSlot rejected the request."), *MostRecentSlotName);
		UpdateStatus(LOCTEXT("ContinueFailed", "Could not continue. The save may be corrupt."));
	}
	return bStarted;
}

void USFMainMenuWidget::StartNewGame()
{
	if (StartingMapName.IsNone())
	{
		UE_LOG(LogSFMainMenu, Warning, TEXT("StartNewGame: StartingMapName is None; nothing to open."));
		UpdateStatus(LOCTEXT("NoStartMap", "Starting map not configured."));
		return;
	}

	// Make absolutely sure no stale pending-load request leaks into the new game.
	if (USignalForgeGameInstance* GI = Cast<USignalForgeGameInstance>(GetGameInstance()))
	{
		GI->ClearPendingLoad();
	}

	UpdateStatus(LOCTEXT("NewGameStarting", "Starting new game..."));
	BP_OnNewGameConfirmed();

	UGameplayStatics::OpenLevel(this, StartingMapName);
}

void USFMainMenuWidget::ShowLoadGamePanel()
{
	if (LoadGamePanel)
	{
		LoadGamePanel->SetVisibility(ESlateVisibility::Visible);
		// Refresh once on entry so the panel reflects the latest disk state.
		LoadGamePanel->RefreshSlotList();
	}
	if (MainButtonColumn)
	{
		MainButtonColumn->SetVisibility(ESlateVisibility::Collapsed);
	}
	BP_OnLoadGameShown(true);
}

void USFMainMenuWidget::HideLoadGamePanel()
{
	if (LoadGamePanel)
	{
		LoadGamePanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (MainButtonColumn)
	{
		MainButtonColumn->SetVisibility(ESlateVisibility::Visible);
	}
	BP_OnLoadGameShown(false);
}

void USFMainMenuWidget::QuitGame()
{
	APlayerController* PC = GetOwningPlayer();
	UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, /*bIgnorePlatformRestrictions*/ false);
}

// =============================================================================
// Button handlers
// =============================================================================

void USFMainMenuWidget::HandleContinueClicked()
{
	ContinueFromMostRecentSave();
}

void USFMainMenuWidget::HandleNewGameClicked()
{
	StartNewGame();
}

void USFMainMenuWidget::HandleLoadGameClicked()
{
	ShowLoadGamePanel();
}

void USFMainMenuWidget::HandleQuitClicked()
{
	QuitGame();
}

void USFMainMenuWidget::HandleBackClicked()
{
	HideLoadGamePanel();
}

void USFMainMenuWidget::HandleServiceSlotListChanged(const FString& /*SlotName*/)
{
	RefreshContinueState();
}

// =============================================================================
// Binding helpers
// =============================================================================

void USFMainMenuWidget::BindButtons()
{
	if (ContinueButton)
	{
		ContinueButton->OnClicked.AddUniqueDynamic(this, &USFMainMenuWidget::HandleContinueClicked);
	}
	if (NewGameButton)
	{
		NewGameButton->OnClicked.AddUniqueDynamic(this, &USFMainMenuWidget::HandleNewGameClicked);
	}
	if (LoadGameButton)
	{
		LoadGameButton->OnClicked.AddUniqueDynamic(this, &USFMainMenuWidget::HandleLoadGameClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddUniqueDynamic(this, &USFMainMenuWidget::HandleQuitClicked);
	}
	if (BackButton)
	{
		BackButton->OnClicked.AddUniqueDynamic(this, &USFMainMenuWidget::HandleBackClicked);
	}
}

void USFMainMenuWidget::UnbindButtons()
{
	if (ContinueButton)
	{
		ContinueButton->OnClicked.RemoveDynamic(this, &USFMainMenuWidget::HandleContinueClicked);
	}
	if (NewGameButton)
	{
		NewGameButton->OnClicked.RemoveDynamic(this, &USFMainMenuWidget::HandleNewGameClicked);
	}
	if (LoadGameButton)
	{
		LoadGameButton->OnClicked.RemoveDynamic(this, &USFMainMenuWidget::HandleLoadGameClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.RemoveDynamic(this, &USFMainMenuWidget::HandleQuitClicked);
	}
	if (BackButton)
	{
		BackButton->OnClicked.RemoveDynamic(this, &USFMainMenuWidget::HandleBackClicked);
	}
}

void USFMainMenuWidget::BindToService()
{
	USFPlayerSaveService* Service = GetSaveService();
	if (!Service)
	{
		return;
	}
	Service->OnSlotListChanged.AddUniqueDynamic(this, &USFMainMenuWidget::HandleServiceSlotListChanged);
}

void USFMainMenuWidget::UnbindFromService()
{
	if (CachedService)
	{
		CachedService->OnSlotListChanged.RemoveDynamic(this, &USFMainMenuWidget::HandleServiceSlotListChanged);
	}
}

void USFMainMenuWidget::UpdateStatus(const FText& Message)
{
	if (StatusText)
	{
		StatusText->SetText(Message);
	}
}

#undef LOCTEXT_NAMESPACE
