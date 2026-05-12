#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SFGameplayAbility.h"
#include "Combat/SFWeaponData.h"
#include "GameplayTagContainer.h"
#include "SFGameplayAbility_WeaponMelee.generated.h"

class ASFCharacterBase;
class ASFWeaponActor;
class UAnimMontage;
class UGameplayEffect;
class USFCombatComponent;
class USFEquipmentComponent;
class USFWeaponData;
struct FSFMeleeWeaponConfig;

/**
 * Melee swing ability for sword/axe/baton-style weapons.
 *
 * Granted via USFWeaponData::PrimaryFireAbility (light combo) and SecondaryFireAbility (heavy
 * combo). Designers point a melee weapon DA at this ability the same way a rifle DA points at
 * WeaponFire.
 *
 * Behaviour:
 *  - On activate: pick the next montage in the combo chain (light or heavy lane based on which
 *    input tag activated us), apply stamina cost via GE, set State.Weapon.MeleeSwinging.
 *  - Hit-detection runs via AnimNotifyState_MeleeWindow inside the montage, which calls into
 *    the existing USFCombatComponent. This ability just drives the lifecycle; it does NOT trace
 *    on its own (avoids duplicating combat-component logic).
 *  - On hit (CombatComponent OnHitDelivered): apply hit-react GE to victim, hitstop on instigator,
 *    fire MeleeHit cue.
 *  - On montage end (or interrupt): EndAbility. The combo step is committed to the weapon
 *    instance so it survives holster / swap.
 *  - Cancel window: while State.Weapon.MeleeCancelWindow is active, a new input press cancels
 *    the current montage and immediately plays the next combo step.
 *  - Paired weapons: alternates which weapon actor's mesh (mainhand vs offhand) hosts the
 *    socket sweep, updating LastSwingHandIndex on the instance each swing.
 *
 * Ranged-only behavior intentionally NOT supported here: no ammo consumption, no recoil, no
 * trace/projectile branches. Melee lives in its own ability so WeaponFire stays clean.
 */
UENUM(BlueprintType)
enum class ESFMeleeLane : uint8
{
	Light	UMETA(DisplayName = "Light"),
	Heavy	UMETA(DisplayName = "Heavy")
};

UCLASS()
class SIGNALFORGERPG_API USFGameplayAbility_WeaponMelee : public USFGameplayAbility
{
	GENERATED_BODY()

public:
	USFGameplayAbility_WeaponMelee();

	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	/**
	 * Which combo lane this ability represents. Set in the constructor of the BP subclass (or
	 * the C++ class) -- designers create two BP subclasses, one for light and one for heavy, and
	 * point PrimaryFireAbility / SecondaryFireAbility at them on the DA.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee")
	ESFMeleeLane Lane = ESFMeleeLane::Light;

	/** Stamina-cost GE applied on activation. Designer-authored; uses Data.StaminaCost SetByCaller. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee|Cost")
	TSubclassOf<UGameplayEffect> StaminaCostEffect;

	/** SetByCaller tag for the stamina-cost magnitude (matches your GE). Default: Data.StaminaCost. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee|Cost")
	FGameplayTag StaminaCostSetByCallerTag;

	/** Optional override damage. If positive, used instead of MeleeConfig per-step damages. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee|Damage")
	float FallbackDamage = 18.0f;

private:
	/** Cached state for the duration of a single swing. */
	FGameplayAbilitySpecHandle CachedHandle;
	const FGameplayAbilityActorInfo* CachedActorInfo = nullptr;
	FGameplayAbilityActivationInfo CachedActivationInfo;
	TWeakObjectPtr<ASFCharacterBase> CachedCharacter;
	TWeakObjectPtr<USFEquipmentComponent> CachedEquipment;
	TWeakObjectPtr<USFCombatComponent> CachedCombat;
	TWeakObjectPtr<UAnimMontage> CurrentMontage;
	int32 ChosenComboStep = 0;
	bool bHitLandedThisSwing = false;
	bool bAppliedSwingingTag = false;

	/** True if the current swing is being driven by the offhand weapon actor (paired weapons only). */
	bool bSwingingOffhand = false;

	/** Resolve weapon/equipment context and validate this is a melee weapon. */
	bool ResolveContext(
		ASFCharacterBase*& OutCharacter,
		USFEquipmentComponent*& OutEquipment,
		USFWeaponData*& OutWeaponData,
		USFCombatComponent*& OutCombat) const;

	/** Pick the montage for the current combo step. Returns nullptr (and logs) if config is empty. */
	UAnimMontage* PickMontageForStep(const FSFMeleeWeaponConfig& MeleeConfig, int32 Step) const;

	/** Apply stamina cost effect with magnitude pulled from FSFWeaponCombatTuning. */
	void ApplyStaminaCost(ASFCharacterBase* Character, const USFWeaponData* WeaponData);

	/** Determine which weapon actor swings (mainhand or offhand) and update the instance state. */
	ASFWeaponActor* ChooseSwingingWeaponActor(USFEquipmentComponent* Equipment, USFWeaponData* WeaponData);

	/** Subscribe to CombatComponent::OnHitDelivered so we can react to landed hits (cue, hitstop). */
	void BindHitDelegate(USFCombatComponent* Combat);
	void UnbindHitDelegate();

	UFUNCTION()
	void OnHitDelivered(const FSFHitData& HitData, const FSFResolvedHit& ResolvedHit);

	/** Bound to montage-end via PlayMontageAndWait-style flow; ends the ability cleanly. */
	UFUNCTION()
	void OnMontageEnded();
	UFUNCTION()
	void OnMontageCancelled();

	/** Fire the whiff cue if the swing closed without any landed hits. */
	void DispatchWhiffCueIfApplicable(ASFCharacterBase* Character);

	/** Commit the chosen combo step back to the weapon instance + advance for the next swing. */
	void CommitComboStepToInstance(USFEquipmentComponent* Equipment, ESFMeleeLane SwingLane, int32 Step);

	void FinishAbility(bool bWasCancelled);
};
