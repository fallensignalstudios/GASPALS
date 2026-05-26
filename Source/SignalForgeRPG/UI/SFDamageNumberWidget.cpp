// Copyright Fallen Signal Studios. All Rights Reserved.

#include "UI/SFDamageNumberWidget.h"

#include "Components/CanvasPanelSlot.h"

void USFDamageNumberWidget::InitDamageNumber(float InDamage, bool bInIsCrit, bool bInIsWeakpoint, FVector2D InScreenPos)
{
	Damage = InDamage;
	bIsCrit = bInIsCrit;
	bIsWeakpoint = bInIsWeakpoint;
	ScreenPos = InScreenPos;

	// Pick a random horizontal drift per-instance so stacked hits don't overlap.
	SideDrift = FMath::FRandRange(-SideJitter, SideJitter);
	Elapsed = 0.0f;
	bInitialized = true;

	// Apply the initial scale punch + zero translation so the first frame is
	// already in the popped state, even before NativeTick runs.
	const float StartScale = bIsCrit ? CritScalePunch : ScalePunch;
	SetRenderScale(FVector2D(StartScale, StartScale));
	SetRenderTranslation(FVector2D::ZeroVector);
	SetRenderOpacity(1.0f);

	// If the widget was placed in a CanvasPanel, snap it to the requested
	// screen position. Subsystem also sets this on the slot directly, but
	// doing it here makes the widget self-sufficient when spawned standalone.
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		CanvasSlot->SetPosition(ScreenPos);
	}

	OnDamageNumberInitialized();
}

FLinearColor USFDamageNumberWidget::GetResolvedColor() const
{
	if (bIsWeakpoint)
	{
		return WeakpointColor;
	}
	if (bIsCrit)
	{
		return CritColor;
	}
	return NormalColor;
}

FText USFDamageNumberWidget::GetTagLabel() const
{
	// Destiny-style single-tag display: a weakpoint hit already auto-crits
	// inside the damage execution, so semantically every weakpoint IS a crit.
	// We surface that as the CRIT chip by default — designers who want the
	// distinct "WEAK" wording back can edit WeakpointTagLabel on the widget
	// CDO. Falls through to the explicit weakpoint label if it has been
	// overridden to something non-empty AND non-equal to the crit label, so
	// the field still has authoring meaning.
	if (bIsWeakpoint)
	{
		return WeakpointTagLabel.IsEmptyOrWhitespace() ? CritTagLabel : WeakpointTagLabel;
	}
	if (bIsCrit)
	{
		return CritTagLabel;
	}
	return FText::GetEmpty();
}

void USFDamageNumberWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bInitialized || Lifespan <= 0.0f)
	{
		return;
	}

	Elapsed += InDeltaTime;
	const float Alpha = FMath::Clamp(Elapsed / Lifespan, 0.0f, 1.0f);

	// Rise + drift. Ease-out so the floater shoots up fast then settles.
	const float RiseAlpha = 1.0f - FMath::Pow(1.0f - Alpha, 2.0f);
	const float DriftAlpha = Alpha; // linear sideways drift feels natural
	SetRenderTranslation(FVector2D(SideDrift * DriftAlpha, -RiseDistance * RiseAlpha));

	// Scale punch ease-back to 1.0 within ScalePunchTime, then hold.
	const float StartScale = bIsCrit ? CritScalePunch : ScalePunch;
	float Scale = 1.0f;
	if (ScalePunchTime > 0.0f && Elapsed < ScalePunchTime)
	{
		const float ScaleAlpha = Elapsed / ScalePunchTime;
		Scale = FMath::Lerp(StartScale, 1.0f, ScaleAlpha);
	}
	SetRenderScale(FVector2D(Scale, Scale));

	// Fade out across the last FadeOutFraction of Lifespan.
	const float FadeStart = FMath::Clamp(1.0f - FadeOutFraction, 0.0f, 1.0f);
	if (Alpha >= FadeStart && FadeOutFraction > 0.0f)
	{
		const float FadeAlpha = (Alpha - FadeStart) / FadeOutFraction;
		SetRenderOpacity(FMath::Clamp(1.0f - FadeAlpha, 0.0f, 1.0f));
	}
	else
	{
		SetRenderOpacity(1.0f);
	}

	if (Elapsed >= Lifespan)
	{
		bInitialized = false;
		RemoveFromParent();
	}
}
