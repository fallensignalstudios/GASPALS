#include "Animation/SFAnimInstanceBase.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Characters/SFCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "KismetAnimationLibrary.h"
#include "TimerManager.h"

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

void USFAnimInstanceBase::NativeUninitializeAnimation()
{
    Super::NativeUninitializeAnimation();

    if (OwningCharacter)
    {
        if (UAbilitySystemComponent* ASC = OwningCharacter->FindComponentByClass<UAbilitySystemComponent>())
        {
            if (ApplyTagsHandle.IsValid())
            {
                ASC->RemoveActiveGameplayEffect(ApplyTagsHandle);
                ApplyTagsHandle.Invalidate();
            }
        }
    }

    OwningCharacter = nullptr;
    MovementComponent = nullptr;
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
        bWantsHeadLookAt = false;
        HeadLookAtAlpha = 0.0f;
        HeadLookAtLocationWS = FVector::ZeroVector;
        HeadLookAtLocationLS = FVector::ZeroVector;
        return;
    }

    UpperBodyOverlayWeight = bUseUpperBodyOverlay
        ? FMath::FInterpTo(UpperBodyOverlayWeight, 1.0f, DeltaSeconds, OverlayBlendSpeed)
        : FMath::FInterpTo(UpperBodyOverlayWeight, 0.0f, DeltaSeconds, OverlayBlendSpeed);

    bool bCharacterWantsLookAt = false;
    const FVector LookAtWS = OwningCharacter->GetHeadLookAtLocation(bCharacterWantsLookAt);

    bWantsHeadLookAt = bCharacterWantsLookAt;
    HeadLookAtLocationWS = LookAtWS;

    if (USkeletalMeshComponent* MeshComp = GetSkelMeshComponent())
    {
        const FTransform MeshTransform = MeshComp->GetComponentTransform();
        HeadLookAtLocationLS = MeshTransform.InverseTransformPosition(LookAtWS);
    }
    else
    {
        HeadLookAtLocationLS = FVector::ZeroVector;
    }

    const float TargetAlpha = bWantsHeadLookAt ? 1.0f : 0.0f;
    HeadLookAtAlpha = FMath::FInterpTo(HeadLookAtAlpha, TargetAlpha, DeltaSeconds, HeadLookAtInterpSpeed);
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
    LastOverrideLayerTag = CurrentOverrideLayerTag;
    CurrentOverrideLayerTag = LayerTag;
    OverrideLayerBlendInTime = BlendInTime;
    OverrideLayerBlendOutTime = 0.0f;
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
}

void USFAnimInstanceBase::OnOverrideLayerBlendedOut()
{
    LastOverrideLayerTag = CurrentOverrideLayerTag;
    CurrentOverrideLayerTag = FGameplayTag();
}