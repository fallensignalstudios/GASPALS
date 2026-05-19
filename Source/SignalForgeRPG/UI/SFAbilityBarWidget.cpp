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
		// Already bound — just resync visuals.
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

	SlotsContainer->ClearChildren();
	SlotWidgets.Reset();

	for (const FSFAbilityBarSlotLayout& Layout : SlotLayout)
	{
		if (!Layout.InputTag.IsValid())
		{
			continue;
		}

		USFAbilitySlotWidget* Slot = CreateWidget<USFAbilitySlotWidget>(this, SlotWidgetClass);
		if (!Slot)
		{
			continue;
		}

		Slot->SetInputTag(Layout.InputTag);
		Slot->HotkeyLabel = Layout.HotkeyLabel;

		SlotsContainer->AddChild(Slot);
		SlotWidgets.Add(Layout.InputTag, Slot);
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
		if (USFAbilitySlotWidget* Slot = GetSlotForTag(Data.InputTag))
		{
			Slot->SetSlotData(Data);
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
	if (USFAbilitySlotWidget* Slot = GetSlotForTag(InputTag))
	{
		Slot->SetSlotData(SlotData);
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
