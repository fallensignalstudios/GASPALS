#include "UI/SFAbilityBarWidget.h"

#include "Characters/SFCharacterBase.h"
#include "Components/PanelWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UI/SFAbilityBarWidgetController.h"
#include "UI/SFAbilitySlotWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogSFAbilityBar, Log, All);

void USFAbilityBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UE_LOG(LogSFAbilityBar, Log,
		TEXT("AbilityBar '%s' constructing. SlotLayout entries=%d, SlotWidgetClass=%s, bAutoBindOnConstruct=%d"),
		*GetNameSafe(this), SlotLayout.Num(), *GetNameSafe(SlotWidgetClass.Get()), (int32)bAutoBindOnConstruct);

	RebuildSlotWidgets();

	if (bAutoBindOnConstruct)
	{
		BindToLocalPlayer();
	}
}

void USFAbilityBarWidget::NativeDestruct()
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->OnPossessedPawnChanged.RemoveDynamic(this, &USFAbilityBarWidget::HandlePossessedPawnChanged);
	}

	TeardownController();
	BoundCharacter = nullptr;

	Super::NativeDestruct();
}

void USFAbilityBarWidget::BindToLocalPlayer()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		UE_LOG(LogSFAbilityBar, Verbose, TEXT("BindToLocalPlayer: no PlayerController yet."));
		return;
	}

	// Track future pawn changes so the bar follows the player.
	PC->OnPossessedPawnChanged.RemoveDynamic(this, &USFAbilityBarWidget::HandlePossessedPawnChanged);
	PC->OnPossessedPawnChanged.AddDynamic(this, &USFAbilityBarWidget::HandlePossessedPawnChanged);

	ASFCharacterBase* Character = Cast<ASFCharacterBase>(PC->GetPawn());
	if (!Character)
	{
		UE_LOG(LogSFAbilityBar, Verbose,
			TEXT("BindToLocalPlayer: local pawn is not an ASFCharacterBase yet (is '%s'); will rebind on possess."),
			*GetNameSafe(PC->GetPawn()));
		return;
	}

	BindToPlayer(Character);
}

void USFAbilityBarWidget::BindToPlayer(ASFCharacterBase* InPlayer)
{
	if (BoundCharacter == InPlayer && AbilityController)
	{
		UE_LOG(LogSFAbilityBar, Verbose, TEXT("BindToPlayer: already bound to '%s'; resyncing."), *GetNameSafe(InPlayer));
		RefreshSlotsFromController();
		return;
	}

	TeardownController();

	BoundCharacter = InPlayer;
	if (!BoundCharacter)
	{
		return;
	}

	AbilityController = NewObject<USFAbilityBarWidgetController>(this);
	if (!AbilityController)
	{
		return;
	}

	AbilityController->OnAbilitySlotUpdated.AddDynamic(this, &USFAbilityBarWidget::HandleSlotUpdated);
	AbilityController->Initialize(BoundCharacter);

	// Diagnostic: how many abilities did the controller actually see?
	TArray<FSFAbilitySlotUIData> AllSlots;
	AbilityController->GetAllSlotData(AllSlots);
	UE_LOG(LogSFAbilityBar, Log,
		TEXT("AbilityBar bound to '%s'. Controller reports %d ability slot(s). HasInitialData=%d"),
		*GetNameSafe(InPlayer), AllSlots.Num(), (int32)AbilityController->HasInitialData());

	for (const FSFAbilitySlotUIData& Data : AllSlots)
	{
		const bool bMatchedLayout = SlotWidgets.Contains(Data.InputTag);
		UE_LOG(LogSFAbilityBar, Log,
			TEXT("  Slot: InputTag='%s' Icon=%s CooldownTag='%s' MatchesLayout=%d"),
			*Data.InputTag.ToString(), *GetNameSafe(Data.Icon),
			*Data.CooldownTag.ToString(), (int32)bMatchedLayout);
	}

	if (AllSlots.Num() == 0)
	{
		UE_LOG(LogSFAbilityBar, Warning,
			TEXT("AbilityBar: controller returned 0 slots. Common causes: "
			     "(1) abilities haven't been granted to the ASC yet, "
			     "(2) abilities don't have bShowInAbilityBar=true, "
			     "(3) abilities have no InputTag set, "
			     "(4) the pawn's ASC is not a USFAbilitySystemComponent."));
	}

	// Pull initial data in case Initialize broadcast before our binding completed
	// (defensive — AddDynamic is processed before Initialize, but this also catches
	// the case where the ASC granted abilities before the widget existed).
	RefreshSlotsFromController();
}

void USFAbilityBarWidget::TeardownController()
{
	if (AbilityController)
	{
		AbilityController->OnAbilitySlotUpdated.RemoveDynamic(this, &USFAbilityBarWidget::HandleSlotUpdated);
		AbilityController = nullptr;
	}
}

void USFAbilityBarWidget::RebuildSlotWidgets()
{
	if (!SlotsContainer)
	{
		UE_LOG(LogSFAbilityBar, Warning,
			TEXT("RebuildSlotWidgets: SlotsContainer is not bound on '%s'. Bind a UPanelWidget named 'SlotsContainer'."),
			*GetNameSafe(this));
		return;
	}

	if (!SlotWidgetClass)
	{
		UE_LOG(LogSFAbilityBar, Warning,
			TEXT("RebuildSlotWidgets: SlotWidgetClass is not set on '%s'. Set it in the bar's class defaults."),
			*GetNameSafe(this));
		return;
	}

	if (SlotLayout.Num() == 0)
	{
		UE_LOG(LogSFAbilityBar, Warning,
			TEXT("RebuildSlotWidgets: SlotLayout on '%s' is empty. Add one entry per visible slot with the matching InputTag."),
			*GetNameSafe(this));
	}

	SlotsContainer->ClearChildren();
	SlotWidgets.Reset();

	for (const FSFAbilityBarSlotLayout& Layout : SlotLayout)
	{
		if (!Layout.InputTag.IsValid())
		{
			continue;
		}

		USFAbilitySlotWidget* SlotWidget = CreateWidget<USFAbilitySlotWidget>(this, SlotWidgetClass);
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->SetInputTag(Layout.InputTag);
		SlotWidget->SetHotkeyLabel(Layout.HotkeyLabel);

		SlotsContainer->AddChild(SlotWidget);
		SlotWidgets.Add(Layout.InputTag, SlotWidget);
	}
}

void USFAbilityBarWidget::RefreshSlotsFromController()
{
	if (!AbilityController)
	{
		return;
	}

	TArray<FSFAbilitySlotUIData> AllSlots;
	AbilityController->GetAllSlotData(AllSlots);

	// First mark every laid-out slot empty so removed abilities clear correctly.
	for (TPair<FGameplayTag, TObjectPtr<USFAbilitySlotWidget>>& Pair : SlotWidgets)
	{
		if (Pair.Value)
		{
			FSFAbilitySlotUIData EmptyData;
			EmptyData.InputTag = Pair.Key;
			EmptyData.bHasAbility = false;
			EmptyData.bIsReady = false;
			Pair.Value->SetSlotData(EmptyData);
		}
	}

	for (const FSFAbilitySlotUIData& Data : AllSlots)
	{
		if (USFAbilitySlotWidget* SlotWidget = GetSlotForTag(Data.InputTag))
		{
			SlotWidget->SetSlotData(Data);
		}
	}
}

USFAbilitySlotWidget* USFAbilityBarWidget::GetSlotForTag(FGameplayTag InputTag) const
{
	if (const TObjectPtr<USFAbilitySlotWidget>* Found = SlotWidgets.Find(InputTag))
	{
		return Found->Get();
	}
	return nullptr;
}

void USFAbilityBarWidget::HandleSlotUpdated(FGameplayTag InputTag, FSFAbilitySlotUIData SlotData)
{
	if (USFAbilitySlotWidget* SlotWidget = GetSlotForTag(InputTag))
	{
		SlotWidget->SetSlotData(SlotData);
	}
}

void USFAbilityBarWidget::HandlePossessedPawnChanged(APawn* /*OldPawn*/, APawn* NewPawn)
{
	if (ASFCharacterBase* Character = Cast<ASFCharacterBase>(NewPawn))
	{
		BindToPlayer(Character);
	}
	else
	{
		TeardownController();
		BoundCharacter = nullptr;
	}
}
