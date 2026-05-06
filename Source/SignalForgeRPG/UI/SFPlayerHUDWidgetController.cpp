#include "UI/SFPlayerHUDWidgetController.h"

#include "Characters/SFCharacterBase.h"
#include "Characters/SFPlayerCharacter.h"
#include "Core/SFAttributeSetBase.h"
#include "UI/SFAttributeBarWidget.h"
#include "Components/SFInteractionComponent.h"
#include "Components/SFInteractableInterface.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/SFEquipmentComponent.h"



void USFPlayerHUDWidgetController::Initialize(ASFCharacterBase* InPlayerCharacter)
{
	PlayerCharacter = InPlayerCharacter;
	if (!PlayerCharacter)
	{
		return;
	}

	// Unbind in case Initialize is called again
	PlayerCharacter->OnHealthChanged.RemoveDynamic(this, &USFPlayerHUDWidgetController::HandleHealthChanged);
	PlayerCharacter->OnEchoChanged.RemoveDynamic(this, &USFPlayerHUDWidgetController::HandleEchoChanged);
	PlayerCharacter->OnShieldsChanged.RemoveDynamic(this, &USFPlayerHUDWidgetController::HandleShieldsChanged);
	PlayerCharacter->OnStaminaChanged.RemoveDynamic(this, &USFPlayerHUDWidgetController::HandleStaminaChanged);

	PlayerCharacter->OnHealthChanged.AddDynamic(this, &USFPlayerHUDWidgetController::HandleHealthChanged);
	PlayerCharacter->OnEchoChanged.AddDynamic(this, &USFPlayerHUDWidgetController::HandleEchoChanged);
	PlayerCharacter->OnShieldsChanged.AddDynamic(this, &USFPlayerHUDWidgetController::HandleShieldsChanged);
	PlayerCharacter->OnStaminaChanged.AddDynamic(this, &USFPlayerHUDWidgetController::HandleStaminaChanged);

	if (const USFAttributeSetBase* Attributes = PlayerCharacter->GetAttributeSet())
	{
		CurrentHealth = Attributes->GetHealth();
		MaxHealth = Attributes->GetMaxHealth();
		CurrentEcho = Attributes->GetEcho();
		MaxEcho = Attributes->GetMaxEcho();
		CurrentShields = Attributes->GetShields();
		MaxShields = Attributes->GetMaxShields();
		CurrentStamina = Attributes->GetStamina();
		MaxStamina = Attributes->GetMaxStamina();
	}

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	OnEchoChanged.Broadcast(CurrentEcho, MaxEcho);
	OnShieldsChanged.Broadcast(CurrentShields, MaxShields);
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);

	UE_LOG(LogTemp, Verbose, TEXT("HUDWidgetController Initialize called: %s"), *GetNameSafe(InPlayerCharacter));

	// Reset interaction HUD state
	bHasInteractionPrompt = false;
	CurrentInteractionPromptText = FText::GetEmpty();
	CurrentInteractionPrimaryOption = FSFInteractionOption();
	CurrentInteractionOptions.Reset();

	if (ASFPlayerCharacter* PlayerPawn = Cast<ASFPlayerCharacter>(PlayerCharacter))
	{
		if (USFInteractionComponent* InteractionComponent = PlayerPawn->GetInteractionComponent())
		{
			InteractionComponent->OnInteractableChanged.RemoveDynamic(
				this, &USFPlayerHUDWidgetController::HandleInteractableChanged);

			InteractionComponent->OnInteractableChanged.AddDynamic(
				this, &USFPlayerHUDWidgetController::HandleInteractableChanged);

			// Prime initial state from the component
			const AActor* CurrentActor = InteractionComponent->GetCurrentInteractableActor();
			const FSFInteractionOption Primary = InteractionComponent->GetCurrentPrimaryOption();
			const TArray<FSFInteractionOption>& AllOptions = InteractionComponent->GetCurrentInteractionOptions();

			HandleInteractableChanged(
				const_cast<AActor*>(CurrentActor),
				Primary,
				AllOptions);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("HUDWidgetController could not find InteractionComponent."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("HUDWidgetController PlayerCharacter was not ASFPlayerCharacter: %s"),
			*GetNameSafe(PlayerCharacter));
	}

	ResetInactivityTimer();
}

void USFPlayerHUDWidgetController::HandleHealthChanged(float NewValue, float InMaxValue)
{
	CurrentHealth = NewValue;
	MaxHealth = InMaxValue;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	NotifyHUDActivity();
}

void USFPlayerHUDWidgetController::HandleEchoChanged(float NewValue, float InMaxValue)
{
	CurrentEcho = NewValue;
	MaxEcho = InMaxValue;
	OnEchoChanged.Broadcast(CurrentEcho, MaxEcho);
	NotifyHUDActivity();
}

float USFPlayerHUDWidgetController::GetHealthPercent() const
{
	return MaxHealth > KINDA_SMALL_NUMBER
		? FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f)
		: 0.0f;
}

float USFPlayerHUDWidgetController::GetEchoPercent() const
{
	return MaxEcho > KINDA_SMALL_NUMBER
		? FMath::Clamp(CurrentEcho / MaxEcho, 0.0f, 1.0f)
		: 0.0f;
}

void USFPlayerHUDWidgetController::HandleShieldsChanged(float NewValue, float InMaxValue)
{
	CurrentShields = NewValue;
	MaxShields = InMaxValue;
	OnShieldsChanged.Broadcast(CurrentShields, MaxShields);
	NotifyHUDActivity();
}

float USFPlayerHUDWidgetController::GetShieldsPercent() const
{
	return MaxShields > KINDA_SMALL_NUMBER
		? FMath::Clamp(CurrentShields / MaxShields, 0.0f, 1.0f)
		: 0.0f;
}

void USFPlayerHUDWidgetController::HandleStaminaChanged(float NewValue, float InMaxValue)
{
	CurrentStamina = NewValue;
	MaxStamina = InMaxValue;
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
	NotifyHUDActivity();
}

float USFPlayerHUDWidgetController::GetStaminaPercent() const
{
	return MaxStamina > KINDA_SMALL_NUMBER
		? FMath::Clamp(CurrentStamina / MaxStamina, 0.0f, 1.0f)
		: 0.0f;
}

float USFPlayerHUDWidgetController::GetTrackedAttributeValue(ESFTrackedAttribute Attribute) const
{
	switch (Attribute)
	{
	case ESFTrackedAttribute::Health:      return CurrentHealth;
	case ESFTrackedAttribute::MaxHealth:   return MaxHealth;
	case ESFTrackedAttribute::Echo:        return CurrentEcho;
	case ESFTrackedAttribute::MaxEcho:     return MaxEcho;
	case ESFTrackedAttribute::Shields:     return CurrentShields;
	case ESFTrackedAttribute::MaxShields:  return MaxShields;
	case ESFTrackedAttribute::Stamina:     return CurrentStamina;
	case ESFTrackedAttribute::MaxStamina:  return MaxStamina;
	default:                               return 0.0f;
	}
}

void USFPlayerHUDWidgetController::SetTrackedEnemy(ASFCharacterBase* InEnemyCharacter)
{
	if (TrackedEnemyCharacter == InEnemyCharacter)
	{
		return;
	}

	if (TrackedEnemyCharacter)
	{
		TrackedEnemyCharacter->OnHealthChanged.RemoveDynamic(
			this, &USFPlayerHUDWidgetController::HandleTrackedEnemyHealthChanged);
		TrackedEnemyCharacter->OnShieldsChanged.RemoveDynamic(
			this, &USFPlayerHUDWidgetController::HandleTrackedEnemyShieldsChanged);
	}

	TrackedEnemyCharacter = InEnemyCharacter;

	if (!TrackedEnemyCharacter)
	{
		OnTrackedEnemyChanged.Broadcast(false);
		OnTrackedEnemyHealthChanged.Broadcast(0.0f, 0.0f);
		OnTrackedEnemyShieldsChanged.Broadcast(0.0f, 0.0f);
		return;
	}

	TrackedEnemyCharacter->OnHealthChanged.RemoveDynamic(
		this, &USFPlayerHUDWidgetController::HandleTrackedEnemyHealthChanged);
	TrackedEnemyCharacter->OnHealthChanged.AddDynamic(
		this, &USFPlayerHUDWidgetController::HandleTrackedEnemyHealthChanged);

	TrackedEnemyCharacter->OnShieldsChanged.RemoveDynamic(
		this, &USFPlayerHUDWidgetController::HandleTrackedEnemyShieldsChanged);
	TrackedEnemyCharacter->OnShieldsChanged.AddDynamic(
		this, &USFPlayerHUDWidgetController::HandleTrackedEnemyShieldsChanged);

	if (const USFAttributeSetBase* Attributes = TrackedEnemyCharacter->GetAttributeSet())
	{
		OnTrackedEnemyHealthChanged.Broadcast(Attributes->GetHealth(), Attributes->GetMaxHealth());
		OnTrackedEnemyShieldsChanged.Broadcast(Attributes->GetShields(), Attributes->GetMaxShields());
	}

	OnTrackedEnemyChanged.Broadcast(true);
}

void USFPlayerHUDWidgetController::HandleTrackedEnemyHealthChanged(float NewValue, float InMaxValue)
{
	OnTrackedEnemyHealthChanged.Broadcast(NewValue, InMaxValue);

	if (TrackedEnemyCharacter && TrackedEnemyCharacter->IsDead())
	{
		SetTrackedEnemy(nullptr);
	}
}

void USFPlayerHUDWidgetController::HandleTrackedEnemyShieldsChanged(float NewValue, float InMaxValue)
{
	OnTrackedEnemyShieldsChanged.Broadcast(NewValue, InMaxValue);

	if (TrackedEnemyCharacter && TrackedEnemyCharacter->IsDead())
	{
		SetTrackedEnemy(nullptr);
	}
}

bool USFPlayerHUDWidgetController::HasTrackedEnemy() const
{
	return TrackedEnemyCharacter != nullptr;
}

void USFPlayerHUDWidgetController::HandleInteractableChanged(
	AActor* NewInteractableActor,
	FSFInteractionOption PrimaryOption,
	const TArray<FSFInteractionOption>& AllOptions)
{
	UE_LOG(LogTemp, Verbose, TEXT("HandleInteractableChanged. Actor: %s, Primary: '%s'"),
		*GetNameSafe(NewInteractableActor),
		*PrimaryOption.PromptText.ToString());

	const bool bHasActor = IsValid(NewInteractableActor);
	CurrentInteractionPrimaryOption = PrimaryOption;
	CurrentInteractionOptions = AllOptions;

	NotifyHUDActivity();

	// Simple prompt layer: use primary option if available.
	bHasInteractionPrompt = bHasActor && !PrimaryOption.PromptText.IsEmpty();
	CurrentInteractionPromptText = bHasInteractionPrompt
		? PrimaryOption.PromptText
		: FText::GetEmpty();

	OnInteractionPromptChanged.Broadcast(bHasInteractionPrompt, CurrentInteractionPromptText);
	OnInteractionOptionsChanged.Broadcast(bHasActor, CurrentInteractionPrimaryOption, CurrentInteractionOptions);
}

bool USFPlayerHUDWidgetController::HasInteractionPrompt() const
{
	return bHasInteractionPrompt;
}

FText USFPlayerHUDWidgetController::GetInteractionPromptText() const
{
	return CurrentInteractionPromptText;
}

void USFPlayerHUDWidgetController::NotifyHUDActivity()
{
	if (!bHUDVisible)
	{
		bHUDVisible = true;
		OnHUDVisibilityChanged.Broadcast(true);
	}

	ResetInactivityTimer();
}

void USFPlayerHUDWidgetController::ResetInactivityTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		HUDInactivityTimerHandle,
		this,
		&USFPlayerHUDWidgetController::OnInactivityTimerExpired,
		HUDInactivityTimeout,
		false); // not looping
}

void USFPlayerHUDWidgetController::OnInactivityTimerExpired()
{
	bHUDVisible = false;
	OnHUDVisibilityChanged.Broadcast(false);
}

UTextureRenderTarget2D* USFPlayerHUDWidgetController::GetPortraitRenderTarget() const
{
	if (const ASFPlayerCharacter* PC = Cast<ASFPlayerCharacter>(PlayerCharacter))
	{
		return PC->GetPortraitRenderTarget();
	}
	return nullptr;
}

USFEquipmentComponent* USFPlayerHUDWidgetController::GetEquipmentComponent() const
{
	if (!PlayerCharacter)
	{
		return nullptr;
	}

	return PlayerCharacter->FindComponentByClass<USFEquipmentComponent>();
}