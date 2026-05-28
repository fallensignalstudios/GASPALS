#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "GameplayEffectTypes.h"
#include "GameplayAbilitySpecHandle.h"
#include "Animation/SFAnimInstanceBase.h"
#include "Combat/SFHitTypes.h"
#include "Combat/SFHitResolverInterface.h"
#include "Animation/SFAnimationTypes.h"
#include "Combat/SFWeaponData.h"
#include "SFCharacterBase.generated.h"

class USFAbilitySystemComponent;
class USFAttributeSetBase;
class USFCombatComponent;
class USFStatRegenComponent;
class USFProgressionComponent;
class USFEquipmentComponent;
class USFAmmoReserveComponent;
class USFInventoryComponent;
class USFFactionComponent;
class USFHitReactionComponent;
class UGameplayEffect;
class UGameplayAbility;
class UAnimMontage;
class USFInteractionComponent;
class UWidgetComponent;
class USFEnemyHealthBarWidget;
class ASFCharacterBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSFAttributeChangedSignature, float, NewValue, float, MaxValue);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnOverlayLinkedAnimLayerChangedSignature,
	TSubclassOf<UAnimInstance>, NewLayerClass,
	TSubclassOf<UAnimInstance>, PreviousLayerClass);

// Broadcast once at the top of HandleDeath(). Listeners receive the
// character that died and the character that dealt the killing blow
// (Killer may be null for environmental / scripted deaths).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnSFCharacterDiedSignature,
	ASFCharacterBase*, DeadCharacter,
	ASFCharacterBase*, Killer);

UCLASS()
class SIGNALFORGERPG_API ASFCharacterBase : public ACharacter, public IAbilitySystemInterface, public ISFHitResolverInterface, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	ASFCharacterBase();

public:
	FGameplayAbilitySpecHandle GrantCharacterAbility(
		TSubclassOf<UGameplayAbility> AbilityClass,
		int32 AbilityLevel = 1);

	// -------------------------------------------------------------------------
	// IAbilitySystemInterface
	// -------------------------------------------------------------------------

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// -------------------------------------------------------------------------
	// ISFHitResolverInterface
	// -------------------------------------------------------------------------

	virtual FSFResolvedHit ResolveIncomingHit_Implementation(const FSFHitData& HitData) override;

	// -------------------------------------------------------------------------
	// Accessors
	// -------------------------------------------------------------------------

	const USFAttributeSetBase* GetAttributeSet()              const { return AttributeSet; }
	USFAbilitySystemComponent* GetSFAbilitySystemComponent()  const { return AbilitySystemComponent; }
	USFCombatComponent* GetCombatComponent()                  const { return CombatComponent; }
	USFStatRegenComponent* GetStatRegenComponent()            const { return StatRegenComponent; }
	USFProgressionComponent* GetProgressionComponent()        const { return ProgressionComponent; }

	UFUNCTION(BlueprintPure, Category = "Components")
	USFEquipmentComponent* GetEquipmentComponent() const { return EquipmentComponent; }

	UFUNCTION(BlueprintPure, Category = "Components")
	USFAmmoReserveComponent* GetAmmoReserveComponent() const { return AmmoReserveComponent; }

	UFUNCTION(BlueprintPure, Category = "Components")
	USFInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(BlueprintPure, Category = "Components")
	USFFactionComponent* GetFactionComponent() const { return FactionComponent; }

	UFUNCTION(BlueprintPure, Category = "Components")
	USFHitReactionComponent* GetHitReactionComponent() const { return HitReactionComponent; }

	UFUNCTION(BlueprintPure, Category = "UI")
	UWidgetComponent* GetHealthBarWidgetComponent() const { return HealthBarWidget; }

	// -------------------------------------------------------------------------
	// IGenericTeamAgentInterface (delegates to FactionComponent)
	// -------------------------------------------------------------------------
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

	// -------------------------------------------------------------------------
	// XP / Death attribution (promoted from ASFEnemyCharacter so any character
	// with a hostile faction relationship can grant XP on kill).
	// -------------------------------------------------------------------------

	/** Record who last damaged us. Combat / projectile paths call this. */
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void SetLastDamagingCharacter(ASFCharacterBase* InCharacter);

	UFUNCTION(BlueprintPure, Category = "Combat")
	ASFCharacterBase* GetLastDamagingCharacter() const { return LastDamagingCharacter; }

	/**
	 * Virtual XP reward granted to the killer on death.
	 * Base class returns the enemy-leveled XP if a ProgressionComponent
	 * exists; subclasses (e.g. ASFEnemyCharacter, future Boss) can override.
	 */
	UFUNCTION(BlueprintPure, Category = "Progression")
	virtual int32 GetXPReward() const;

	TSubclassOf<UGameplayEffect> GetLightAttackDamageEffect() const { return LightAttackDamageEffect; }
	TSubclassOf<UGameplayEffect> GetHeavyAttackDamageEffect() const { return HeavyAttackDamageEffect; }

	// -------------------------------------------------------------------------
	// State queries
	// -------------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "State")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsSFCrouched() const { return bIsCrouched; }

	// -------------------------------------------------------------------------
	// Movement
	// -------------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float WalkSpeed = 450.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float SprintSpeed = 700.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
	float CrouchSpeed = 200.0f;

	UFUNCTION(BlueprintCallable, Category = "Movement")
	virtual void StartSprint();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	virtual void StopSprint();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	virtual void StartCrouch();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	virtual void StopCrouch();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	virtual void PerformDash(const FVector& DashDirection, float DashStrength);

	// -------------------------------------------------------------------------
	// Combat
	// -------------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void HandleLightAttackHitEvent();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void HandleHeavyAttackHitEvent();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void HandleHitReact();

	/** Called when Guard is reduced to 0. Override for custom guard-break behavior. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Combat")
	void HandleGuardBreak();
	virtual void HandleGuardBreak_Implementation();

	/** Called when Poise is reduced to 0. Override for custom stagger behavior. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Combat")
	void HandlePoiseBreak();
	virtual void HandlePoiseBreak_Implementation();

	// -------------------------------------------------------------------------
	// Death
	// -------------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death")
	bool bEnableRagdollOnDeath = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Death", meta = (ClampMin = "0.0"))
	float DeathCleanupDelay = 10.0f;

	UFUNCTION(BlueprintCallable, Category = "State")
	virtual void HandleDeath();

	UFUNCTION(BlueprintCallable, Category = "State")
	virtual void StartDeathCleanupTimer();

	/**
	 * Fires once at the top of HandleDeath(), before ragdoll / cleanup.
	 * Use this from loot droppers, quest objectives, kill-feed UI, etc.
	 * Single-fire: re-entry into HandleDeath after bIsDead is set is a
	 * no-op so this delegate cannot double-broadcast.
	 */
	UPROPERTY(BlueprintAssignable, Category = "State")
	FOnSFCharacterDiedSignature OnCharacterDied;

	// -------------------------------------------------------------------------
	// UI delegates
	// -------------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnSFAttributeChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnSFAttributeChangedSignature OnEchoChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnSFAttributeChangedSignature OnShieldsChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnSFAttributeChangedSignature OnStaminaChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnSFAttributeChangedSignature OnGuardChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnSFAttributeChangedSignature OnPoiseChanged;

	UFUNCTION(BlueprintPure, Category = "Animation|Weapon")
	ESFOverlayMode GetCurrentOverlayMode() const { return CurrentOverlayMode; }

	UFUNCTION(BlueprintPure, Category = "Animation|Weapon")
	ESFCombatMode GetCurrentCombatMode() const { return CurrentCombatMode; }

	UFUNCTION(BlueprintPure, Category = "Animation|Weapon")
	bool GetUseUpperBodyOverlay() const { return bUseUpperBodyOverlay; }

	/**
	 * Returns whether the character currently wants to aim down sights / hold
	 * an aim pose. Independent of whether an aim ability is actually active —
	 * this is the player/AI intent, not the gameplay state. Animation graphs
	 * read this (via Property Access on the AnimInstance mirror) to drive
	 * overlay blends, AO blendspaces, and pose selection.
	 */
	UFUNCTION(BlueprintPure, Category = "Animation|Locomotion")
	bool GetWantsToAim() const { return bWantsToAim; }

	/**
	 * Set the aim-intent flag. Safe to spam every frame from input or from
	 * a gameplay ability — the broadcast only fires on actual state change.
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Locomotion")
	void SetWantsToAim(bool bInWantsToAim);

	UFUNCTION(BlueprintPure, Category = "Animation|Locomotion")
	bool GetWantsToSprint() const { return bWantsToSprint; }

	UFUNCTION(BlueprintCallable, Category = "Animation|Locomotion")
	void SetWantsToSprint(bool bInWantsToSprint);

	/**
	 * Walk-intent: when true, GetDesiredGait will clamp the chosen gait to Walk
	 * even if full stick deflection would otherwise produce Run / Sprint. This
	 * is the GASP "Wants to Walk" channel.
	 */
	UFUNCTION(BlueprintPure, Category = "Animation|Locomotion")
	bool GetWantsToWalk() const { return bWantsToWalk; }

	UFUNCTION(BlueprintCallable, Category = "Animation|Locomotion")
	void SetWantsToWalk(bool bInWantsToWalk);

	/**
	 * Strafe-intent: when true, UpdateRotation_PreCMC switches CMC out of
	 * Orient-Rotation-To-Movement and into Use-Controller-Desired-Rotation, so
	 * the character holds the control yaw while side / back-stepping. Aim-down
	 * sights typically forces this on as well. This is the GASP "Wants to
	 * Strafe" channel.
	 */
	UFUNCTION(BlueprintPure, Category = "Animation|Locomotion")
	bool GetWantsToStrafe() const { return bWantsToStrafe; }

	UFUNCTION(BlueprintCallable, Category = "Animation|Locomotion")
	void SetWantsToStrafe(bool bInWantsToStrafe);

	/** Fires on actual aim-intent change (not every set). */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSFWantsToAimChangedSignature, bool, bNewWantsToAim);
	UPROPERTY(BlueprintAssignable, Category = "Animation|Locomotion")
	FOnSFWantsToAimChangedSignature OnWantsToAimChanged;

	/** Fires on actual sprint-intent change (not every set). */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSFWantsToSprintChangedSignature, bool, bNewWantsToSprint);
	UPROPERTY(BlueprintAssignable, Category = "Animation|Locomotion")
	FOnSFWantsToSprintChangedSignature OnWantsToSprintChanged;

	/** Fires on actual walk-intent change (not every set). */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSFWantsToWalkChangedSignature, bool, bNewWantsToWalk);
	UPROPERTY(BlueprintAssignable, Category = "Animation|Locomotion")
	FOnSFWantsToWalkChangedSignature OnWantsToWalkChanged;

	/** Fires on actual strafe-intent change (not every set). */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSFWantsToStrafeChangedSignature, bool, bNewWantsToStrafe);
	UPROPERTY(BlueprintAssignable, Category = "Animation|Locomotion")
	FOnSFWantsToStrafeChangedSignature OnWantsToStrafeChanged;

	/**
	 * Single chokepoint that fires after ANY of the four intent flags changes.
	 * BP subclasses (e.g. CBP_SandboxCharacter) override this to push the C++
	 * intent state into their BP "Character Input State" struct so the existing
	 * GASP BP functions (UpdateRotation_PreCMC, GetDesiredGait, CanSprint,
	 * CalculateMaxSpeed) continue to read a single source of truth.
	 *
	 * BlueprintNativeEvent so C++ subclasses can also implement it for AI
	 * controllers or replicated companions.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Animation|Locomotion")
	void OnLocomotionIntentChanged();
	virtual void OnLocomotionIntentChanged_Implementation() {}

	UFUNCTION(BlueprintPure, Category = "Animation|Weapon")
	const FSFWeaponAnimationProfile& GetCurrentWeaponAnimationProfile() const
	{
		return CurrentWeaponAnimationProfile;
	}

	UFUNCTION(BlueprintPure, Category = "Animation|Weapon")
	bool HasWeaponAnimationProfile() const { return bHasWeaponAnimationProfile; }

	UFUNCTION(BlueprintPure, Category = "Animation|State")
	bool IsAnimBlocking() const { return bAnimIsBlocking; }

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Animation|LookAt")
	FVector GetHeadLookAtLocation(bool& bOutWantsLookAt) const;

	/** Called by equipment when the active weapon changes. */
	void ApplyWeaponAnimationFromData(const USFWeaponData* WeaponData);
	void ClearWeaponAnimationProfile();

protected:
	/**
	 * Performs the actual LinkAnimClassLayers / UnlinkAnimClassLayers calls
	 * on the main mesh, broadcasts OnOverlayLinkedAnimLayerChanged, and
	 * updates CurrentOverlayLinkedAnimLayerClass. Safe to call with nullptr
	 * to clear.
	 */
	void SetOverlayLinkedAnimLayer(TSubclassOf<UAnimInstance> NewLayerClass);

public:

	UFUNCTION(BlueprintPure, Category = "Interaction")
	USFInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }

	UFUNCTION(BlueprintPure, Category = "Animation|Weapon")
	TSubclassOf<UAnimInstance> GetCurrentOverlayLinkedAnimLayerClass() const
	{
		return CurrentOverlayLinkedAnimLayerClass;
	}

	/**
	 * Form tag used to pick a per-form overlay layer from
	 * USFWeaponData::FormSpecificOverlayLayers. Default returns an invalid tag,
	 * which means "use the weapon's base OverlayLinkedAnimLayerClass".
	 * Override in subclasses (player, companion, enemy variants) to opt into
	 * form-specific layers.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Animation|Weapon")
	FGameplayTag GetCharacterFormTag() const;

	/**
	 * Re-applies the currently equipped weapon's overlay anim layer to the
	 * mesh. Useful after a form/stance change that affects which form-specific
	 * layer should be active.
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Weapon")
	void RefreshOverlayLinkedAnimLayer();

	/** Fires whenever the linked overlay layer class changes (link or unlink). */
	UPROPERTY(BlueprintAssignable, Category = "Animation|Weapon")
	FOnOverlayLinkedAnimLayerChangedSignature OnOverlayLinkedAnimLayerChanged;

	/**
	 * Anim Blueprint applied to the main mesh on PostInitializeComponents.
	 * Centralizes AnimClass assignment so subclass BPs don't each have to
	 * configure it on their Mesh component. Set this once on a base
	 * archetype BP (typically BP_SFCharacterBase) to your biped ABP
	 * (e.g. ABP_Biped); every subclass inherits the value. Override per
	 * subclass only when a character uses a different rig family
	 * (quadruped, drone, etc.).
	 *
	 * If the mesh's AnimClass is already set to a *different* non-null class
	 * at PostInitializeComponents time, the existing assignment wins — so
	 * BPs that intentionally want a custom ABP still work without code
	 * changes.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TSubclassOf<UAnimInstance> DefaultAnimClass;

	UFUNCTION(BlueprintPure, Category = "Animation|Weapon|Sequences")
	UAnimSequenceBase* GetCurrentIdleOverlaySequence() const
	{
		return CurrentIdleOverlaySequence;
	}

	UFUNCTION(BlueprintPure, Category = "Animation|Weapon|Sequences")
	UAnimSequenceBase* GetCurrentWalkOverlaySequence() const
	{
		return CurrentWalkOverlaySequence;
	}

	UFUNCTION(BlueprintPure, Category = "Animation|Weapon|Sequences")
	UAnimSequenceBase* GetCurrentSprintOverlaySequence() const
	{
		return CurrentSprintOverlaySequence;
	}

	UFUNCTION(BlueprintPure, Category = "Animation|Weapon|Sequences")
	bool HasValidIdleOverlaySequence() const
	{
		return CurrentIdleOverlaySequence != nullptr;
	}

	UFUNCTION(BlueprintNativeEvent, Category = "Animation|Weapon")
	void OnWeaponEquipped(const USFWeaponData* WeaponData);

	/**
	 * Override APawn::GetActorEyesViewPoint so weapon abilities (hitscan & projectile) aim correctly
	 * for AI-controlled pawns. The default Pawn implementation returns GetControlRotation(), which on
	 * an AIController only updates if something explicitly drives it (SetFocus / SetControlRotation).
	 * For AI we instead derive the rotation from the controller's focal point if set, then fall back
	 * to the actor's facing yaw so aim never points at the stale spawn rotation.
	 */
	virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const override;

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// -------------------------------------------------------------------------
	// GAS setup helpers
	// -------------------------------------------------------------------------

	void InitializeDefaultAttributes();
	void GrantStartupAbilities();
	void BindAttributeDelegates();
	void BroadcastInitialAttributeValues();

	// -------------------------------------------------------------------------
	// Attribute change handlers
	// -------------------------------------------------------------------------

	virtual void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);
	virtual void HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData);
	virtual void HandleEchoChanged(const FOnAttributeChangeData& ChangeData);
	virtual void HandleMaxEchoChanged(const FOnAttributeChangeData& ChangeData);
	virtual void HandleShieldsChanged(const FOnAttributeChangeData& ChangeData);
	virtual void HandleMaxShieldsChanged(const FOnAttributeChangeData& ChangeData);
	virtual void HandleStaminaChanged(const FOnAttributeChangeData& ChangeData);
	virtual void HandleMaxStaminaChanged(const FOnAttributeChangeData& ChangeData);

	virtual void HandleGuardChanged(const FOnAttributeChangeData& ChangeData);
	virtual void HandleMaxGuardChanged(const FOnAttributeChangeData& ChangeData);
	virtual void HandlePoiseChanged(const FOnAttributeChangeData& ChangeData);

	void HandleBlockTagChanged(const FGameplayTag Tag, int32 NewCount);

	/** Forwards blocking state into the AnimInstance. */
	void OnBlockTagChanged(bool bHasBlockTag);

	// If we want block state broadly available to anim logic:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|State")
	bool bAnimIsBlocking = false;

protected:
	// -------------------------------------------------------------------------
	// Components
	// -------------------------------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<USFAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<USFAttributeSetBase> AttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USFStatRegenComponent> StatRegenComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USFProgressionComponent> ProgressionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<USFCombatComponent> CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USFEquipmentComponent> EquipmentComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USFAmmoReserveComponent> AmmoReserveComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USFInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USFInteractionComponent> InteractionComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USFFactionComponent> FactionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USFHitReactionComponent> HitReactionComponent;

	// -------------------------------------------------------------------------
	// Floating combat HUD (name + level + shields + health) above the head.
	// Screen-space UWidgetComponent that hosts USFEnemyHealthBarWidget. Designers
	// override HealthBarWidgetClass on the BP defaults to swap visuals; leave it
	// unset to keep the slot dormant (e.g. for the player pawn).
	// -------------------------------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UWidgetComponent> HealthBarWidget;

	/** Widget class to instance into HealthBarWidget at runtime. Set on BP defaults to WBP_EnemyHealthBar. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<USFEnemyHealthBarWidget> HealthBarWidgetClass;

	/** Offset above the actor origin where the bar should sit. Tune per archetype height. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	FVector HealthBarOffset = FVector(0.0f, 0.0f, 120.0f);

	/** Last character that damaged us; used for XP attribution on death. */
	UPROPERTY(Transient)
	TObjectPtr<ASFCharacterBase> LastDamagingCharacter;

	// -------------------------------------------------------------------------
	// GAS data
	// -------------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TSubclassOf<UGameplayEffect> DefaultPrimaryAttributesEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TSubclassOf<UGameplayEffect> LightAttackDamageEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TSubclassOf<UGameplayEffect> HeavyAttackDamageEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UAnimMontage> HitReactMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UAnimMontage> GuardBreakMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UAnimMontage> PoiseBreakMontage;

	// -------------------------------------------------------------------------
	// State
	// -------------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDead = false;

	private:

		// Animation-facing weapon state shared by all characters
		UPROPERTY(Transient)
		ESFOverlayMode CurrentOverlayMode = ESFOverlayMode::Unarmed;

		UPROPERTY(Transient)
		ESFCombatMode CurrentCombatMode = ESFCombatMode::None;

		UPROPERTY(Transient)
		bool bUseUpperBodyOverlay = true;

		/**
		 * Player/AI intent flags. BlueprintReadOnly so Property Access in the
		 * AnimGraph can read them off the character directly (thread-safe),
		 * while SetWantsToAim / SetWantsToSprint own the write path on the
		 * game thread and fire delegates on transition.
		 */
		UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (AllowPrivateAccess = "true"))
		bool bWantsToAim = false;

		UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (AllowPrivateAccess = "true"))
		bool bWantsToSprint = false;

		UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (AllowPrivateAccess = "true"))
		bool bWantsToWalk = false;

		UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|Locomotion", meta = (AllowPrivateAccess = "true"))
		bool bWantsToStrafe = false;

		UPROPERTY(Transient)
		FSFWeaponAnimationProfile CurrentWeaponAnimationProfile;

		UPROPERTY(Transient)
		bool bHasWeaponAnimationProfile = false;

		UPROPERTY(Transient)
		TSubclassOf<UAnimInstance> CurrentOverlayLinkedAnimLayerClass = nullptr;

		// Cached animation sequences resolved from WeaponAnimationSet
		UPROPERTY(Transient)
		TObjectPtr<UAnimSequenceBase> CurrentIdleOverlaySequence = nullptr;

		UPROPERTY(Transient)
		TObjectPtr<UAnimSequenceBase> CurrentWalkOverlaySequence = nullptr;

		UPROPERTY(Transient)
		TObjectPtr<UAnimSequenceBase> CurrentSprintOverlaySequence = nullptr;
};