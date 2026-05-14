#pragma once

#include "CoreMinimal.h"
#include "UI/SFUserWidgetBase.h"
#include "Save/SFPlayerSaveTypes.h"
#include "SFMainMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UWidget;
class UWorld;
class USFPlayerSaveService;
class USFSaveSlotListWidget;

/**
 * USFMainMenuWidget
 *
 * The front-of-house menu the player sees when launching the game. Drives:
 *
 *   - Continue: load the most recent valid save via the travel-then-apply
 *     path (USFPlayerSaveService::BeginLoadFromSlot). The destination map's
 *     GameMode/PlayerController must call ConsumePendingLoadAndApply on
 *     BeginPlay to finish the load.
 *   - New Game: OpenLevel to the starting map (clears any pending load
 *     request first so a stale one doesn't fire on the new game).
 *   - Load Game: reveals an embedded USFSaveSlotListWidget configured with
 *     bUseBeginLoadForLoadButton = true so its row Load buttons also use
 *     the travel-then-apply path.
 *   - Quit: QuitGame.
 *
 * UMG layout authoring stays in Blueprint. Required setup in the WBP
 * subclass:
 *   1. Reparent your main-menu WBP to this class.
 *   2. (Optional) Bind any of ContinueButton / NewGameButton / LoadGameButton
 *      / QuitButton / BackButton -- all are BindWidgetOptional, so missing
 *      bindings are silently skipped.
 *   3. (Optional) Bind VersionText / MostRecentSaveText / StatusText.
 *   4. (Optional) Embed a USFSaveSlotListWidget named LoadGamePanel for the
 *      Load Game flow. The widget will toggle its visibility for you.
 *   5. Set StartingMapName on the class defaults to the FName of your
 *      gameplay start map (e.g. "L_Tutorial_01").
 *
 * The widget self-binds button OnClicked handlers in NativeConstruct and
 * unbinds them in NativeDestruct -- no Blueprint glue required.
 */
UCLASS(Blueprintable)
class SIGNALFORGERPG_API USFMainMenuWidget : public USFUserWidgetBase
{
	GENERATED_BODY()

public:
	/** Programmatic entry point. Same path the Continue button uses. */
	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	bool ContinueFromMostRecentSave();

	/** Programmatic entry point. Travels to StartingMapName. */
	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void StartNewGame();

	/** Programmatic entry point. Shows the embedded LoadGamePanel. */
	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void ShowLoadGamePanel();

	/** Programmatic entry point. Hides the embedded LoadGamePanel. */
	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void HideLoadGamePanel();

	/** Programmatic entry point. Calls UKismetSystemLibrary::QuitGame. */
	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void QuitGame();

	/** Re-pull save-slot info and refresh Continue button enabled state + label. */
	UFUNCTION(BlueprintCallable, Category = "Main Menu")
	void RefreshContinueState();

	/** Resolves the service from the owning game instance. Cached after first call. */
	UFUNCTION(BlueprintPure, Category = "Main Menu")
	USFPlayerSaveService* GetSaveService() const;

	UFUNCTION(BlueprintPure, Category = "Main Menu")
	bool HasAnyValidSave() const { return bHasAnyValidSave; }

	UFUNCTION(BlueprintPure, Category = "Main Menu")
	FString GetMostRecentSlotName() const { return MostRecentSlotName; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// -------------------------------------------------------------------------
	// Buttons (all optional -- bind only the ones your layout exposes)
	// -------------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, Category = "Main Menu", meta = (BindWidgetOptional))
	TObjectPtr<UButton> ContinueButton = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Main Menu", meta = (BindWidgetOptional))
	TObjectPtr<UButton> NewGameButton = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Main Menu", meta = (BindWidgetOptional))
	TObjectPtr<UButton> LoadGameButton = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Main Menu", meta = (BindWidgetOptional))
	TObjectPtr<UButton> QuitButton = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Main Menu", meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackButton = nullptr;

	// -------------------------------------------------------------------------
	// Text (all optional)
	// -------------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, Category = "Main Menu", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> VersionText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Main Menu", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MostRecentSaveText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Main Menu", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText = nullptr;

	// -------------------------------------------------------------------------
	// Panels (optional)
	// -------------------------------------------------------------------------

	/**
	 * Optional embedded slot-browser panel. When the player clicks Load Game,
	 * this is set Visible (and the main button column is set Collapsed via
	 * BP_OnLoadGameShown). The author should configure this widget's
	 * bUseBeginLoadForLoadButton = true so its row loads also travel.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Main Menu", meta = (BindWidgetOptional))
	TObjectPtr<USFSaveSlotListWidget> LoadGamePanel = nullptr;

	/**
	 * Optional container that wraps the Continue / New Game / Load Game / Quit
	 * stack so the widget can hide it when Load Game is open. If left null,
	 * the BP_OnLoadGameShown hook is the only signal the layout has to swap.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Main Menu", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> MainButtonColumn = nullptr;

	// -------------------------------------------------------------------------
	// Class defaults
	// -------------------------------------------------------------------------

	/**
	 * FName of the gameplay start map opened when the player picks New Game.
	 * Plain FName (not TSoftObjectPtr<UWorld>) so it round-trips OpenLevel
	 * predictably in packaged builds.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Main Menu")
	FName StartingMapName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Main Menu")
	int32 UserIndex = 0;

	/** Whether NativeConstruct should call RefreshContinueState() automatically. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Main Menu")
	bool bAutoRefreshOnConstruct = true;

	/** Whether NativeConstruct should hide LoadGamePanel + show MainButtonColumn. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Main Menu")
	bool bStartWithLoadPanelHidden = true;

	// -------------------------------------------------------------------------
	// Resolved state -- BP-readable
	// -------------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, Category = "Main Menu")
	bool bHasAnyValidSave = false;

	UPROPERTY(BlueprintReadOnly, Category = "Main Menu")
	FString MostRecentSlotName;

	UPROPERTY(BlueprintReadOnly, Category = "Main Menu")
	FSFPlayerSaveSlotInfo MostRecentSlotInfo;

	// -------------------------------------------------------------------------
	// Button handlers
	// -------------------------------------------------------------------------

	UFUNCTION()
	void HandleContinueClicked();

	UFUNCTION()
	void HandleNewGameClicked();

	UFUNCTION()
	void HandleLoadGameClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UFUNCTION()
	void HandleBackClicked();

	/** Re-runs RefreshContinueState whenever the slot list changes. */
	UFUNCTION()
	void HandleServiceSlotListChanged(const FString& SlotName);

	// -------------------------------------------------------------------------
	// BP hooks (override in your WBP to animate / swap subpanels)
	// -------------------------------------------------------------------------

	UFUNCTION(BlueprintImplementableEvent, Category = "Main Menu")
	void BP_OnMainMenuShown();

	UFUNCTION(BlueprintImplementableEvent, Category = "Main Menu")
	void BP_OnLoadGameShown(bool bShown);

	UFUNCTION(BlueprintImplementableEvent, Category = "Main Menu")
	void BP_OnContinueClicked();

	UFUNCTION(BlueprintImplementableEvent, Category = "Main Menu")
	void BP_OnNewGameConfirmed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Main Menu")
	void BP_OnContinueStateChanged(bool bHasValidSave, const FSFPlayerSaveSlotInfo& MostRecent);

private:
	void BindButtons();
	void UnbindButtons();
	void BindToService();
	void UnbindFromService();
	void UpdateStatus(const FText& Message);

	UPROPERTY()
	TObjectPtr<USFPlayerSaveService> CachedService = nullptr;
};
