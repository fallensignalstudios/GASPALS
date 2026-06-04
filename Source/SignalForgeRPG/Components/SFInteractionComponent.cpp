#include "Components/SFInteractionComponent.h"

#include "Characters/SFPlayerCharacter.h"
#include "Components/SFInteractableInterface.h"
#include "Dialogue/Data/SFConversationSourceInterface.h"
#include "Dialogue/Data/SFConversationDataAsset.h"
#include "Dialogue/Data/SFDialogueComponent.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Core/SignalForgeLogChannels.h"

static TAutoConsoleVariable<int32> CVarSFInteractionDebug(
	TEXT("sf.Interaction.Debug"),
	0,
	TEXT("When >0, log every interaction trace result to LogTemp at Display level."),
	ECVF_Default);

USFInteractionComponent::USFInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USFInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());

	// Unconditional log so we always know whether the component spawned.
	UE_LOG(LogSFUI, Display,
		TEXT("SFInteractionComponent::BeginPlay on '%s' (Owner=%s, TickEnabled=%d, NetMode=%d)"),
		*GetNameSafe(this),
		*GetNameSafe(GetOwner()),
		IsComponentTickEnabled() ? 1 : 0,
		GetWorld() ? static_cast<int32>(GetWorld()->GetNetMode()) : -1);

	if (!OwnerCharacter)
	{
		UE_LOG(LogSFUI, Warning,
			TEXT("SFInteractionComponent owner is not a Character: %s. Tick disabled."),
			*GetNameSafe(GetOwner()));
		SetComponentTickEnabled(false);
	}
}

void USFInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CurrentInteractableActor && CurrentInteractableActor->GetClass()->ImplementsInterface(USFInteractableInterface::StaticClass()))
	{
		const FSFInteractionContext Context = BuildInteractionContextForActor(CurrentInteractableActor);
		ISFInteractableInterface::Execute_EndInteractionFocus(CurrentInteractableActor, Context);
	}

	Super::EndPlay(EndPlayReason);
}

void USFInteractionComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Heartbeat: prove Tick is firing at all. Throttled per-instance so we
	// don't only see one component when multiple are alive (every NPC has one).
	if (CVarSFInteractionDebug.GetValueOnGameThread() > 0)
	{
		const double Now = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;
		if (Now - LastHeartbeatLogTime > 1.0)
		{
			LastHeartbeatLogTime = Now;
			UE_LOG(LogSFUI, Display,
				TEXT("SFInteractionComponent: Tick alive (Enabled=%d, LocallyControlled=%d, Owner=%s)"),
				bInteractionEnabled ? 1 : 0,
				IsLocallyControlled() ? 1 : 0,
				*GetNameSafe(OwnerCharacter));
		}
	}

	if (!bInteractionEnabled || !IsLocallyControlled())
	{
		if (CurrentInteractableActor)
		{
			const FSFInteractionContext Context = BuildInteractionContextForActor(CurrentInteractableActor);
			SetCurrentInteractable(nullptr, Context);
		}
		return;
	}

	UpdateCurrentInteractable();
}

void USFInteractionComponent::TryInteract()
{
	if (!bInteractionEnabled || !OwnerCharacter || !CurrentInteractableActor)
	{
		return;
	}

	if (TryStartConversation(CurrentInteractableActor))
	{
		return;
	}

	if (!CurrentInteractableActor->GetClass()->ImplementsInterface(USFInteractableInterface::StaticClass()))
	{
		return;
	}

	const FSFInteractionContext Context = BuildInteractionContextForActor(CurrentInteractableActor);
	const FSFInteractionOption PrimaryOption =
		ISFInteractableInterface::Execute_GetPrimaryInteractionOption(CurrentInteractableActor, Context);

	if (!PrimaryOption.IsAvailable())
	{
		return;
	}

	TryInteractWithOption(PrimaryOption.OptionId);
}

void USFInteractionComponent::TryInteractWithOption(FName OptionId)
{
	if (!bInteractionEnabled || !OwnerCharacter || !CurrentInteractableActor)
	{
		return;
	}

	if (!CurrentInteractableActor->GetClass()->ImplementsInterface(USFInteractableInterface::StaticClass()))
	{
		return;
	}

	const FSFInteractionContext Context = BuildInteractionContextForActor(CurrentInteractableActor);

	const FSFInteractionExecutionResult Result =
		ISFInteractableInterface::Execute_InteractWithOption(CurrentInteractableActor, Context, OptionId);

	if (!Result.bSucceeded)
	{
		// Hook for failure UI/audio later if desired.
	}
}

void USFInteractionComponent::SetInteractionEnabled(bool bEnabled)
{
	if (bInteractionEnabled == bEnabled)
	{
		return;
	}

	bInteractionEnabled = bEnabled;

	if (!bInteractionEnabled && CurrentInteractableActor)
	{
		const FSFInteractionContext Context = BuildInteractionContextForActor(CurrentInteractableActor);
		SetCurrentInteractable(nullptr, Context);
	}
}

void USFInteractionComponent::UpdateCurrentInteractable()
{
	FHitResult HitResult;
	AActor* NewInteractableActor = nullptr;
	FSFInteractionContext NewContext;

	if (PerformInteractionTrace(HitResult))
	{
		AActor* HitActor = HitResult.GetActor();
		if (IsValid(HitActor) && HitActor->GetClass()->ImplementsInterface(USFInteractableInterface::StaticClass()))
		{
			NewContext = BuildInteractionContextFromHit(HitResult);

			if (IsActorInteractable(NewContext))
			{
				NewInteractableActor = HitActor;
			}
			else if (CVarSFInteractionDebug.GetValueOnGameThread() > 0)
			{
				const ESFInteractionAvailability Availability =
					ISFInteractableInterface::Execute_GetInteractionAvailability(HitActor, NewContext);
				UE_LOG(LogSFUI, Display,
					TEXT("SFInteractionComponent: trace hit '%s' but it was rejected (Availability=%d)."),
					*GetNameSafe(HitActor),
					static_cast<int32>(Availability));
			}
		}
		else if (IsValid(HitActor) && CVarSFInteractionDebug.GetValueOnGameThread() > 0)
		{
			UE_LOG(LogSFUI, Display,
				TEXT("SFInteractionComponent: trace hit '%s' which does not implement ISFInteractableInterface."),
				*GetNameSafe(HitActor));
		}
	}
	else if (CVarSFInteractionDebug.GetValueOnGameThread() > 0)
	{
		UE_LOG(LogSFUI, Verbose, TEXT("SFInteractionComponent: trace did not hit any actor."));
	}

	SetCurrentInteractable(NewInteractableActor, NewContext);
}

bool USFInteractionComponent::PerformInteractionTrace(FHitResult& OutHitResult) const
{
	if (!OwnerCharacter)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FVector EyeLocation;
	FRotator EyeRotation;
	OwnerCharacter->GetActorEyesViewPoint(EyeLocation, EyeRotation);

	const FVector Start = EyeLocation;
	const FVector End = Start + (EyeRotation.Vector() * InteractionTraceDistance);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SFInteractionTrace), false);
	QueryParams.AddIgnoredActor(OwnerCharacter);

	const bool bHit = World->SweepSingleByChannel(
		OutHitResult,
		Start,
		End,
		FQuat::Identity,
		InteractionTraceChannel,
		FCollisionShape::MakeSphere(InteractionTraceRadius),
		QueryParams
	);

	if (bDrawDebugInteractionTrace || CVarSFInteractionDebug.GetValueOnGameThread() > 0)
	{
		DrawDebugLine(World, Start, End, bHit ? FColor::Green : FColor::Red, false, 0.0f, 0, 1.0f);
		DrawDebugSphere(World, End, InteractionTraceRadius, 12, bHit ? FColor::Green : FColor::Red, false, 0.0f);
		if (bHit && IsValid(OutHitResult.GetActor()))
		{
			DrawDebugSphere(World, OutHitResult.ImpactPoint, InteractionTraceRadius * 0.5f, 12, FColor::Yellow, false, 0.0f);
		}
	}

	return bHit && IsValid(OutHitResult.GetActor());
}

void USFInteractionComponent::SetCurrentInteractable(
	AActor* NewInteractableActor,
	const FSFInteractionContext& NewContext)
{
	if (CurrentInteractableActor == NewInteractableActor)
	{
		if (CurrentInteractableActor && CurrentInteractableActor->GetClass()->ImplementsInterface(USFInteractableInterface::StaticClass()))
		{
			const FSFInteractionContext Context = BuildInteractionContextForActor(CurrentInteractableActor);
			CurrentInteractionOptions = ResolveInteractionOptions(Context);
			CurrentPrimaryOption = CurrentInteractionOptions.Num() > 0
				? CurrentInteractionOptions[0]
				: ResolvePrimaryOption(Context);

			OnInteractableChanged.Broadcast(CurrentInteractableActor, CurrentPrimaryOption, CurrentInteractionOptions);
		}
		return;
	}

	if (CurrentInteractableActor && CurrentInteractableActor->GetClass()->ImplementsInterface(USFInteractableInterface::StaticClass()))
	{
		const FSFInteractionContext OldContext = BuildInteractionContextForActor(CurrentInteractableActor);
		ISFInteractableInterface::Execute_EndInteractionFocus(CurrentInteractableActor, OldContext);
	}

	CurrentInteractableActor = NewInteractableActor;
	CurrentPrimaryOption = FSFInteractionOption();
	CurrentInteractionOptions.Reset();

	if (CurrentInteractableActor && CurrentInteractableActor->GetClass()->ImplementsInterface(USFInteractableInterface::StaticClass()))
	{
		const FSFInteractionContext FocusContext =
			NewContext.IsValid() ? NewContext : BuildInteractionContextForActor(CurrentInteractableActor);

		ISFInteractableInterface::Execute_BeginInteractionFocus(CurrentInteractableActor, FocusContext);

		CurrentInteractionOptions = ResolveInteractionOptions(FocusContext);
		CurrentPrimaryOption = CurrentInteractionOptions.Num() > 0
			? CurrentInteractionOptions[0]
			: ResolvePrimaryOption(FocusContext);
	}

	UE_LOG(LogSFUI, Verbose,
		TEXT("SFInteractionComponent: SetCurrentInteractable -> %s, prompt='%s', options=%d"),
		*GetNameSafe(CurrentInteractableActor),
		*CurrentPrimaryOption.PromptText.ToString(),
		CurrentInteractionOptions.Num());

	OnInteractableChanged.Broadcast(CurrentInteractableActor, CurrentPrimaryOption, CurrentInteractionOptions);
}

bool USFInteractionComponent::IsLocallyControlled() const
{
	const APawn* OwnerPawn = Cast<APawn>(OwnerCharacter);
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
	{
		return false;
	}

	// Critical: every ASFCharacterBase has an InteractionComponent, including
	// NPCs and AI companions. On standalone / listen-server, AI-controlled
	// pawns also return IsLocallyControlled()=true because the server is the
	// local authority. We must additionally require an APlayerController so
	// only the actual player runs interaction traces.
	return OwnerPawn->GetController() && OwnerPawn->GetController()->IsA<APlayerController>();
}

bool USFInteractionComponent::IsActorInteractable(const FSFInteractionContext& Context) const
{
	if (!Context.IsValid() || !IsValid(Context.HitResult.GetActor()))
	{
		return false;
	}

	AActor* TargetActor = Context.HitResult.GetActor();
	if (!TargetActor->GetClass()->ImplementsInterface(USFInteractableInterface::StaticClass()))
	{
		return false;
	}

	const ESFInteractionAvailability Availability =
		ISFInteractableInterface::Execute_GetInteractionAvailability(TargetActor, Context);

	if (Availability == ESFInteractionAvailability::Available)
	{
		return true;
	}

	// Hard-disabled (Disabled / Invalid) targets are never surfaced.
	if (Availability == ESFInteractionAvailability::Disabled
		|| Availability == ESFInteractionAvailability::Invalid)
	{
		return false;
	}

	// Soft-unavailable targets (e.g. OutOfRange) are surfaced if the option asks
	// the HUD to display an unavailable prompt. This lets designers show a grey
	// "Talk" prompt while the player closes the gap, instead of nothing at all.
	const FSFInteractionOption PrimaryOption =
		ISFInteractableInterface::Execute_GetPrimaryInteractionOption(TargetActor, Context);

	return PrimaryOption.bShowPromptWhenUnavailable;
}

FSFInteractionContext USFInteractionComponent::BuildInteractionContextFromHit(const FHitResult& HitResult) const
{
	FSFInteractionContext Context;
	Context.InteractingActor = OwnerCharacter;
	Context.InteractingComponent = const_cast<USFInteractionComponent*>(this);
	Context.HitResult = HitResult;

	if (OwnerCharacter)
	{
		FVector EyeLocation;
		FRotator EyeRotation;
		OwnerCharacter->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		Context.InteractionOrigin = EyeLocation;
		Context.InteractionDirection = EyeRotation.Vector();
		Context.DistanceToTarget = FVector::Dist(EyeLocation, HitResult.ImpactPoint);
	}

	return Context;
}

FSFInteractionContext USFInteractionComponent::BuildInteractionContextForActor(AActor* Actor) const
{
	FSFInteractionContext Context;
	Context.InteractingActor = OwnerCharacter;
	Context.InteractingComponent = const_cast<USFInteractionComponent*>(this);

	if (OwnerCharacter && Actor)
	{
		FVector EyeLocation;
		FRotator EyeRotation;
		OwnerCharacter->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		Context.InteractionOrigin = EyeLocation;
		Context.InteractionDirection = (Actor->GetActorLocation() - EyeLocation).GetSafeNormal();
		Context.DistanceToTarget = FVector::Dist(EyeLocation, Actor->GetActorLocation());
	}

	return Context;
}

FSFInteractionOption USFInteractionComponent::ResolvePrimaryOption(const FSFInteractionContext& Context) const
{
	FSFInteractionOption Option;

	AActor* TargetActor = Context.HitResult.GetActor();
	if (!TargetActor)
	{
		TargetActor = CurrentInteractableActor;
	}

	if (!TargetActor || !TargetActor->GetClass()->ImplementsInterface(USFInteractableInterface::StaticClass()))
	{
		return Option;
	}

	return ISFInteractableInterface::Execute_GetPrimaryInteractionOption(TargetActor, Context);
}

TArray<FSFInteractionOption> USFInteractionComponent::ResolveInteractionOptions(const FSFInteractionContext& Context) const
{
	TArray<FSFInteractionOption> Options;

	AActor* TargetActor = Context.HitResult.GetActor();
	if (!TargetActor)
	{
		TargetActor = CurrentInteractableActor;
	}

	if (!TargetActor || !TargetActor->GetClass()->ImplementsInterface(USFInteractableInterface::StaticClass()))
	{
		return Options;
	}

	Options = ISFInteractableInterface::Execute_GetInteractionOptions(TargetActor, Context);

	if (Options.Num() == 0)
	{
		const FSFInteractionOption PrimaryOption =
			ISFInteractableInterface::Execute_GetPrimaryInteractionOption(TargetActor, Context);

		if (!PrimaryOption.OptionId.IsNone() || !PrimaryOption.PromptText.IsEmpty())
		{
			Options.Add(PrimaryOption);
		}
	}

	return Options;
}

bool USFInteractionComponent::TryStartConversation(AActor* TargetActor)
{
	if (!IsValid(TargetActor) || !OwnerCharacter)
	{
		return false;
	}

	if (!TargetActor->GetClass()->ImplementsInterface(USFConversationSourceInterface::StaticClass()))
	{
		return false;
	}

	ASFPlayerCharacter* PlayerCharacter = Cast<ASFPlayerCharacter>(OwnerCharacter);
	if (!PlayerCharacter)
	{
		return false;
	}

	USFDialogueComponent* DialogueComponent = PlayerCharacter->GetDialogueComponent();
	if (!DialogueComponent || DialogueComponent->IsConversationActive())
	{
		return false;
	}

	USFConversationDataAsset* ConversationAsset =
		ISFConversationSourceInterface::Execute_GetConversationAsset(TargetActor);

	if (!IsValid(ConversationAsset) || !ConversationAsset->IsValidConversation())
	{
		return false;
	}

	return DialogueComponent->StartConversation(ConversationAsset, TargetActor);
}