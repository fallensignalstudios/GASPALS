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

    const float MaxWalkSpeed = FMath::Max(MovementComponent->GetMaxSpeed(), 1.0f);
    GroundSpeedNormalized = FMath::Clamp(GroundSpeed / MaxWalkSpeed, 0.0f, 1.0f);

    const FVector CurrentAcceleration = MovementComponent->GetCurrentAcceleration();
    MovementInputAmount = FMath::Clamp(CurrentAcceleration.Size() / MaxWalkSpeed, 0.0f, 1.0f);

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

    if (bIsFalling)
    {
        LocomotionState = ESFLocomotionState::InAir;
    }
    else if (!bShouldMove)
    {
        LocomotionState = ESFLocomotionState::Idle;
    }
    else if (GroundSpeed < WalkSpeedThreshold)
    {
        LocomotionState = ESFLocomotionState::Walk;
    }
    else if (GroundSpeed < JogSpeedThreshold)
    {
        LocomotionState = ESFLocomotionState::Jog;
    }
    else
    {
        LocomotionState = ESFLocomotionState::Sprint;
    }
}

void USFAnimInstanceBase::UpdateAnimationStateFromCharacter(float DeltaSeconds)
{
    if (!HasValidOwner())
    {
        ResetAnimationProfile();
        return;
    }

    UpperBodyOverlayWeight = bUseUpperBodyOverlay
        ? FMath::FInterpTo(UpperBodyOverlayWeight, 1.0f, DeltaSeconds, OverlayBlendSpeed)
        : FMath::FInterpTo(UpperBodyOverlayWeight, 0.0f, DeltaSeconds, OverlayBlendSpeed);

    // Later: query character API to drive overlay/combat/profile.
    // OverlayMode = OwningCharacter->GetOverlayMode();
    // CombatMode = OwningCharacter->GetCombatMode();
    // FSFWeaponAnimationProfile Profile = OwningCharacter->GetWeaponAnimationProfile();
    // SetAnimationProfile(Profile, DeltaSeconds);
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