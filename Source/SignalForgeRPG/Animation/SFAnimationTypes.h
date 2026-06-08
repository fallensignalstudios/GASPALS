#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Core/SignalForgeTypes.h"
#include "SFAnimationTypes.generated.h"

UENUM(BlueprintType)
enum class ESFLocomotionState : uint8
{
	Idle	UMETA(DisplayName = "Idle"),
	Walk	UMETA(DisplayName = "Walk"),
	Jog		UMETA(DisplayName = "Jog"),
	Sprint	UMETA(DisplayName = "Sprint"),
	InAir	UMETA(DisplayName = "In Air")
};

/**
 * Agnostic locomotion gait. Defined here (rather than in SFCharacterBase.h)
 * because the anim instance needs to mirror it as a UPROPERTY and including
 * SFCharacterBase.h from SFAnimInstanceBase.h would be circular.
 *
 * Drives ASFCharacterBase::UpdateMovement_PreCMC and is mirrored into
 * USFAnimInstanceBase::Gait every tick so ABP_Biped can read it through the
 * anim instance without casting to any specific character BP.
 */
UENUM(BlueprintType)
enum class ESFGait : uint8
{
	Walk	UMETA(DisplayName = "Walk"),
	Run		UMETA(DisplayName = "Run"),
	Sprint	UMETA(DisplayName = "Sprint")
};

/**
 * Per-character movement tuning bundle consumed by
 * ASFCharacterBase::UpdateMovement_PreCMC. Replaces ad-hoc scalars on
 * individual character blueprints so NPC AI works with zero BP wiring.
 */
USTRUCT(BlueprintType)
struct FSFGaitSpeedProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gait", meta = (ClampMin = "0.0"))
	float WalkSpeed = 175.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gait", meta = (ClampMin = "0.0"))
	float RunSpeed = 375.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gait", meta = (ClampMin = "0.0"))
	float SprintSpeed = 650.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gait", meta = (ClampMin = "0.0"))
	float CrouchSpeed = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gait", meta = (ClampMin = "0.0"))
	float MaxAcceleration = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gait", meta = (ClampMin = "0.0"))
	float BrakingDeceleration = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gait", meta = (ClampMin = "0.0"))
	float GroundFriction = 5.f;
};

UENUM(BlueprintType)
enum class ESFOverlayMode : uint8
{
	Unarmed		UMETA(DisplayName = "Unarmed"),
	OneHanded	UMETA(DisplayName = "One Handed"),
	TwoHanded	UMETA(DisplayName = "Two Handed"),
	Rifle		UMETA(DisplayName = "Rifle"),
	Caster		UMETA(DisplayName = "Caster")
};

USTRUCT(BlueprintType)
struct FSFWeaponAnimationProfile
{
	GENERATED_BODY()

public:
	FSFWeaponAnimationProfile() = default;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	ESFOverlayMode OverlayMode = ESFOverlayMode::Unarmed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	ESFCombatMode CombatMode = ESFCombatMode::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimSequenceBase> IdleOverride = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages")
	TObjectPtr<UAnimMontage> LightAttackMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages")
	TObjectPtr<UAnimMontage> HeavyAttackMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages")
	TObjectPtr<UAnimMontage> AbilityMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages")
	TObjectPtr<UAnimMontage> EquipMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages")
	TObjectPtr<UAnimMontage> UnequipMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	bool bUseUpperBodyOverlay = true;
};