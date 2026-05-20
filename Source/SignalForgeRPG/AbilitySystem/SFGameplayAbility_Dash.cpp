#include "AbilitySystem/SFGameplayAbility_Dash.h"

#include "Animation/AnimMontage.h"
#include "Characters/SFCharacterBase.h"
#include "Core/SignalForgeGameplayTags.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"

USFGameplayAbility_Dash::USFGameplayAbility_Dash()
{
	const FSignalForgeGameplayTags& GameplayTags = FSignalForgeGameplayTags::Get();

	InputTag = GameplayTags.Input_Ability_3;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(GameplayTags.Ability_Movement_Dash);
	SetAssetTags(AssetTags);

	// While dashing, tag the owner so anim BPs, damage handlers, and other systems can react.
	ActivationOwnedTags.AddTag(GameplayTags.State_Movement_Dashing);
	if (bGrantInvulnerabilityDuringDash)
	{
		ActivationOwnedTags.AddTag(GameplayTags.State_Invulnerable_Dodge);
	}

	// Don't re-dash while a dash window is already active.
	ActivationBlockedTags.AddTag(GameplayTags.State_Movement_Dashing);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool USFGameplayAbility_Dash::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	ASFCharacterBase* Character = nullptr;
	return TryGetDashCharacter(ActorInfo, Character) && DashStrength > 0.0f;
}

void USFGameplayAbility_Dash::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ASFCharacterBase* Character = nullptr;
	if (!TryGetDashCharacter(ActorInfo, Character))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FVector DashDirection = ResolveDashDirection(Character);
	if (DashDirection.IsNearlyZero())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Open the dash window BEFORE the launch so gravity is already suspended when the impulse fires.
	BeginDashWindow(Character);

	// Prefer base-class ActivationMontage (set via editor) and only fall back to legacy DashMontage.
	if (!PlayAbilityMontage())
	{
		if (DashMontage)
		{
			Character->PlayAnimMontage(DashMontage);
		}
	}

	Character->PerformDash(DashDirection, DashStrength);

	// Prefer base-class ActivationVFX (set via editor), fall back to legacy DashNiagara.
	SpawnAbilityVFX();
	if (DashNiagara)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			DashNiagara,
			Character->GetRootComponent(),
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true);
	}

	PlayAbilitySFX();

	// NOTE: We intentionally do NOT EndAbility here. The ability stays active for DashDuration so
	// ActivationOwnedTags (State.Movement.Dashing / State.Invulnerable.Dodge) remain applied for
	// the full dash window. HandleDashWindowExpired ends the ability when the timer fires.
}

void USFGameplayAbility_Dash::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// Always restore movement state, even on cancel, so we never leave the character in a no-gravity / no-friction state.
	EndDashWindow();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool USFGameplayAbility_Dash::TryGetDashCharacter(
	const FGameplayAbilityActorInfo* ActorInfo,
	ASFCharacterBase*& OutCharacter) const
{
	OutCharacter = nullptr;

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		return false;
	}

	OutCharacter = Cast<ASFCharacterBase>(ActorInfo->AvatarActor.Get());
	return OutCharacter != nullptr;
}

FVector USFGameplayAbility_Dash::ResolveDashDirection(const ASFCharacterBase* Character) const
{
	if (!Character)
	{
		return FVector::ZeroVector;
	}

	if (bUseLastMovementInputForDash)
	{
		FVector InputDirection = Character->GetLastMovementInputVector();
		InputDirection.Z = 0.0f;

		if (!InputDirection.IsNearlyZero())
		{
			return InputDirection.GetSafeNormal();
		}
	}

	FVector ForwardDirection = Character->GetActorForwardVector();
	ForwardDirection.Z = 0.0f;
	return ForwardDirection.GetSafeNormal();
}

void USFGameplayAbility_Dash::BeginDashWindow(ASFCharacterBase* Character)
{
	if (!Character)
	{
		return;
	}

	UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	CachedMovementComp = MoveComp;
	CachedGravityScale = MoveComp->GravityScale;
	CachedBrakingFrictionFactor = MoveComp->BrakingFrictionFactor;

	if (bZeroVelocityBeforeLaunch)
	{
		// Wipe current velocity so the LaunchCharacter impulse is consistent regardless of pre-dash motion.
		MoveComp->Velocity = FVector::ZeroVector;
	}

	if (bSuspendGravityDuringDash)
	{
		// Keeps the dash perfectly horizontal even when airborne; gravity is restored when the window closes.
		MoveComp->GravityScale = 0.0f;
	}

	// Kill friction so the launch impulse isn't immediately bled off.
	MoveComp->BrakingFrictionFactor = 0.0f;

	bDashWindowActive = true;

	// Schedule the window to close. Use the world timer manager so it survives ability re-instancing.
	if (UWorld* World = Character->GetWorld())
	{
		World->GetTimerManager().ClearTimer(DashWindowTimer);
		World->GetTimerManager().SetTimer(
			DashWindowTimer,
			this,
			&USFGameplayAbility_Dash::HandleDashWindowExpired,
			FMath::Max(0.05f, DashDuration),
			false);
	}
}

void USFGameplayAbility_Dash::EndDashWindow()
{
	if (!bDashWindowActive)
	{
		return;
	}

	if (UCharacterMovementComponent* MoveComp = CachedMovementComp.Get())
	{
		MoveComp->GravityScale = CachedGravityScale;
		MoveComp->BrakingFrictionFactor = CachedBrakingFrictionFactor;

		if (UWorld* World = MoveComp->GetWorld())
		{
			World->GetTimerManager().ClearTimer(DashWindowTimer);
		}
	}

	CachedMovementComp.Reset();
	bDashWindowActive = false;
}

void USFGameplayAbility_Dash::HandleDashWindowExpired()
{
	// Restore movement state and end the ability so ActivationOwnedTags are removed.
	EndDashWindow();

	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}
