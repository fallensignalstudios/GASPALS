#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Animation/SFAnimationTypes.h"
#include "SFWeaponAnimationSet.generated.h"

class UAnimSequenceBase;
class UAnimMontage;

/**
 * Data asset holding the animation sequences for a weapon's overlay layer.
 * Designed to be read at runtime by the ABP's Sequence Player / Play Animation nodes.
 */
UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFWeaponAnimationSet : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Which overlay mode this set corresponds to. Used for validation and ABP dispatch. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Identity")
	ESFOverlayMode OverlayMode = ESFOverlayMode::Unarmed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Identity")
	TSoftObjectPtr<UAnimSequenceBase> IdleSequenceBaseRaw;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Identity")
	TSoftObjectPtr<UAnimSequenceBase> WalkSequenceBaseRaw;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Identity")
	TSoftObjectPtr<UAnimSequenceBase> SprintSequenceBaseRaw;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Identity")
	TSoftObjectPtr<UAnimSequenceBase> StrafeSequenceBaseRaw;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Identity")
	TSoftObjectPtr<UAnimSequenceBase> TurnInPlaceLeftSequenceBaseRaw;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Identity")
	TSoftObjectPtr<UAnimSequenceBase> TurnInPlaceRightSequenceBaseRaw;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Identity")
	TSoftObjectPtr<UAnimSequenceBase> JumpSequenceBaseRaw;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Identity")
	TSoftObjectPtr<UAnimSequenceBase> LandSequenceBaseRaw;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Identity")
	TSoftObjectPtr<UAnimSequenceBase> DeathForwardSequenceBaseRaw;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Identity")
	TSoftObjectPtr<UAnimSequenceBase> DeathBackwardSequenceBaseRaw;

	/** Runtime-cached references. Resolved in ApplyWeaponAnimationFromData. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Cached")
	TObjectPtr<UAnimSequenceBase> IdleSequenceBase = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Cached")
	TObjectPtr<UAnimSequenceBase> WalkSequenceBase = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Cached")
	TObjectPtr<UAnimSequenceBase> SprintSequenceBase = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Cached")
	TObjectPtr<UAnimSequenceBase> StrafeSequenceBase = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Cached")
	TObjectPtr<UAnimSequenceBase> TurnInPlaceLeftSequenceBase = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Cached")
	TObjectPtr<UAnimSequenceBase> TurnInPlaceRightSequenceBase = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Cached")
	TObjectPtr<UAnimSequenceBase> JumpSequenceBase = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Cached")
	TObjectPtr<UAnimSequenceBase> LandSequenceBase = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Cached")
	TObjectPtr<UAnimSequenceBase> DeathForwardSequenceBase = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Cached")
	TObjectPtr<UAnimSequenceBase> DeathBackwardSequenceBase = nullptr;

public:
	/**
	 * Synchronously resolves all soft references into runtime pointers.
	 *
	 * At runtime, TSoftObjectPtr is just internally a string. This synchronously
	 * resolves most pointers so that the abp does not have to store raw pointers.
	 * It does not currently worry about async, but will load in parallel on a
	 * streaming thread, so the caller may want
	 * to wait/canvas tick after calling this.
	 */
	void LoadAnimationSetContentSynchronously();

	UFUNCTION(BlueprintCallable, Category = "Animation|Cache")
	void LoadContentSynchronously()
	{
		LoadAnimationSetContentSynchronously();
	}

	/** Returns the idle overlay sequence, or null if not set. */
	UFUNCTION(BlueprintPure, Category = "Animation|Access")
	UAnimSequenceBase* GetIdleOverlaySequence() const
	{
		return IdleSequenceBase;
	}

	/** Returns the walk overlay sequence, or null if not set. */
	UFUNCTION(BlueprintPure, Category = "Animation|Access")
	UAnimSequenceBase* GetWalkOverlaySequence() const
	{
		return WalkSequenceBase;
	}

	/** Returns the sprint overlay sequence, or null if not set. */
	UFUNCTION(BlueprintPure, Category = "Animation|Access")
	UAnimSequenceBase* GetSprintOverlaySequence() const
	{
		return SprintSequenceBase;
	}
};