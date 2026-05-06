#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayTagContainer.h"
#include "Animation/SFAnimationTypes.h"
#include "SFAnimInstanceBase.generated.h"

class ASFCharacterBase;
class UCharacterMovementComponent;
class UAnimSequenceBase;
class UAnimMontage;

UCLASS(Blueprintable)
class SIGNALFORGERPG_API USFAnimInstanceBase : public UAnimInstance
{
    GENERATED_BODY()

public:
    USFAnimInstanceBase();

    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

    UFUNCTION(BlueprintPure, Category = "Animation")
    bool HasValidOwner() const;

    UFUNCTION(BlueprintCallable, Category = "Animation|State")
    void SetIsBlocking(bool bInIsBlocking)
    {
        bIsBlocking = bInIsBlocking;
    }

    /** Look up a weapon/attack profile by gameplay tag. */
    UFUNCTION(BlueprintPure, Category = "Animation|Profiles")
    bool GetWeaponProfileByTag(const FGameplayTag& ProfileTag, FSFWeaponAnimationProfile& OutProfile) const;

    /** Apply a tagged override layer (e.g. special pose / layer ABP). */
    UFUNCTION(BlueprintCallable, Category = "Animation|OverrideLayer")
    bool ApplyOverrideLayer(const FGameplayTag& LayerTag, float BlendInTime);

    /** Remove the current override layer. */
    UFUNCTION(BlueprintCallable, Category = "Animation|OverrideLayer")
    void RemoveOverrideLayer(float BlendOutTime);

    UFUNCTION(BlueprintPure, Category = "Animation|OverrideLayer")
    bool HasOverrideLayer() const { return bHasOverrideLayer; }

    UFUNCTION(BlueprintPure, Category = "Animation|OverrideLayer")
    FGameplayTag GetOverrideLayerTag() const { return CurrentOverrideLayerTag; }

protected:
    void UpdateOwnerReferences();
    void ResetRuntimeData();
    void ResetAnimationProfile();
    void UpdateMovementData(float DeltaSeconds);
    void UpdateAnimationStateFromCharacter(float DeltaSeconds);

    UFUNCTION(BlueprintCallable, Category = "Animation")
    void SetAnimationProfile(const FSFWeaponAnimationProfile& InProfile, float DeltaSeconds);

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Tuning", meta = (ClampMin = "0.0"))
    float MoveThreshold = 3.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Tuning", meta = (ClampMin = "0.0"))
    float WalkSpeedThreshold = 200.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Tuning", meta = (ClampMin = "0.0"))
    float JogSpeedThreshold = 450.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Tuning", meta = (ClampMin = "0.0"))
    float OverlayBlendSpeed = 8.0f;

    /** Tagged weapon/ability animation profiles (Narrative-style anim sets). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Profiles", meta = (Categories = "SignalForge.Anim.Profile", ForceInlineRow))
    TMap<FGameplayTag, FSFWeaponAnimationProfile> TaggedWeaponProfiles;

    /** Tagged override layers that can temporarily replace/augment this ABP. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|OverrideLayer", meta = (Categories = "SignalForge.Anim.OverrideLayer", ForceInlineRow))
    TMap<FGameplayTag, TSubclassOf<UAnimInstance>> TaggedOverrideLayers;

    /** Tags automatically applied while this AnimInstance is active (optional, for GAS/gameplay). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Tags")
    FGameplayTagContainer ApplyTags;

private:
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|References", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<ASFCharacterBase> OwningCharacter = nullptr;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|References", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UCharacterMovementComponent> MovementComponent = nullptr;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Movement", meta = (AllowPrivateAccess = "true"))
    FVector WorldVelocity = FVector::ZeroVector;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Movement", meta = (AllowPrivateAccess = "true"))
    FVector LocalVelocity = FVector::ZeroVector;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Movement", meta = (AllowPrivateAccess = "true"))
    float GroundSpeed = 0.0f;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Movement", meta = (AllowPrivateAccess = "true"))
    float GroundSpeedNormalized = 0.0f;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Movement", meta = (AllowPrivateAccess = "true"))
    float VelocityZ = 0.0f;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Movement", meta = (AllowPrivateAccess = "true"))
    float Direction = 0.0f;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Movement", meta = (AllowPrivateAccess = "true"))
    float MovementInputAmount = 0.0f;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Movement", meta = (AllowPrivateAccess = "true"))
    float YawDeltaSpeed = 0.0f;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Movement", meta = (AllowPrivateAccess = "true"))
    bool bIsFalling = false;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Movement", meta = (AllowPrivateAccess = "true"))
    bool bIsAccelerating = false;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Movement", meta = (AllowPrivateAccess = "true"))
    bool bShouldMove = false;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Movement", meta = (AllowPrivateAccess = "true"))
    bool bHasMovementInput = false;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Movement", meta = (AllowPrivateAccess = "true"))
    FRotator AimRotation = FRotator::ZeroRotator;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Movement", meta = (AllowPrivateAccess = "true"))
    FRotator ActorRotation = FRotator::ZeroRotator;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|State", meta = (AllowPrivateAccess = "true"))
    ESFLocomotionState LocomotionState = ESFLocomotionState::Idle;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|State", meta = (AllowPrivateAccess = "true"))
    ESFOverlayMode OverlayMode = ESFOverlayMode::Unarmed;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|State", meta = (AllowPrivateAccess = "true"))
    ESFCombatMode CombatMode = ESFCombatMode::None;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|State", meta = (AllowPrivateAccess = "true"))
    bool bUseUpperBodyOverlay = true;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|State", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAnimSequenceBase> IdleOverride = nullptr;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Montages", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAnimMontage> LightAttackMontage = nullptr;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Montages", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAnimMontage> HeavyAttackMontage = nullptr;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Montages", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UAnimMontage> AbilityMontage = nullptr;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|State", meta = (AllowPrivateAccess = "true"))
    float UpperBodyOverlayWeight = 0.0f;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|State", meta = (AllowPrivateAccess = "true"))
    bool bIsBlocking = false;

    /** Simple override-layer state (no separate instance yet, but tag-driven). */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|OverrideLayer", meta = (AllowPrivateAccess = "true"))
    bool bHasOverrideLayer = false;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|OverrideLayer", meta = (AllowPrivateAccess = "true"))
    FGameplayTag CurrentOverrideLayerTag;

    float OverrideLayerBlendInTime = 0.0f;
    float OverrideLayerBlendOutTime = 0.0f;

    FRotator PreviousActorRotation = FRotator::ZeroRotator;
};