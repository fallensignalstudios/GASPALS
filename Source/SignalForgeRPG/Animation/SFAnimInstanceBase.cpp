#include "Animation/SFAnimInstanceBase.h"

#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimMontage.h"
#include "Characters/SFCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "KismetAnimationLibrary.h"

USFAnimInstanceBase::USFAnimInstanceBase()
{
}

void USFAnimInstanceBase::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    UpdateOwnerReferences();
    ResetRuntimeData();
    ResetAnimationProfile();

    if (HasValidOwner())
    {
        PreviousActorRotation = OwningCharacter->GetActorRotation();
        UpdateAnimationStateFromCharacter(0.0f);
    }
}

void USFAnimInstanceBase::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    UpdateOwnerReferences();

    if (!HasValidOwner())
    {
        ResetRuntimeData();
        ResetAnimationProfile();
        return;
    }

    UpdateMovementData(DeltaSeconds);
    UpdateAnimationStateFromCharacter(DeltaSeconds);
}

bool USFAnimInstanceBase::HasValidOwner() const
{
    return IsValid(OwningCharacter) && IsValid(MovementComponent);
}

void USFAnimInstanceBase::UpdateOwnerReferences()
{
    if (IsValid(OwningCharacter) && IsValid(MovementComponent))
    {
        return;
    }

    APawn* PawnOwner = TryGetPawnOwner();
    if (!IsValid(PawnOwner))
    {
        OwningCharacter = nullptr;
        MovementComponent = nullptr;
        return;
    }

    OwningCharacter = Cast<ASFCharacterBase>(PawnOwner);
    MovementComponent = PawnOwner->FindComponentByClass<UCharacterMovementComponent>();
}

void USFAnimInstanceBase::ResetRuntimeData()
{
    WorldVelocity = FVector::ZeroVector;
    LocalVelocity = FVector::ZeroVector;
    GroundSpeed = 0.0f;
    GroundSpeedNormalized = 0.0f;
    VelocityZ = 0.0f;
    Direction = 0.0f;
    MovementInputAmount = 0.0f;
    YawDeltaSpeed = 0.0f;
    bIsFalling = false;
    bIsAccelerating = false;
    bShouldMove = false;
    bHasMovementInput = false;
    AimRotation = FRotator::ZeroRotator;
    ActorRotation = FRotator::ZeroRotator;
    LocomotionState = ESFLocomotionState::Idle;
    UpperBodyOverlayWeight = 0.0f;
    PreviousActorRotation = FRotator::ZeroRotator;
    bIsBlocking = false;
}

void USFAnimInstanceBase::ResetAnimationProfile()
{
    OverlayMode = ESFOverlayMode::Unarmed;
    CombatMode = ESFCombatMode::None;
    bUseUpperBodyOverlay = true;
    IdleOverride = nullptr;
    LightAttackMontage = nullptr;
    HeavyAttackMontage = nullptr;
    AbilityMontage = nullptr;
}

void USFAnimInstanceBase::UpdateMovementData(float DeltaSeconds)
{
    if (!HasValidOwner())
    {
        ResetRuntimeData();
        return;
    }

    WorldVelocity = MovementComponent->Velocity;
    VelocityZ = WorldVelocity.Z;

    const FVector HorizontalVelocity(WorldVelocity.X, WorldVelocity.Y, 0.0f);
    GroundSpeed = HorizontalVelocity.Size();

    ActorRotation = OwningCharacter->GetActorRotation();
    AimRotation = OwningCharacter->GetBaseAimRotation();

    LocalVelocity = ActorRotation.UnrotateVector(WorldVelocity);
    Direction = UKismetAnimationLibrary::CalculateDirection(WorldVelocity, ActorRotation);

    bIsFalling = MovementComponent->IsFalling();
    bIsAccelerating = MovementComponent->GetCurrentAcceleration().SizeSquared() > 0.0f;
    bHasMovementInput = bIsAccelerating;
    bShouldMove = GroundSpeed > MoveThreshold && !bIsFalling;

    // Normalize against the *configured* MaxWalkSpeed property (a stable value),
    // not GetMaxSpeed() — GetMaxSpeed() is modulated by analog-input modifier,
    // crouch, root motion, and movement-mode and therefore shimmers frame-to-frame.
    // Feeding a shimmering normalized speed into Motion Matching produces visible
    // stutter because the database picks a different pose every tick.
    const float MaxReferenceSpeed = FMath::Max(MovementComponent->MaxWalkSpeed, 1.0f);
    GroundSpeedNormalized = FMath::Clamp(GroundSpeed / MaxReferenceSpeed, 0.0f, 1.0f);

    const FVector CurrentAcceleration = MovementComponent->GetCurrentAcceleration();
    MovementInputAmount = FMath::Clamp(CurrentAcceleration.Size() / MaxReferenceSpeed, 0.0f, 1.0f);

    if (DeltaSeconds > SMALL_NUMBER)
    {
        const float DeltaYaw = FMath::FindDeltaAngleDegrees(PreviousActorRotation.Yaw, ActorRotation.Yaw);
        YawDeltaSpeed = DeltaYaw / DeltaSeconds;
    }
    else
    {
        YawDeltaSpeed = 0.0f;
    }

    PreviousActorRotation = ActorRotation;

    // Locomotion state uses *proportional* thresholds against MaxWalkSpeed, not
    // raw cm/s. Two reasons:
    //   1) Strafe / backpedal speeds are clamped lower than forward by
    //      CharacterMovement (lateral input modifier), so a flat "GroundSpeed
    //      < WalkSpeedThreshold" comparison would snap a full-stick strafe
    //      back to Walk even though forward at the same input is Jog. That's
    //      the "strafe drops me back to walking" symptom.
    //   2) Changing MaxWalkSpeed (sprint multiplier, ADS slowdown, crouch, etc.)
    //      automatically rescales the bands instead of dragging the player
    //      through Walk/Jog/Sprint as the cap changes.
    //
    // Hysteresis: once we're in a state, the band we have to drop below to
    // exit it is slightly tighter than the band we had to cross to enter it.
    // This is what kills MM pose-flicker at the band boundaries.
    const float WalkPct = GroundSpeedNormalized;
    constexpr float WalkUp     = 0.35f;
    constexpr float WalkDown   = 0.30f;
    constexpr float JogUp      = 0.85f;
    constexpr float JogDown    = 0.80f;

    if (bIsFalling)
    {
        LocomotionState = ESFLocomotionState::InAir;
    }
    else if (!bShouldMove)
    {
        LocomotionState = ESFLocomotionState::Idle;
    }
    else
    {
        const ESFLocomotionState PrevState = LocomotionState;
        ESFLocomotionState NextState = PrevState;

        switch (PrevState)
        {
        case ESFLocomotionState::Sprint:
            // Drop to Jog only if we've cleanly fallen below JogDown.
            if (WalkPct < JogDown)
            {
                NextState = (WalkPct < WalkDown) ? ESFLocomotionState::Walk
                                                 : ESFLocomotionState::Jog;
            }
            break;

        case ESFLocomotionState::Jog:
            if (WalkPct >= JogUp)
            {
                NextState = ESFLocomotionState::Sprint;
            }
            else if (WalkPct < WalkDown)
            {
                NextState = ESFLocomotionState::Walk;
            }
            break;

        case ESFLocomotionState::Walk:
            if (WalkPct >= JogUp)
            {
                NextState = ESFLocomotionState::Sprint;
            }
            else if (WalkPct >= WalkUp)
            {
                NextState = ESFLocomotionState::Jog;
            }
            break;

        case ESFLocomotionState::Idle:
        case ESFLocomotionState::InAir:
        default:
            // Coming out of Idle / InAir — pick the band on entry-thresholds.
            if (WalkPct >= JogUp)        NextState = ESFLocomotionState::Sprint;
            else if (WalkPct >= WalkUp)  NextState = ESFLocomotionState::Jog;
            else                         NextState = ESFLocomotionState::Walk;
            break;
        }

        LocomotionState = NextState;
    }
}

void USFAnimInstanceBase::UpdateAnimationStateFromCharacter(float DeltaSeconds)
{
    if (!HasValidOwner())
    {
        ResetAnimationProfile();
        return;
    }

    // Mirror the character's current animation state into the ABP every tick.
    // Order matters: pull bUseUpperBodyOverlay from the character first so the
    // weight interpolation below sees the correct target value. If a weapon
    // profile is active, SetAnimationProfile will overwrite IdleOverride and
    // the montages too — the upper-body flag from the profile takes priority
    // over the bare character flag (Destiny-style: weapon dictates stance).
    OverlayMode = OwningCharacter->GetCurrentOverlayMode();
    CombatMode = OwningCharacter->GetCurrentCombatMode();
    bUseUpperBodyOverlay = OwningCharacter->GetUseUpperBodyOverlay();
    bWantsToAim = OwningCharacter->GetWantsToAim();
    bWantsToSprint = OwningCharacter->GetWantsToSprint();
    bWantsToWalk = OwningCharacter->GetWantsToWalk();
    bWantsToStrafe = OwningCharacter->GetWantsToStrafe();
    Gait = OwningCharacter->GetCurrentGait();

    if (OwningCharacter->HasWeaponAnimationProfile())
    {
        SetAnimationProfile(OwningCharacter->GetCurrentWeaponAnimationProfile(), DeltaSeconds);
    }
    else
    {
        // No weapon profile active — just blend the upper-body overlay weight
        // based on the character flag. SetAnimationProfile would do this for
        // us, but we don't want to stomp IdleOverride / montages with zeros.
        UpperBodyOverlayWeight = bUseUpperBodyOverlay
            ? FMath::FInterpTo(UpperBodyOverlayWeight, 1.0f, DeltaSeconds, OverlayBlendSpeed)
            : FMath::FInterpTo(UpperBodyOverlayWeight, 0.0f, DeltaSeconds, OverlayBlendSpeed);
    }
}

void USFAnimInstanceBase::SetAnimationProfile(const FSFWeaponAnimationProfile& InProfile, float DeltaSeconds)
{
    IdleOverride = InProfile.IdleOverride;
    LightAttackMontage = InProfile.LightAttackMontage;
    HeavyAttackMontage = InProfile.HeavyAttackMontage;
    AbilityMontage = InProfile.AbilityMontage;
    bUseUpperBodyOverlay = InProfile.bUseUpperBodyOverlay;

    UpperBodyOverlayWeight = bUseUpperBodyOverlay
        ? FMath::FInterpTo(UpperBodyOverlayWeight, 1.0f, DeltaSeconds, OverlayBlendSpeed)
        : FMath::FInterpTo(UpperBodyOverlayWeight, 0.0f, DeltaSeconds, OverlayBlendSpeed);
}

bool USFAnimInstanceBase::GetWeaponProfileByTag(const FGameplayTag& ProfileTag, FSFWeaponAnimationProfile& OutProfile) const
{
    if (const FSFWeaponAnimationProfile* FoundProfile = TaggedWeaponProfiles.Find(ProfileTag))
    {
        OutProfile = *FoundProfile;
        return true;
    }
    return false;
}

bool USFAnimInstanceBase::ApplyOverrideLayer(const FGameplayTag& LayerTag, float BlendInTime)
{
    if (!TaggedOverrideLayers.Contains(LayerTag))
    {
        return false;
    }

    bHasOverrideLayer = true;
    CurrentOverrideLayerTag = LayerTag;
    OverrideLayerBlendInTime = BlendInTime;
    // Your ABP can read these and drive a linked layer / state machine.
    return true;
}

void USFAnimInstanceBase::RemoveOverrideLayer(float BlendOutTime)
{
    if (!bHasOverrideLayer)
    {
        return;
    }

    bHasOverrideLayer = false;
    OverrideLayerBlendOutTime = BlendOutTime;
    // Optionally keep CurrentOverrideLayerTag for history, or clear it.
}