#include "UI/SFEnemyHealthBarWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Characters/SFCharacterBase.h"
#include "Characters/SFNPCBase.h"
#include "Components/ProgressBar.h"
#include "Components/SFProgressionComponent.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "Core/SFAttributeSetBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogSFEnemyHealthBar, Log, All);

namespace
{
	/** Safe percent: returns 0 if Max <= 0, otherwise Current/Max clamped to [0,1]. */
	float SafePct(float Current, float Max)
	{
		if (Max <= KINDA_SMALL_NUMBER)
		{
			return 0.0f;
		}
		return FMath::Clamp(Current / Max, 0.0f, 1.0f);
	}
}

void USFEnemyHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Hide until first damage. The owning UWidgetComponent stays alive; we just
	// hide the inner content so the world-space slot doesn't render a ghost bar.
	SetVisibility(ESlateVisibility::Hidden);

	// If the widget is hosted inside a UWidgetComponent (the typical case for
	// world-space NPC bars), the component's owner is the pawn we want to track.
	// GetTypedOuter walks the outer chain so it works whether UE puts the widget
	// directly under the component or under a transient package in between.
	UWidgetComponent* OwningComp = GetTypedOuter<UWidgetComponent>();
	ASFCharacterBase* Owner = OwningComp ? Cast<ASFCharacterBase>(OwningComp->GetOwner()) : nullptr;

	UE_LOG(LogSFEnemyHealthBar, Log,
		TEXT("NativeConstruct widget=%s OwningComp=%s OwnerActor=%s OwnerAsCharacter=%s"),
		*GetNameSafe(this),
		*GetNameSafe(OwningComp),
		OwningComp ? *GetNameSafe(OwningComp->GetOwner()) : TEXT("<null>"),
		*GetNameSafe(Owner));

	if (Owner)
	{
		InitializeForCharacter(Owner);
	}
}

void USFEnemyHealthBarWidget::NativeDestruct()
{
	UnbindFromCharacterDelegates();
	Super::NativeDestruct();
}

void USFEnemyHealthBarWidget::NativeTick(const FGeometry& MyGeometry, float DeltaTime)
{
	Super::NativeTick(MyGeometry, DeltaTime);

	// Smooth fill so a one-shot burst "bleeds down" instead of snapping. Destiny-y.
	DisplayedHealthPct = FMath::FInterpTo(DisplayedHealthPct, TargetHealthPct, DeltaTime, BarInterpSpeed);
	DisplayedShieldsPct = FMath::FInterpTo(DisplayedShieldsPct, TargetShieldsPct, DeltaTime, BarInterpSpeed);

	if (HealthBar)
	{
		HealthBar->SetPercent(DisplayedHealthPct);
	}
	if (ShieldsBar)
	{
		ShieldsBar->SetPercent(DisplayedShieldsPct);
		// Hide the shield bar entirely when the NPC has no shield attribute set up,
		// so unshielded mooks just show a single red health bar (Destiny grunt feel).
		const bool bHasShields = TargetShieldsPct > KINDA_SMALL_NUMBER || DisplayedShieldsPct > KINDA_SMALL_NUMBER;
		ShieldsBar->SetVisibility(bHasShields ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	// Auto-hide after idle period \u2014 only while bar is visible.
	if (bBarVisible)
	{
		SecondsSinceLastDamage += DeltaTime;
		if (SecondsSinceLastDamage >= HideAfterSecondsIdle)
		{
			RequestHide();
		}
	}
}

void USFEnemyHealthBarWidget::InitializeForCharacter(ASFCharacterBase* InTarget)
{
	if (TargetCharacter.Get() == InTarget)
	{
		return;
	}

	UnbindFromCharacterDelegates();
	TargetCharacter = InTarget;

	if (!InTarget)
	{
		return;
	}

	BindToCharacterDelegates();
	RefreshIdentityText();

	// Prime displayed values from the attribute set so when the bar first appears
	// after damage, it already reflects the NPC's pre-hit state.
	if (const USFAttributeSetBase* Attrs = InTarget->GetAttributeSet())
	{
		TargetHealthPct = SafePct(Attrs->GetHealth(), Attrs->GetMaxHealth());
		TargetShieldsPct = SafePct(Attrs->GetShields(), Attrs->GetMaxShields());
		DisplayedHealthPct = TargetHealthPct;
		DisplayedShieldsPct = TargetShieldsPct;
	}
}

void USFEnemyHealthBarWidget::ShowBarTransient()
{
	RequestShow();
}

void USFEnemyHealthBarWidget::BindToCharacterDelegates()
{
	ASFCharacterBase* Target = TargetCharacter.Get();
	if (!Target)
	{
		return;
	}

	Target->OnHealthChanged.AddDynamic(this, &USFEnemyHealthBarWidget::HandleHealthChanged);
	Target->OnShieldsChanged.AddDynamic(this, &USFEnemyHealthBarWidget::HandleShieldsChanged);
	Target->OnCharacterDied.AddDynamic(this, &USFEnemyHealthBarWidget::HandleCharacterDied);

	// ASFCharacterBase::BeginPlay calls InitWidget on the floating-bar component
	// BEFORE BroadcastInitialAttributeValues, so we are guaranteed to receive the
	// priming broadcast on possession. Swallow that first event so the bar doesn't
	// pop visible the instant the character spawns; every subsequent broadcast is
	// a real damage / heal event and will trigger the fade-in.
	bHasReceivedInitialBroadcast = false;
}

void USFEnemyHealthBarWidget::UnbindFromCharacterDelegates()
{
	ASFCharacterBase* Target = TargetCharacter.Get();
	if (!Target)
	{
		return;
	}

	Target->OnHealthChanged.RemoveDynamic(this, &USFEnemyHealthBarWidget::HandleHealthChanged);
	Target->OnShieldsChanged.RemoveDynamic(this, &USFEnemyHealthBarWidget::HandleShieldsChanged);
	Target->OnCharacterDied.RemoveDynamic(this, &USFEnemyHealthBarWidget::HandleCharacterDied);
}

void USFEnemyHealthBarWidget::RefreshIdentityText()
{
	ASFCharacterBase* Target = TargetCharacter.Get();
	if (!Target)
	{
		return;
	}

	if (NameText)
	{
		// Prefer the NPC's authored display name when available; fall back to actor label
		// (cooked label is class-display-name) so we never show an empty plate.
		FText DisplayName;
		if (const ASFNPCBase* NPC = Cast<ASFNPCBase>(Target))
		{
			DisplayName = NPC->GetNPCName();
		}
		if (DisplayName.IsEmpty())
		{
			DisplayName = FText::FromString(Target->GetName());
		}
		NameText->SetText(DisplayName);
	}

	if (LevelText)
	{
		int32 Level = 1;
		if (const USFProgressionComponent* Progression = Target->FindComponentByClass<USFProgressionComponent>())
		{
			Level = Progression->GetCurrentLevel();
		}
		LevelText->SetText(FText::Format(NSLOCTEXT("SFEnemyHUD", "LevelFmt", "Lv {0}"), Level));
	}
}

void USFEnemyHealthBarWidget::HandleHealthChanged(float NewValue, float MaxValue)
{
	TargetHealthPct = SafePct(NewValue, MaxValue);

	// The first broadcast can come from BindAttributeDelegates priming UI listeners on
	// possession \u2014 not a real damage event. Swallow that one so we don't pop the
	// bar visible the instant the NPC spawns.
	if (!bHasReceivedInitialBroadcast)
	{
		bHasReceivedInitialBroadcast = true;
		DisplayedHealthPct = TargetHealthPct;
		UE_LOG(LogSFEnemyHealthBar, Log,
			TEXT("HandleHealthChanged PRIMING-SWALLOW widget=%s NewValue=%.2f Max=%.2f"),
			*GetNameSafe(this), NewValue, MaxValue);
		return;
	}

	UE_LOG(LogSFEnemyHealthBar, Log,
		TEXT("HandleHealthChanged widget=%s NewValue=%.2f Max=%.2f -> RequestShow"),
		*GetNameSafe(this), NewValue, MaxValue);
	RequestShow();
}

void USFEnemyHealthBarWidget::HandleShieldsChanged(float NewValue, float MaxValue)
{
	TargetShieldsPct = SafePct(NewValue, MaxValue);

	// Same initial-broadcast guard as health — flip the bit on either path so
	// the first real event after priming (whether it hits Health or Shields)
	// correctly fades the bar in.
	if (!bHasReceivedInitialBroadcast)
	{
		bHasReceivedInitialBroadcast = true;
		DisplayedShieldsPct = TargetShieldsPct;
		return;
	}

	RequestShow();
}

void USFEnemyHealthBarWidget::HandleCharacterDied(ASFCharacterBase* /*Victim*/, ASFCharacterBase* /*Killer*/)
{
	// Force the bars to empty so a partial-fill bar doesn't get frozen mid-fade.
	TargetHealthPct = 0.0f;
	TargetShieldsPct = 0.0f;
	DisplayedHealthPct = 0.0f;
	DisplayedShieldsPct = 0.0f;
	bBarVisible = false;

	SetVisibility(ESlateVisibility::Hidden);

	UnbindFromCharacterDelegates();
	TargetCharacter.Reset();
}

void USFEnemyHealthBarWidget::RequestShow()
{
	SecondsSinceLastDamage = 0.0f;

	if (bBarVisible)
	{
		return;
	}
	bBarVisible = true;

	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (FadeInAnim)
	{
		PlayAnimation(FadeInAnim);
	}
}

void USFEnemyHealthBarWidget::RequestHide()
{
	if (!bBarVisible)
	{
		return;
	}
	bBarVisible = false;

	if (FadeOutAnim)
	{
		PlayAnimation(FadeOutAnim);
		// Hand off to the WBP \u2014 author can SetVisibility(Hidden) at end of FadeOutAnim.
		// If the WBP doesn't author that, we hide here so the bar truly disappears.
	}
	SetVisibility(ESlateVisibility::Hidden);
}
