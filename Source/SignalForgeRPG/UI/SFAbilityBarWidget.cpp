#include "UI/SFAbilityBarWidget.h"

#include "Characters/SFCharacterBase.h"
#include "Components/PanelWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Input/SFPlayerController.h"
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

	// Prefer the PlayerController's existing AbilityBarWidgetController when
	// available — it is created in ASFPlayerController::InitializeUIControllers
	// and stays subscribed to OnAbilitiesChanged for the lifetime of the PC.
	// This avoids a race where the bar widget's own controller is created and
	// destroyed across HUD rebuilds before late ability grants arrive.
	ASFPlayerController* SFPlayerController =
		Cast<ASFPlayerController>(BoundCharacter->GetController());
	if (!SFPlayerController)
	{
		SFPlayerController = Cast<ASFPlayerController>(
			UGameplayStatics::GetPlayerController(this, 0));
	}

	if (SFPlayerController && SFPlayerController->GetAbilityBarWidgetController())
	{
		AbilityController = SFPlayerController->GetAbilityBarWidgetController();
		bOwnsAbilityController = false;
		UE_LOG(LogSFAbilityBar, Log,
			TEXT("BindToPlayer: reusing PlayerController-owned AbilityBarWidgetController (%p)."),
			AbilityController.Get());
	}
	else
	{
		AbilityController = NewObject<USFAbilityBarWidgetController>(this);
		bOwnsAbilityController = true;
		if (!AbilityController)
		{
			return;
		}
		AbilityController->Initialize(BoundCharacter);
		UE_LOG(LogSFAbilityBar, Log,
			TEXT("BindToPlayer: no PC-owned controller; created bar-owned one (%p)."),
			AbilityController.Get());
	}

	AbilityController->OnAbilitySlotUpdated.AddDynamic(this, &USFAbilityBarWidget::HandleSlotUpdated);

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
		// Only forget the pointer — never destroy a controller we don't own.
		AbilityController = nullptr;
	}
	bOwnsAbilityController = false;
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

	UE_LOG(LogSFAbilityBar, Log,
		TEXT("RebuildSlotWidgets: iterating %d SlotLayout entries on '%s'."),
		SlotLayout.Num(), *GetNameSafe(this));

	int32 LayoutIndex = 0;
	for (const FSFAbilityBarSlotLayout& Layout : SlotLayout)
	{
		UE_LOG(LogSFAbilityBar, Log,
			TEXT("  SlotLayout[%d]: InputTag='%s' IsValid=%d HotkeyLabel='%s'"),
			LayoutIndex,
			*Layout.InputTag.ToString(),
			(int32)Layout.InputTag.IsValid(),
			*Layout.HotkeyLabel.ToString());
		++LayoutIndex;

		if (!Layout.InputTag.IsValid())
		{
			UE_LOG(LogSFAbilityBar, Warning,
				TEXT("    -> skipped: InputTag is not valid."));
			continue;
		}

		USFAbilitySlotWidget* SlotWidget = CreateWidget<USFAbilitySlotWidget>(this, SlotWidgetClass);
		if (!SlotWidget)
		{
			UE_LOG(LogSFAbilityBar, Warning,
				TEXT("    -> CreateWidget returned null for class '%s'."),
				*GetNameSafe(SlotWidgetClass.Get()));
			continue;
		}

		SlotWidget->SetInputTag(Layout.InputTag);
		SlotWidget->SetHotkeyLabel(Layout.HotkeyLabel);

		SlotsContainer->AddChild(SlotWidget);
		SlotWidgets.Add(Layout.InputTag, SlotWidget);

		UE_LOG(LogSFAbilityBar, Log,
			TEXT("    -> created slot widget '%s' added under '%s'."),
			*GetNameSafe(SlotWidget), *GetNameSafe(SlotsContainer));
	}

	UE_LOG(LogSFAbilityBar, Log,
		TEXT("RebuildSlotWidgets done: SlotWidgets.Num()=%d, SlotsContainer children=%d."),
		SlotWidgets.Num(),
		SlotsContainer ? SlotsContainer->GetChildrenCount() : -1);
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
	USFAbilitySlotWidget* SlotWidget = GetSlotForTag(InputTag);
	if (SlotWidget)
	{
		UE_LOG(LogSFAbilityBar, Verbose,
			TEXT("HandleSlotUpdated: tag='%s' -> matched slot widget '%s'."),
			*InputTag.ToString(), *GetNameSafe(SlotWidget));
		SlotWidget->SetSlotData(SlotData);
	}
	else
	{
		FString KnownTags;
		for (const TPair<FGameplayTag, TObjectPtr<USFAbilitySlotWidget>>& Pair : SlotWidgets)
		{
			if (!KnownTags.IsEmpty()) { KnownTags += TEXT(", "); }
			KnownTags += Pair.Key.ToString();
		}
		UE_LOG(LogSFAbilityBar, Warning,
			TEXT("HandleSlotUpdated: NO MATCHING SLOT for InputTag='%s'. SlotsContainer bound=%d, SlotWidgetClass set=%d, SlotWidgets count=%d. Known layout tags: [%s]."),
			*InputTag.ToString(),
			(int32)(SlotsContainer != nullptr),
			(int32)(SlotWidgetClass != nullptr),
			SlotWidgets.Num(),
			*KnownTags);
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
