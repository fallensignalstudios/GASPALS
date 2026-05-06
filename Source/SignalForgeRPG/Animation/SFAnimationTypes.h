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