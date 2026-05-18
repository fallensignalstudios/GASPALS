// Copyright Fallen Signal Studios LLC. All Rights Reserved.

#include "UI/SFCompassHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Fonts/SlateFontInfo.h"
#include "GameFramework/PlayerController.h"
#include "Narrative/SFNarrativeWaypointSubsystem.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogSFCompassHUD, Log, All);

namespace SFCompassHUD
{
	static const TCHAR* CardinalLabels[] = {
		TEXT("N"), TEXT("NE"), TEXT("E"), TEXT("SE"),
		TEXT("S"), TEXT("SW"), TEXT("W"), TEXT("NW")
	};
	static constexpr int32 NumCardinalLabels = UE_ARRAY_COUNT(CardinalLabels);
	static constexpr float CardinalStepDegrees = 360.0f / static_cast<float>(NumCardinalLabels);

	static bool BrushHasResource(const FSlateBrush& Brush)
	{
		return Brush.GetResourceObject() != nullptr;
	}

	// Build a UBorder configured as a sized solid-color rect.
	static UBorder* MakeSolidBorder(UWidgetTree& Tree, FName Name, const FLinearColor& Color, FVector2D DesiredSize)
	{
		UBorder* B = Tree.ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
		FSlateBrush Brush;
		Brush.ImageSize = DesiredSize;
		Brush.TintColor = FSlateColor(Color);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		B->SetBrush(Brush);
		B->SetPadding(FMargin(0.0f));
		return B;
	}
}

USFCompassHUDWidget::USFCompassHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

TSharedRef<SWidget> USFCompassHUDWidget::RebuildWidget()
{
	BuildVisualTree();
	return Super::RebuildWidget();
}

void USFCompassHUDWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	RootCanvas = nullptr;
	CompassSizeBox = nullptr;
	CompassOverlay = nullptr;
	BackgroundBorder = nullptr;
	StripCanvas = nullptr;
	CenterPipBorder = nullptr;
	CardinalLabelOverlays.Reset();
	MarkerWidgets.Reset();
	bVisualTreeBuilt = false;
}

void USFCompassHUDWidget::BuildVisualTree()
{
	if (bVisualTreeBuilt && RootCanvas)
	{
		return;
	}
	UWidgetTree* Tree = WidgetTree;
	if (!Tree)
	{
		return;
	}

	Tree->RemoveWidget(Tree->RootWidget);

	RootCanvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CompassRootCanvas"));
	Tree->RootWidget = RootCanvas;

	CompassSizeBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CompassSizeBox"));
	CompassSizeBox->SetWidthOverride(StripPixelWidth);
	CompassSizeBox->SetHeightOverride(StripPixelHeight);

	if (UCanvasPanelSlot* SizeSlot = RootCanvas->AddChildToCanvas(CompassSizeBox))
	{
		SizeSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
		SizeSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		SizeSlot->SetAutoSize(true);
		SizeSlot->SetPosition(FVector2D(0.0f, 0.0f));
	}

	CompassOverlay = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("CompassOverlay"));
	CompassSizeBox->SetContent(CompassOverlay);

	if (BackgroundColor.A > 0.0f)
	{
		BackgroundBorder = SFCompassHUD::MakeSolidBorder(
			*Tree, TEXT("CompassBackground"), BackgroundColor,
			FVector2D(StripPixelWidth, StripPixelHeight));
		if (UOverlaySlot* BgSlot = CompassOverlay->AddChildToOverlay(BackgroundBorder))
		{
			BgSlot->SetHorizontalAlignment(HAlign_Fill);
			BgSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	StripCanvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CompassStripCanvas"));
	StripCanvas->SetClipping(EWidgetClipping::ClipToBounds);
	if (UOverlaySlot* StripSlot = CompassOverlay->AddChildToOverlay(StripCanvas))
	{
		StripSlot->SetHorizontalAlignment(HAlign_Fill);
		StripSlot->SetVerticalAlignment(VAlign_Fill);
	}

	CenterPipBorder = SFCompassHUD::MakeSolidBorder(
		*Tree, TEXT("CompassCenterPip"), CenterPipColor, FVector2D(4.0f, StripPixelHeight));
	if (UOverlaySlot* PipSlot = CompassOverlay->AddChildToOverlay(CenterPipBorder))
	{
		PipSlot->SetHorizontalAlignment(HAlign_Center);
		PipSlot->SetVerticalAlignment(VAlign_Fill);
	}

	bVisualTreeBuilt = true;
}

void USFCompassHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BuildCardinalTickLabels();
	SpawnCardinalLabelWidgets();
	TryResolveSubsystem();
}

void USFCompassHUDWidget::NativeDestruct()
{
	StopAutoResolveRetryTimer();

	if (USFNarrativeWaypointSubsystem* Sub = Subsystem.Get())
	{
		Sub->OnWaypointsChanged.RemoveDynamic(this, &USFCompassHUDWidget::HandleWaypointsChanged);
		Sub->OnTrackedWaypointChanged.RemoveDynamic(this, &USFCompassHUDWidget::HandleTrackedWaypointChanged);
	}
	Subsystem = nullptr;

	Super::NativeDestruct();
}

void USFCompassHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshPlayerYaw();
	RecomputeStripCoordinates();
	UpdateChildPositions();
	BP_OnCompassTick(InDeltaTime);
}

void USFCompassHUDWidget::BuildCardinalTickLabels()
{
	CardinalTicks.Reset(SFCompassHUD::NumCardinalLabels);
	for (int32 Index = 0; Index < SFCompassHUD::NumCardinalLabels; ++Index)
	{
		FSFCompassCardinalTick Tick;
		Tick.Label = SFCompassHUD::CardinalLabels[Index];
		Tick.YawDegrees = static_cast<float>(Index) * SFCompassHUD::CardinalStepDegrees;
		CardinalTicks.Add(Tick);
	}
}

void USFCompassHUDWidget::SpawnCardinalLabelWidgets()
{
	if (!StripCanvas || !WidgetTree)
	{
		return;
	}

	for (auto& Pair : CardinalLabelOverlays)
	{
		if (UOverlay* Old = Pair.Value)
		{
			Old->RemoveFromParent();
		}
	}
	CardinalLabelOverlays.Reset();

	const bool bHasTickBrush = SFCompassHUD::BrushHasResource(TickBrush);

	for (const FSFCompassCardinalTick& Tick : CardinalTicks)
	{
		const FString WidgetName = FString::Printf(TEXT("CompassLabel_%s"), *Tick.Label);
		UOverlay* LabelOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), FName(*WidgetName));

		// Tick: UImage if designer set a brush; otherwise UBorder solid rect.
		UWidget* TickVisual = nullptr;
		if (bHasTickBrush)
		{
			UImage* Img = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), FName(*(WidgetName + TEXT("_Tick"))));
			Img->SetBrush(TickBrush);
			TickVisual = Img;
		}
		else
		{
			TickVisual = SFCompassHUD::MakeSolidBorder(
				*WidgetTree, FName(*(WidgetName + TEXT("_Tick"))),
				FLinearColor(1.0f, 1.0f, 1.0f, 0.55f),
				FVector2D(2.0f, 12.0f));
		}
		if (UOverlaySlot* TickSlot = LabelOverlay->AddChildToOverlay(TickVisual))
		{
			TickSlot->SetHorizontalAlignment(HAlign_Center);
			TickSlot->SetVerticalAlignment(VAlign_Bottom);
		}

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*(WidgetName + TEXT("_Text"))));
		Label->SetText(FText::FromString(Tick.Label));
		FSlateFontInfo Font = Label->GetFont();
		Font.Size = 12;
		Font.TypefaceFontName = FName(TEXT("Bold"));
		Label->SetFont(Font);
		Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Label->SetJustification(ETextJustify::Center);
		if (UOverlaySlot* TextSlot = LabelOverlay->AddChildToOverlay(Label))
		{
			TextSlot->SetHorizontalAlignment(HAlign_Center);
			TextSlot->SetVerticalAlignment(VAlign_Top);
			TextSlot->SetPadding(FMargin(0.0f, CardinalLabelTopPadding, 0.0f, 0.0f));
		}

		if (UCanvasPanelSlot* CanvasSlot = StripCanvas->AddChildToCanvas(LabelOverlay))
		{
			CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetPosition(FVector2D(0.0f, 0.0f));
		}

		CardinalLabelOverlays.Add(Tick.Label, LabelOverlay);
	}
}

void USFCompassHUDWidget::TryResolveSubsystem()
{
	if (Subsystem.IsValid())
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		StartAutoResolveRetryTimer();
		return;
	}
	USFNarrativeWaypointSubsystem* Sub = World->GetSubsystem<USFNarrativeWaypointSubsystem>();
	if (!Sub)
	{
		StartAutoResolveRetryTimer();
		return;
	}

	Subsystem = Sub;
	Sub->OnWaypointsChanged.AddDynamic(this, &USFCompassHUDWidget::HandleWaypointsChanged);
	Sub->OnTrackedWaypointChanged.AddDynamic(this, &USFCompassHUDWidget::HandleTrackedWaypointChanged);

	HandleWaypointsChanged();
	FSFWaypointSnapshot Tracked;
	if (Sub->GetTrackedWaypoint(Tracked))
	{
		HandleTrackedWaypointChanged(Tracked);
	}

	StopAutoResolveRetryTimer();
}

void USFCompassHUDWidget::StartAutoResolveRetryTimer()
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
		&USFCompassHUDWidget::TickAutoResolveRetry,
		Period,
		/*bLoop=*/true);
}

void USFCompassHUDWidget::StopAutoResolveRetryTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoResolveRetryHandle);
	}
	AutoResolveRetryElapsedSeconds = 0.0f;
}

void USFCompassHUDWidget::TickAutoResolveRetry()
{
	if (Subsystem.IsValid())
	{
		StopAutoResolveRetryTimer();
		return;
	}
	TryResolveSubsystem();
	if (Subsystem.IsValid())
	{
		StopAutoResolveRetryTimer();
		return;
	}

	AutoResolveRetryElapsedSeconds += FMath::Max(0.1f, AutoResolveRetryPeriodSeconds);
	if (AutoResolveRetryElapsedSeconds >= MaxAutoResolveRetrySeconds)
	{
		UE_LOG(LogSFCompassHUD, Warning,
			TEXT("[CompassHUD] Gave up resolving USFNarrativeWaypointSubsystem after %.1fs."),
			AutoResolveRetryElapsedSeconds);
		StopAutoResolveRetryTimer();
	}
}

void USFCompassHUDWidget::HandleWaypointsChanged()
{
	USFNarrativeWaypointSubsystem* Sub = Subsystem.Get();
	if (!Sub)
	{
		return;
	}
	CachedActiveWaypoints = Sub->GetActiveWaypoints();

	CompassMarkers.Reset(CachedActiveWaypoints.Num());
	for (const FSFWaypointSnapshot& Snapshot : CachedActiveWaypoints)
	{
		if (!Snapshot.IsValidSnapshot() || !Snapshot.bRevealOnCompass)
		{
			continue;
		}
		FSFCompassMarker Marker;
		Marker.Waypoint = Snapshot;
		Marker.bIsTracked = Snapshot.bIsTracked;
		CompassMarkers.Add(Marker);
	}

	RebuildMarkerWidgets();
	ApplyMarkerVisualState();
	BP_OnCompassMarkersChanged(CompassMarkers);
}

void USFCompassHUDWidget::HandleTrackedWaypointChanged(const FSFWaypointSnapshot& InTrackedWaypoint)
{
	CurrentTrackedWaypoint = InTrackedWaypoint;
	bHasTrackedWaypoint = InTrackedWaypoint.IsValidSnapshot();
	ApplyMarkerVisualState();
	BP_OnTrackedWaypointChanged(CurrentTrackedWaypoint, bHasTrackedWaypoint);
}

void USFCompassHUDWidget::RebuildMarkerWidgets()
{
	if (!StripCanvas || !WidgetTree)
	{
		return;
	}

	TSet<FString> WantedKeys;
	WantedKeys.Reserve(CompassMarkers.Num());
	for (const FSFCompassMarker& Marker : CompassMarkers)
	{
		WantedKeys.Add(MakeMarkerKey(Marker.Waypoint));
	}

	TArray<FString> StaleKeys;
	for (const TPair<FString, FMarkerWidgetSet>& Pair : MarkerWidgets)
	{
		if (!WantedKeys.Contains(Pair.Key))
		{
			StaleKeys.Add(Pair.Key);
		}
	}
	for (const FString& Key : StaleKeys)
	{
		if (UOverlay* Root = MarkerWidgets[Key].Root.Get())
		{
			Root->RemoveFromParent();
		}
		MarkerWidgets.Remove(Key);
	}

	for (const FSFCompassMarker& Marker : CompassMarkers)
	{
		const FString Key = MakeMarkerKey(Marker.Waypoint);
		if (MarkerWidgets.Contains(Key))
		{
			continue;
		}

		const FString BaseName = FString::Printf(TEXT("Marker_%s_%s"),
			*Marker.Waypoint.QuestId.ToString(),
			*Marker.Waypoint.TaskId.ToString());

		UOverlay* MarkerRoot = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), FName(*BaseName));

		FMarkerWidgetSet Set;
		Set.Root = MarkerRoot;

		// Behind chevron.
		const bool bHasBehindBrush = SFCompassHUD::BrushHasResource(BehindArrowBrush);
		if (bHasBehindBrush)
		{
			UImage* Img = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), FName(*(BaseName + TEXT("_Behind"))));
			Img->SetBrush(BehindArrowBrush);
			Img->SetVisibility(ESlateVisibility::Collapsed);
			Set.BehindArrow = Img;
			Set.bBehindIsBorder = false;
			if (UOverlaySlot* Slot = MarkerRoot->AddChildToOverlay(Img))
			{
				Slot->SetHorizontalAlignment(HAlign_Center);
				Slot->SetVerticalAlignment(VAlign_Center);
			}
		}
		else
		{
			UBorder* B = SFCompassHUD::MakeSolidBorder(
				*WidgetTree, FName(*(BaseName + TEXT("_Behind"))),
				FLinearColor(1.0f, 1.0f, 1.0f, 0.9f),
				FVector2D(MarkerPixelSize * 0.5f, MarkerPixelSize * 0.5f));
			B->SetVisibility(ESlateVisibility::Collapsed);
			Set.BehindArrow = B;
			Set.bBehindIsBorder = true;
			if (UOverlaySlot* Slot = MarkerRoot->AddChildToOverlay(B))
			{
				Slot->SetHorizontalAlignment(HAlign_Center);
				Slot->SetVerticalAlignment(VAlign_Center);
			}
		}

		// Tracked ring.
		const bool bHasRingBrush = SFCompassHUD::BrushHasResource(TrackedRingBrush);
		if (bHasRingBrush)
		{
			UImage* Img = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), FName(*(BaseName + TEXT("_Ring"))));
			Img->SetBrush(TrackedRingBrush);
			Img->SetVisibility(ESlateVisibility::Collapsed);
			Set.Ring = Img;
			Set.bRingIsBorder = false;
			if (UOverlaySlot* Slot = MarkerRoot->AddChildToOverlay(Img))
			{
				Slot->SetHorizontalAlignment(HAlign_Center);
				Slot->SetVerticalAlignment(VAlign_Center);
			}
		}
		else
		{
			// Fallback ring: a slightly larger tinted border behind the icon.
			UBorder* B = SFCompassHUD::MakeSolidBorder(
				*WidgetTree, FName(*(BaseName + TEXT("_Ring"))),
				FLinearColor(TrackedMarkerTint.R, TrackedMarkerTint.G, TrackedMarkerTint.B, 0.35f),
				FVector2D(MarkerPixelSize * 1.4f, MarkerPixelSize * 1.4f));
			B->SetVisibility(ESlateVisibility::Collapsed);
			Set.Ring = B;
			Set.bRingIsBorder = true;
			if (UOverlaySlot* Slot = MarkerRoot->AddChildToOverlay(B))
			{
				Slot->SetHorizontalAlignment(HAlign_Center);
				Slot->SetVerticalAlignment(VAlign_Center);
			}
		}

		// Icon.
		FSlateBrush IconBrush;
		FLinearColor IconTint;
		ResolveMarkerStyle(Marker.Waypoint, IconBrush, IconTint);
		if (SFCompassHUD::BrushHasResource(IconBrush))
		{
			UImage* Img = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), FName(*(BaseName + TEXT("_Icon"))));
			Img->SetBrush(IconBrush);
			Set.Icon = Img;
			Set.bIconIsBorder = false;
			if (UOverlaySlot* Slot = MarkerRoot->AddChildToOverlay(Img))
			{
				Slot->SetHorizontalAlignment(HAlign_Center);
				Slot->SetVerticalAlignment(VAlign_Center);
			}
		}
		else
		{
			UBorder* B = SFCompassHUD::MakeSolidBorder(
				*WidgetTree, FName(*(BaseName + TEXT("_Icon"))),
				IconTint, FVector2D(MarkerPixelSize, MarkerPixelSize));
			Set.Icon = B;
			Set.bIconIsBorder = true;
			if (UOverlaySlot* Slot = MarkerRoot->AddChildToOverlay(B))
			{
				Slot->SetHorizontalAlignment(HAlign_Center);
				Slot->SetVerticalAlignment(VAlign_Center);
			}
		}

		// Distance text.
		if (bShowMarkerDistance && DistanceFontSize > 0.0f)
		{
			UTextBlock* Distance = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*(BaseName + TEXT("_Distance"))));
			FSlateFontInfo Font = Distance->GetFont();
			Font.Size = static_cast<int32>(DistanceFontSize);
			Distance->SetFont(Font);
			Distance->SetJustification(ETextJustify::Center);
			Distance->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.85f)));
			Distance->SetText(FText::GetEmpty());
			Set.DistanceText = Distance;
			if (UOverlaySlot* Slot = MarkerRoot->AddChildToOverlay(Distance))
			{
				Slot->SetHorizontalAlignment(HAlign_Center);
				Slot->SetVerticalAlignment(VAlign_Bottom);
				Slot->SetPadding(FMargin(0.0f, MarkerPixelSize + 2.0f, 0.0f, 0.0f));
			}
		}

		if (UCanvasPanelSlot* CanvasSlot = StripCanvas->AddChildToCanvas(MarkerRoot))
		{
			CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetPosition(FVector2D(0.0f, 0.0f));
		}

		MarkerWidgets.Add(Key, Set);
	}
}

void USFCompassHUDWidget::ApplyMarkerVisualState()
{
	for (const FSFCompassMarker& Marker : CompassMarkers)
	{
		const FString Key = MakeMarkerKey(Marker.Waypoint);
		const FMarkerWidgetSet* Found = MarkerWidgets.Find(Key);
		if (!Found)
		{
			continue;
		}

		// Ring visibility.
		if (UWidget* Ring = Found->Ring.Get())
		{
			Ring->SetVisibility(Marker.bIsTracked ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}

		// Icon tint.
		FSlateBrush IconBrush;
		FLinearColor IconTint;
		ResolveMarkerStyle(Marker.Waypoint, IconBrush, IconTint);
		const FLinearColor FinalTint = Marker.bIsTracked ? TrackedMarkerTint : IconTint;

		if (UWidget* IconW = Found->Icon.Get())
		{
			if (Found->bIconIsBorder)
			{
				if (UBorder* B = Cast<UBorder>(IconW))
				{
					B->SetBrushColor(FinalTint);
				}
			}
			else
			{
				if (UImage* Img = Cast<UImage>(IconW))
				{
					IconBrush.TintColor = FSlateColor(FinalTint);
					Img->SetBrush(IconBrush);
				}
			}
		}
	}
}

void USFCompassHUDWidget::RefreshPlayerYaw()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	float NewYaw = CachedPlayerYaw;
	if (bUseCameraRotation && PC->PlayerCameraManager)
	{
		NewYaw = PC->PlayerCameraManager->GetCameraRotation().Yaw;
	}
	else
	{
		NewYaw = PC->GetControlRotation().Yaw;
	}
	CachedPlayerYaw = WrapYawPositive(NewYaw);
}

void USFCompassHUDWidget::RecomputeStripCoordinates()
{
	for (FSFCompassCardinalTick& Tick : CardinalTicks)
	{
		const float Signed = WrapSignedDegrees(Tick.YawDegrees - CachedPlayerYaw);
		Tick.AngularOffsetDegrees = Signed;
		Tick.StripCoordinate = AngleToStripCoordinate(Signed);
	}

	APlayerController* PC = GetOwningPlayer();
	FVector PlayerLoc = FVector::ZeroVector;
	bool bHasPlayerLoc = false;
	if (PC)
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			PlayerLoc = Pawn->GetActorLocation();
			bHasPlayerLoc = true;
		}
		else if (PC->PlayerCameraManager)
		{
			PlayerLoc = PC->PlayerCameraManager->GetCameraLocation();
			bHasPlayerLoc = true;
		}
	}

	for (FSFCompassMarker& Marker : CompassMarkers)
	{
		float Bearing = 0.0f;
		if (bHasPlayerLoc)
		{
			const FVector Delta = Marker.Waypoint.WorldLocation - PlayerLoc;
			Marker.DistanceMeters = static_cast<float>(Delta.Size2D() * 0.01);
			const float BearingYaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
			Bearing = WrapSignedDegrees(BearingYaw - CachedPlayerYaw);
		}
		Marker.AngularOffsetDegrees = Bearing;
		Marker.bIsBehind = FMath::Abs(Bearing) > 90.0f;
		Marker.StripCoordinate = AngleToStripCoordinate(Bearing);
	}
}

void USFCompassHUDWidget::UpdateChildPositions()
{
	if (!StripCanvas)
	{
		return;
	}

	const float MaxAbs = FMath::Max(0.1f, MaxStripCoordinateAbs);

	for (const FSFCompassCardinalTick& Tick : CardinalTicks)
	{
		const TObjectPtr<UOverlay>* FoundPtr = CardinalLabelOverlays.Find(Tick.Label);
		if (!FoundPtr || !*FoundPtr)
		{
			continue;
		}
		UOverlay* LabelOverlay = *FoundPtr;
		UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(LabelOverlay->Slot);
		if (!Slot)
		{
			continue;
		}
		const float PixelX = StripCoordinateToPixelX(Tick.StripCoordinate);
		Slot->SetPosition(FVector2D(PixelX, 0.0f));

		const bool bVisible = FMath::Abs(Tick.StripCoordinate) <= MaxAbs;
		LabelOverlay->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	for (const FSFCompassMarker& Marker : CompassMarkers)
	{
		const FString Key = MakeMarkerKey(Marker.Waypoint);
		const FMarkerWidgetSet* Found = MarkerWidgets.Find(Key);
		if (!Found)
		{
			continue;
		}
		UOverlay* MarkerRoot = Found->Root.Get();
		if (!MarkerRoot)
		{
			continue;
		}

		float DisplayStrip = Marker.StripCoordinate;
		bool bVisible = true;
		bool bShowBehind = false;

		if (Marker.bIsBehind || FMath::Abs(Marker.StripCoordinate) > MaxAbs)
		{
			if (bPinBehindMarkersToEdge)
			{
				const float Sign = (Marker.AngularOffsetDegrees < 0.0f) ? -1.0f : 1.0f;
				DisplayStrip = Sign * MaxAbs;
				bShowBehind = true;
			}
			else
			{
				bVisible = false;
			}
		}

		MarkerRoot->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (!bVisible)
		{
			continue;
		}

		if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(MarkerRoot->Slot))
		{
			const float PixelX = StripCoordinateToPixelX(DisplayStrip);
			Slot->SetPosition(FVector2D(PixelX, 0.0f));
		}

		if (UWidget* Behind = Found->BehindArrow.Get())
		{
			Behind->SetVisibility(bShowBehind ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}

		if (UWidget* Icon = Found->Icon.Get())
		{
			Icon->SetVisibility(bShowBehind ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		}

		if (UTextBlock* DistanceText = Found->DistanceText.Get())
		{
			if (bShowMarkerDistance && !bShowBehind)
			{
				DistanceText->SetVisibility(ESlateVisibility::HitTestInvisible);
				const int32 Meters = FMath::RoundToInt(Marker.DistanceMeters);
				DistanceText->SetText(FText::FromString(FString::Printf(TEXT("%dm"), Meters)));
			}
			else
			{
				DistanceText->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}

float USFCompassHUDWidget::AngleToStripCoordinate(float SignedAngleDegrees) const
{
	const float SafeHalfFov = FMath::Max(1.0f, CompassHalfFovDegrees);
	return SignedAngleDegrees / SafeHalfFov;
}

float USFCompassHUDWidget::StripCoordinateToPixelX(float StripCoordinate) const
{
	return StripCoordinate * (StripPixelWidth * 0.5f);
}

void USFCompassHUDWidget::ResolveMarkerStyle(const FSFWaypointSnapshot& Snapshot, FSlateBrush& OutBrush, FLinearColor& OutTint) const
{
	OutBrush = FSlateBrush();
	OutTint = UntrackedMarkerTint;

	if (Snapshot.IconTag.IsValid())
	{
		if (const FSFCompassMarkerStyle* Override = MarkerStylesByTag.Find(Snapshot.IconTag))
		{
			OutTint = Override->IconTint;
			if (SFCompassHUD::BrushHasResource(Override->IconBrush))
			{
				OutBrush = Override->IconBrush;
				OutBrush.ImageSize = FVector2D(MarkerPixelSize, MarkerPixelSize);
				OutBrush.TintColor = FSlateColor(OutTint);
			}
			return;
		}
	}

	if (SFCompassHUD::BrushHasResource(DefaultMarkerBrush))
	{
		OutBrush = DefaultMarkerBrush;
		OutBrush.ImageSize = FVector2D(MarkerPixelSize, MarkerPixelSize);
		OutBrush.TintColor = FSlateColor(OutTint);
	}
	// else OutBrush stays empty -> caller falls back to UBorder solid fill with OutTint.
}

bool USFCompassHUDWidget::GetStripCoordinateForWorldLocation(
	FVector WorldLocation,
	float& OutStripCoordinate,
	float& OutAngularOffsetDegrees,
	bool& OutbIsBehind) const
{
	OutStripCoordinate = 0.0f;
	OutAngularOffsetDegrees = 0.0f;
	OutbIsBehind = false;

	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return false;
	}

	FVector PlayerLoc = FVector::ZeroVector;
	bool bHasPlayerLoc = false;
	if (APawn* Pawn = PC->GetPawn())
	{
		PlayerLoc = Pawn->GetActorLocation();
		bHasPlayerLoc = true;
	}
	else if (PC->PlayerCameraManager)
	{
		PlayerLoc = PC->PlayerCameraManager->GetCameraLocation();
		bHasPlayerLoc = true;
	}
	if (!bHasPlayerLoc)
	{
		return false;
	}

	const FVector Delta = WorldLocation - PlayerLoc;
	const float BearingYaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
	const float Signed = WrapSignedDegrees(BearingYaw - CachedPlayerYaw);
	OutAngularOffsetDegrees = Signed;
	OutbIsBehind = FMath::Abs(Signed) > 90.0f;
	OutStripCoordinate = AngleToStripCoordinate(Signed);
	return true;
}

bool USFCompassHUDWidget::GetStripCoordinateForYaw(
	float TargetYawDegrees,
	float& OutStripCoordinate,
	float& OutAngularOffsetDegrees) const
{
	const float Signed = WrapSignedDegrees(TargetYawDegrees - CachedPlayerYaw);
	OutAngularOffsetDegrees = Signed;
	OutStripCoordinate = AngleToStripCoordinate(Signed);
	return true;
}

float USFCompassHUDWidget::WrapYawPositive(float Yaw)
{
	float Wrapped = FMath::Fmod(Yaw, 360.0f);
	if (Wrapped < 0.0f)
	{
		Wrapped += 360.0f;
	}
	return Wrapped;
}

float USFCompassHUDWidget::WrapSignedDegrees(float Angle)
{
	float Wrapped = FMath::Fmod(Angle + 180.0f, 360.0f);
	if (Wrapped < 0.0f)
	{
		Wrapped += 360.0f;
	}
	return Wrapped - 180.0f;
}

FString USFCompassHUDWidget::MakeMarkerKey(const FSFWaypointSnapshot& Snapshot)
{
	return FString::Printf(TEXT("%s|%s"), *Snapshot.QuestId.ToString(), *Snapshot.TaskId.ToString());
}
