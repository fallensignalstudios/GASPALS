#include "Characters/SFCharacterBase.h"

#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/SFGameplayAbility.h"
#include "Abilities/GameplayAbility.h"
#include "AIController.h"
#include "GameFramework/PlayerController.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "Animation/AnimInstance.h"
#include "Animation/SFAnimInstanceBase.h"
#include "Engine/SkeletalMesh.h"
#include "Combat/SFCombatComponent.h"
#include "Combat/SFHitResolverInterface.h"
#include "Components/CapsuleComponent.h"
#include "Combat/SFWeaponActor.h"
#include "Components/SFEquipmentComponent.h"
#include "Components/SFAmmoReserveComponent.h"
#include "Components/SFInventoryComponent.h"
#include "Components/SFProgressionComponent.h"
#include "Components/SFStatRegenComponent.h"
#include "Combat/SFHitReactionComponent.h"
#include "Faction/SFFactionComponent.h"
#include "Faction/SFFactionStatics.h"
#include "Core/SFAttributeSetBase.h"
#include "Core/SignalForgeGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "Combat/SFWeaponData.h"
#include "Components/SFInteractionComponent.h"
#include "Components/WidgetComponent.h"
#include "Combat/SFWeaponAnimationSet.h"
#include "UI/SFEnemyHealthBarWidget.h"
#include "Core/SignalForgeLogChannels.h"

// -----------------------------------------------------------------------------
// Construction
// -----------------------------------------------------------------------------

ASFCharacterBase::ASFCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	AbilitySystemComponent = CreateDefaultSubobject<USFAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	StatRegenComponent = CreateDefaultSubobject<USFStatRegenComponent>(TEXT("StatRegenComponent"));
	AttributeSet = CreateDefaultSubobject<USFAttributeSetBase>(TEXT("AttributeSet"));
	CombatComponent = CreateDefaultSubobject<USFCombatComponent>(TEXT("CombatComponent"));
	ProgressionComponent = CreateDefaultSubobject<USFProgressionComponent>(TEXT("ProgressionComponent"));
	EquipmentComponent = CreateDefaultSubobject<USFEquipmentComponent>(TEXT("EquipmentComponent"));
	AmmoReserveComponent = CreateDefaultSubobject<USFAmmoReserveComponent>(TEXT("AmmoReserveComponent"));
	InventoryComponent = CreateDefaultSubobject<USFInventoryComponent>(TEXT("InventoryComponent"));
	InteractionComponent = CreateDefaultSubobject<USFInteractionComponent>(TEXT("InteractionComponent"));
	FactionComponent = CreateDefaultSubobject<USFFactionComponent>(TEXT("FactionComponent"));
	HitReactionComponent = CreateDefaultSubobject<USFHitReactionComponent>(TEXT("HitReactionComponent"));

	// Floating world health/shield bar slot. Lives on the base class so every
	// character archetype (player, NPC, enemy, companion) can host one — leave
	// HealthBarWidgetClass unset on a BP to keep the slot dormant. Screen-space
	// so the bar always faces the camera and stays a consistent size. Designers
	// pick the widget class and per-archetype Z offset on BP defaults.
	HealthBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidget"));
	HealthBarWidget->SetupAttachment(GetRootComponent());
	HealthBarWidget->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarWidget->SetDrawAtDesiredSize(true);
	HealthBarWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HealthBarWidget->SetGenerateOverlapEvents(false);
	HealthBarWidget->SetRelativeLocation(HealthBarOffset);
	// Hide until BeginPlay; we don't want a ghost bar on a freshly-spawned
	// character that has not taken damage yet. The inner widget also self-hides
	// on NativeConstruct.
	HealthBarWidget->SetVisibility(false);

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = WalkSpeed;
		MoveComp->MaxWalkSpeedCrouched = CrouchSpeed;
		MoveComp->GetNavAgentPropertiesRef().bCanCrouch = true;
	}

	// Ensure the capsule blocks the Visibility trace channel so the
	// InteractionComponent's sphere sweep can hit characters. The default Pawn
	// collision profile in many UE5 projects sets Visibility = Ignore, which
	// silently breaks line-of-sight interaction. We override per-channel
	// without changing the profile so other code (camera occlusion etc.) is
	// unaffected.
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}
}

// -----------------------------------------------------------------------------
// View point
// -----------------------------------------------------------------------------

void ASFCharacterBase::GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	AController* OwningController = GetController();

	// Player path: use the PlayerController's view point (the gameplay camera location +
	// control rotation). This is the correct anchor for hitscan / projectile aim in
	// third-person \u2014 BaseEyeHeight + control rotation would trace from the character's
	// head, not from where the player actually sees the crosshair, and the offset between
	// head and SpringArm camera is enough to make bullets visually "fly past" targets at
	// any distance. We still mirror APawn::GetActorEyesViewPoint when the controller can't
	// supply a view point.
	if (APlayerController* PC = Cast<APlayerController>(OwningController))
	{
		PC->GetPlayerViewPoint(OutLocation, OutRotation);
		return;
	}

	// Default eye location for AI / unpossessed pawns: actor origin + BaseEyeHeight.
	OutLocation = GetPawnViewLocation();

	if (!OwningController)
	{
		OutRotation = GetViewRotation();
		return;
	}

	// AI path: prefer the controller's focal point if one is set (SetFocus / SetFocalPoint),
	// otherwise use the actor's facing yaw so aim follows the body rotation that SFBTTask_FaceTarget drives.
	if (const AAIController* AI = Cast<AAIController>(OwningController))
	{
		const FVector Focal = AI->GetFocalPoint();
		if (!Focal.IsNearlyZero())
		{
			const FVector ToFocal = Focal - OutLocation;
			if (!ToFocal.IsNearlyZero())
			{
				OutRotation = ToFocal.Rotation();
				return;
			}
		}
	}

	// Fallback: use the controller's current ControlRotation (which SFBTTask_FaceTarget now drives),
	// or the actor rotation if for some reason that's stale.
	const FRotator ControlRot = OwningController->GetControlRotation();
	OutRotation = ControlRot.IsNearlyZero() ? GetActorRotation() : ControlRot;
}

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

void ASFCharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Apply DefaultAnimClass centrally so subclass BPs don't each have to
	// configure AnimClass on their Mesh component. The base archetype BP
	// sets DefaultAnimClass = ABP_Biped once and every subclass inherits.
	//
	// If a subclass BP intentionally set a *different* non-null AnimClass on
	// its Mesh component, that authored value wins — we only stamp our class
	// when the mesh is currently unset or already matches.
	if (DefaultAnimClass && GetMesh())
	{
		USkeletalMeshComponent* MeshComp = GetMesh();
		const UClass* CurrentAnimClass = MeshComp->GetAnimClass();
		if (CurrentAnimClass == nullptr || CurrentAnimClass == *DefaultAnimClass)
		{
			MeshComp->SetAnimInstanceClass(DefaultAnimClass);
		}

#if !UE_BUILD_SHIPPING
		// Skeleton-mismatch diagnostic. UE silently no-ops when an ABP is bound
		// to a mesh on the wrong skeleton (the character plays the ref pose
		// with no editor warning), so log loudly when we detect the mismatch.
		// This catches the most common cause of "ABP works on Sandbox but not
		// on my new character" — the new character's mesh was assigned a
		// different USkeleton than ABP_Biped was compiled against.
		if (const USkeletalMesh* SkelMesh = MeshComp->GetSkeletalMeshAsset())
		{
			if (const UAnimBlueprintGeneratedClass* AnimBPClass =
				Cast<UAnimBlueprintGeneratedClass>(DefaultAnimClass.Get()))
			{
				const USkeleton* MeshSkel = SkelMesh->GetSkeleton();
				const USkeleton* AnimBPSkel = AnimBPClass->TargetSkeleton;
				if (MeshSkel && AnimBPSkel && MeshSkel != AnimBPSkel)
				{
					UE_LOG(LogSFCharacter, Warning,
						TEXT("[%s] DefaultAnimClass %s expects skeleton %s but mesh ")
						TEXT("uses %s — ABP will not run. Reassign the mesh to the ")
						TEXT("correct skeleton or set a different DefaultAnimClass."),
						*GetName(),
						*DefaultAnimClass->GetName(),
						*AnimBPSkel->GetName(),
						*MeshSkel->GetName());
				}
			}
		}
#endif
	}
}

void ASFCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	InitializeDefaultAttributes();

	if (ProgressionComponent)
	{
		ProgressionComponent->ApplyLevelStats(true);
	}

	GrantStartupAbilities();
	BindAttributeDelegates();

	// Wire the floating health/shield bar BEFORE BroadcastInitialAttributeValues so
	// the widget's delegate bindings hook up in time to consume the priming broadcast.
	// Designers set HealthBarWidgetClass on the BP defaults to WBP_EnemyHealthBar (or
	// a per-archetype variant). If no class is set we leave the slot dormant — some
	// characters (the player pawn, vendors, story NPCs) intentionally don't want a
	// combat bar floating above their head.
	if (HealthBarWidget)
	{
		HealthBarWidget->SetRelativeLocation(HealthBarOffset);
		if (HealthBarWidgetClass)
		{
			HealthBarWidget->SetWidgetClass(HealthBarWidgetClass);
			HealthBarWidget->SetVisibility(true);
			// Force the widget to instantiate now so its delegate bindings hook up
			// before the first damage hit — otherwise UWidgetComponent lazily creates
			// the widget on first render, which can drop the first damage event.
			HealthBarWidget->InitWidget();

			// In UE 5.7 the widget's Outer is not always the owning UWidgetComponent
			// (it can be a transient package), so GetTypedOuter<UWidgetComponent>()
			// inside the widget returns null and the widget can't discover its host
			// pawn. Push the target character explicitly so binding is deterministic.
			if (USFEnemyHealthBarWidget* HealthBarUserWidget = Cast<USFEnemyHealthBarWidget>(HealthBarWidget->GetUserWidgetObject()))
			{
				HealthBarUserWidget->InitializeForCharacter(this);
			}

			UE_LOG(LogSFCharacter, Log,
				TEXT("HealthBar wired: actor=%s class=%s widget=%s"),
				*GetName(),
				*GetNameSafe(HealthBarWidgetClass),
				*GetNameSafe(HealthBarWidget->GetUserWidgetObject()));
		}
		else
		{
			UE_LOG(LogSFCharacter, Verbose,
				TEXT("HealthBar dormant on %s (HealthBarWidgetClass not set)"), *GetName());
		}
	}

	BroadcastInitialAttributeValues();

	if (AbilitySystemComponent)
	{
		const FSignalForgeGameplayTags& GameplayTags = FSignalForgeGameplayTags::Get();
		AbilitySystemComponent
			->RegisterGameplayTagEvent(GameplayTags.State_Blocking, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &ASFCharacterBase::HandleBlockTagChanged);
	}
}

void ASFCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Drain the ASC input queue every frame for both players and AI-controlled NPCs.
	// Without this, BT tasks (or any caller) that push presses via AbilityInputTagPressed
	// would enqueue them forever because TryActivateAbility is only invoked from
	// ProcessAbilityInput. The player previously did this in its own Tick; doing it
	// here means NPCs get the same treatment so AI fire / melee / ability presses
	// actually drive ability activation.
	if (AbilitySystemComponent)
	{
		const bool bGamePaused = GetWorld() && GetWorld()->IsPaused();
		AbilitySystemComponent->ProcessAbilityInput(DeltaTime, bGamePaused);
	}

	// Per-tick locomotion brain. Runs BEFORE the CharacterMovementComponent
	// ticks (Character's Tick runs ahead of CMC's component tick by default)
	// so any speed / accel / friction change lands in this frame's movement.
	// BlueprintNativeEvent: player BP overrides to keep its GASP curve graph;
	// NPCs use the C++ default in UpdateMovement_PreCMC_Implementation.
	UpdateMovement_PreCMC(DeltaTime);
}

void ASFCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// One-shot diagnostic: prints which controller class actually possessed this
	// pawn so we can tell at a glance whether a BP override has redirected
	// AIControllerClass away from the intended SF controller.
	UE_LOG(LogSFCharacter, Log,
		TEXT("[SFCharacter] PossessedBy: pawn='%s' (class '%s') controller='%s' (class '%s')"),
		*GetName(),
		*GetClass()->GetName(),
		NewController ? *NewController->GetName() : TEXT("<null>"),
		NewController ? *NewController->GetClass()->GetName() : TEXT("<null>"));

	// Make our Tick run AFTER the controller's tick so the ASC input drain in
	// ASFCharacterBase::Tick observes any input pressed/released the BehaviorTree
	// queued this frame. Without this, AIControllers (which own the BTComponent)
	// may tick AFTER their pawn in the same frame -- meaning the BT FireWeapon
	// task pushes a press/release pair into InputPressedSpecHandles, and the
	// character's next-frame drain sees an empty queue because the press got
	// queued AFTER the drain ran. PlayerControllers don't hit this because their
	// input bindings naturally establish controller-before-pawn ordering.
	if (NewController)
	{
		AddTickPrerequisiteActor(NewController);
	}
}

void ASFCharacterBase::UnPossessed()
{
	// Symmetric cleanup so a possessed-then-unpossessed pawn doesn't keep an old
	// prerequisite around if a new controller takes over later.
	if (AController* OldController = GetController())
	{
		RemoveTickPrerequisiteActor(OldController);
	}

	Super::UnPossessed();
}

// -----------------------------------------------------------------------------
// IAbilitySystemInterface
// -----------------------------------------------------------------------------

UAbilitySystemComponent* ASFCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// -----------------------------------------------------------------------------
// GAS setup
// -----------------------------------------------------------------------------

void ASFCharacterBase::InitializeDefaultAttributes()
{
	if (!AbilitySystemComponent || !DefaultPrimaryAttributesEffect)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	const FGameplayEffectSpecHandle EffectSpecHandle =
		AbilitySystemComponent->MakeOutgoingSpec(DefaultPrimaryAttributesEffect, 1.0f, EffectContext);

	if (EffectSpecHandle.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
	}
}

void ASFCharacterBase::GrantStartupAbilities()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : StartupAbilities)
	{
		if (!AbilityClass)
		{
			continue;
		}

		// Build spec at level 1; progression can drive level later.
		FGameplayAbilitySpec AbilitySpec(AbilityClass, 1);

		// Stamp the ability's input tag onto the dynamic spec tags so the ASC
		// input routing can match by tag without needing a separate lookup.
		if (const USFGameplayAbility* AbilityCDO = AbilityClass->GetDefaultObject<USFGameplayAbility>())
		{
			const FGameplayTag InputTag = AbilityCDO->GetInputTag();
			if (InputTag.IsValid())
			{
				AbilitySpec.GetDynamicSpecSourceTags().AddTag(InputTag);
			}
		}

		AbilitySystemComponent->GiveAbility(AbilitySpec);
	}
}

void ASFCharacterBase::BindAttributeDelegates()
{
	if (!AbilitySystemComponent || !AttributeSet)
	{
		return;
	}

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute())
		.AddUObject(this, &ASFCharacterBase::HandleHealthChanged);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMaxHealthAttribute())
		.AddUObject(this, &ASFCharacterBase::HandleMaxHealthChanged);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetEchoAttribute())
		.AddUObject(this, &ASFCharacterBase::HandleEchoChanged);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMaxEchoAttribute())
		.AddUObject(this, &ASFCharacterBase::HandleMaxEchoChanged);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetShieldsAttribute())
		.AddUObject(this, &ASFCharacterBase::HandleShieldsChanged);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMaxShieldsAttribute())
		.AddUObject(this, &ASFCharacterBase::HandleMaxShieldsChanged);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetStaminaAttribute())
		.AddUObject(this, &ASFCharacterBase::HandleStaminaChanged);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMaxStaminaAttribute())
		.AddUObject(this, &ASFCharacterBase::HandleMaxStaminaChanged);

	// Guard / MaxGuard / Poise
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetGuardAttribute())
		.AddUObject(this, &ASFCharacterBase::HandleGuardChanged);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMaxGuardAttribute())
		.AddUObject(this, &ASFCharacterBase::HandleMaxGuardChanged);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetPoiseAttribute())
		.AddUObject(this, &ASFCharacterBase::HandlePoiseChanged);
}

void ASFCharacterBase::BroadcastInitialAttributeValues()
{
	if (!AttributeSet)
	{
		return;
	}

	OnHealthChanged.Broadcast(AttributeSet->GetHealth(), AttributeSet->GetMaxHealth());
	OnEchoChanged.Broadcast(AttributeSet->GetEcho(), AttributeSet->GetMaxEcho());
	OnShieldsChanged.Broadcast(AttributeSet->GetShields(), AttributeSet->GetMaxShields());
	OnStaminaChanged.Broadcast(AttributeSet->GetStamina(), AttributeSet->GetMaxStamina());

	// New: initial Guard / Poise to UI
	OnGuardChanged.Broadcast(AttributeSet->GetGuard(), AttributeSet->GetMaxGuard());
	OnPoiseChanged.Broadcast(AttributeSet->GetPoise(), AttributeSet->GetPoise()); // Poise has no MaxPoise yet
}

// -----------------------------------------------------------------------------
// Attribute change handlers
// -----------------------------------------------------------------------------

void ASFCharacterBase::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	if (!AttributeSet) { return; }
	UE_LOG(LogSFCharacter, Log,
		TEXT("HandleHealthChanged actor=%s OldValue=%.2f NewValue=%.2f MaxHealth=%.2f"),
		*GetName(), ChangeData.OldValue, ChangeData.NewValue, AttributeSet->GetMaxHealth());
	OnHealthChanged.Broadcast(ChangeData.NewValue, AttributeSet->GetMaxHealth());
}

void ASFCharacterBase::HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	if (!AttributeSet) { return; }
	UE_LOG(LogSFCharacter, Log,
		TEXT("HandleMaxHealthChanged actor=%s OldMax=%.2f NewMax=%.2f Health=%.2f"),
		*GetName(), ChangeData.OldValue, ChangeData.NewValue, AttributeSet->GetHealth());
	OnHealthChanged.Broadcast(AttributeSet->GetHealth(), ChangeData.NewValue);
}

void ASFCharacterBase::HandleEchoChanged(const FOnAttributeChangeData& ChangeData)
{
	if (!AttributeSet) { return; }
	OnEchoChanged.Broadcast(ChangeData.NewValue, AttributeSet->GetMaxEcho());
}

void ASFCharacterBase::HandleMaxEchoChanged(const FOnAttributeChangeData& ChangeData)
{
	if (!AttributeSet) { return; }
	OnEchoChanged.Broadcast(AttributeSet->GetEcho(), ChangeData.NewValue);
}

void ASFCharacterBase::HandleShieldsChanged(const FOnAttributeChangeData& ChangeData)
{
	if (!AttributeSet) { return; }
	OnShieldsChanged.Broadcast(ChangeData.NewValue, AttributeSet->GetMaxShields());
}

void ASFCharacterBase::HandleMaxShieldsChanged(const FOnAttributeChangeData& ChangeData)
{
	if (!AttributeSet) { return; }
	OnShieldsChanged.Broadcast(AttributeSet->GetShields(), ChangeData.NewValue);
}

void ASFCharacterBase::HandleStaminaChanged(const FOnAttributeChangeData& ChangeData)
{
	if (!AttributeSet) { return; }
	OnStaminaChanged.Broadcast(ChangeData.NewValue, AttributeSet->GetMaxStamina());
}

void ASFCharacterBase::HandleMaxStaminaChanged(const FOnAttributeChangeData& ChangeData)
{
	if (!AttributeSet) { return; }
	OnStaminaChanged.Broadcast(AttributeSet->GetStamina(), ChangeData.NewValue);
}

void ASFCharacterBase::HandleGuardChanged(const FOnAttributeChangeData& ChangeData)
{
	if (!AttributeSet) { return; }
	OnGuardChanged.Broadcast(ChangeData.NewValue, AttributeSet->GetMaxGuard());
}

void ASFCharacterBase::HandleMaxGuardChanged(const FOnAttributeChangeData& ChangeData)
{
	if (!AttributeSet) { return; }
	OnGuardChanged.Broadcast(AttributeSet->GetGuard(), ChangeData.NewValue);
}

void ASFCharacterBase::HandlePoiseChanged(const FOnAttributeChangeData& ChangeData)
{
	if (!AttributeSet) { return; }
	// If you later add MaxPoise, pass that as the second param.
	OnPoiseChanged.Broadcast(ChangeData.NewValue, ChangeData.NewValue);
}

// -----------------------------------------------------------------------------
// Combat – new break handlers
// -----------------------------------------------------------------------------

void ASFCharacterBase::HandleGuardBreak_Implementation()
{
	if (bIsDead)
	{
		return;
	}

	UE_LOG(LogSFCharacter, Verbose, TEXT("%s: guard break triggered."), *GetName());

	if (!GuardBreakMontage || !GetMesh())
	{
		// Fallback to normal hit react if no dedicated montage
		HandleHitReact();
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(GuardBreakMontage);
	}
}

void ASFCharacterBase::HandlePoiseBreak_Implementation()
{
	if (bIsDead)
	{
		return;
	}

	UE_LOG(LogSFCharacter, Verbose, TEXT("%s: poise break triggered."), *GetName());

	if (!PoiseBreakMontage || !GetMesh())
	{
		// Fallback to normal hit react if no dedicated montage
		HandleHitReact();
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(PoiseBreakMontage);
	}
}

// -----------------------------------------------------------------------------
// Combat
// -----------------------------------------------------------------------------

void ASFCharacterBase::HandleLightAttackHitEvent()
{
	if (CombatComponent)
	{
		CombatComponent->HandleLightAttackHit();
	}
}

void ASFCharacterBase::HandleHeavyAttackHitEvent()
{
	if (CombatComponent)
	{
		CombatComponent->HandleHeavyAttackHit();
	}
}

void ASFCharacterBase::HandleHitReact()
{
	if (bIsDead)
	{
		return;
	}

	UE_LOG(LogSFCharacter, Verbose, TEXT("%s: hit react triggered."), *GetName());

	if (!HitReactMontage || !GetMesh())
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(HitReactMontage);
	}
}

// -----------------------------------------------------------------------------
// Death
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Faction / team interface
// -----------------------------------------------------------------------------

FGenericTeamId ASFCharacterBase::GetGenericTeamId() const
{
	if (FactionComponent)
	{
		return FactionComponent->GetTeamId();
	}
	return FGenericTeamId::NoTeam;
}

ETeamAttitude::Type ASFCharacterBase::GetTeamAttitudeTowards(const AActor& Other) const
{
	if (FactionComponent)
	{
		return FactionComponent->GetAttitudeTowardOther(Other);
	}
	return ETeamAttitude::Neutral;
}

// -----------------------------------------------------------------------------
// XP / Death attribution
// -----------------------------------------------------------------------------

void ASFCharacterBase::SetLastDamagingCharacter(ASFCharacterBase* InCharacter)
{
	LastDamagingCharacter = InCharacter;
}

// -----------------------------------------------------------------------------
// ISFCombatantInterface
// -----------------------------------------------------------------------------

FVector ASFCharacterBase::GetCombatLocation() const
{
	// Mid-capsule (center-of-mass) is a better aim target than actor origin (feet).
	if (const UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		return Capsule->GetComponentLocation();
	}
	return GetActorLocation();
}

FTransform ASFCharacterBase::GetCombatSocketTransform(FName SocketName) const
{
	if (const USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		if (SocketName != NAME_None && SkelMesh->DoesSocketExist(SocketName))
		{
			return SkelMesh->GetSocketTransform(SocketName);
		}
	}
	return GetActorTransform();
}

void ASFCharacterBase::GetCombatStateTags(FGameplayTagContainer& OutTags) const
{
	if (const USFAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		ASC->GetOwnedGameplayTags(OutTags);
	}
}

void ASFCharacterBase::RegisterDamageInstigator(AActor* InInstigator)
{
	// Only characters are tracked for XP/death attribution; non-character
	// damage sources (turrets, traps) intentionally leave LastDamagingCharacter
	// alone so the previous attacker still gets credit.
	// (Parameter is InInstigator -- not Instigator -- to avoid shadowing
	// AActor::Instigator under MSVC /WX.)
	if (ASFCharacterBase* AsCharacter = Cast<ASFCharacterBase>(InInstigator))
	{
		LastDamagingCharacter = AsCharacter;
	}
}

// -----------------------------------------------------------------------------
// ISFWeaponHolderInterface
// -----------------------------------------------------------------------------

bool ASFCharacterBase::GetActiveMuzzleTransform(FTransform& OutTransform) const
{
	// Default impl walks: this -> EquipmentComponent -> equipped ASFWeaponActor -> muzzle socket.
	// Non-character holders (turrets, mounted guns) can override to provide a muzzle transform
	// without going through the equipment component at all.
	OutTransform = FTransform::Identity;
	if (!EquipmentComponent)
	{
		return false;
	}

	ASFWeaponActor* WeaponActor = EquipmentComponent->GetEquippedWeaponActor();
	if (!WeaponActor || !WeaponActor->HasValidMuzzleSocket())
	{
		return false;
	}

	OutTransform = FTransform(WeaponActor->GetMuzzleRotation(), WeaponActor->GetMuzzleLocation());
	return true;
}

int32 ASFCharacterBase::GetXPReward() const
{
	if (const USFProgressionComponent* Progression = GetProgressionComponent())
	{
		return Progression->GetEnemyXPRewardForCurrentLevel();
	}
	return 0;
}

void ASFCharacterBase::HandleDeath()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	StopCrouch();

	// Tag the ASC as dead BEFORE broadcasting / cleanup so any reactive logic
	// (offensive abilities, AI ticks, behaviour-tree decorators, status
	// applicators) that wakes up inside OnCharacterDied sees the dead state.
	// Listed as a BlockedTag on every offensive ability so corpses can't keep
	// firing weapons or swinging melee.
	if (AbilitySystemComponent)
	{
		// Renamed to SFTags to avoid shadowing AActor::Tags (TArray<FName>) under
		// -WX (C4458). UBT promotes the shadow warning to a hard error on MSVC.
		const FSignalForgeGameplayTags& SFTags = FSignalForgeGameplayTags::Get();
		if (SFTags.State_Dead.IsValid())
		{
			AbilitySystemComponent->AddLooseGameplayTag(SFTags.State_Dead);
		}

		// Hard-stop any abilities currently mid-activation (fire bursts, beam
		// tick loops, melee montages). The State.Dead BlockedTag prevents new
		// activations; this kills the ones already running.
		FGameplayTagContainer CancelAllAbilities;
		AbilitySystemComponent->CancelAbilities(&CancelAllAbilities, nullptr, nullptr);
	}

	// Broadcast death AS EARLY AS POSSIBLE -- listeners (loot droppers,
	// quest objectives, kill feeds, achievements) typically want the dying
	// actor still fully present (location, mesh, transform) when they run.
	// We're past the bIsDead guard so this can never double-fire even if
	// HandleDeath is re-entered from a listener.
	OnCharacterDied.Broadcast(this, LastDamagingCharacter);

	// Generic XP grant: if our killer's faction was hostile to ours, reward them.
	// This replaces ASFEnemyCharacter::HandleDeath's class-locked attribution.
	if (LastDamagingCharacter && LastDamagingCharacter != this && !LastDamagingCharacter->IsDead())
	{
		if (USFFactionStatics::AreHostile(LastDamagingCharacter, this))
		{
			if (USFProgressionComponent* Progression = LastDamagingCharacter->GetProgressionComponent())
			{
				const int32 XPReward = GetXPReward();
				if (XPReward > 0)
				{
					Progression->AddXP(XPReward);
				}
			}
		}
	}

	UE_LOG(LogSFCharacter, Log, TEXT("%s has died."), *GetName());

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->DisableMovement();
		MoveComp->StopMovementImmediately();
	}

	if (AController* LocalController = GetController())
	{
		LocalController->StopMovement();
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (bEnableRagdollOnDeath)
		{
			MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
			MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			MeshComp->SetSimulatePhysics(true);
			MeshComp->SetAllBodiesSimulatePhysics(true);
			MeshComp->WakeAllRigidBodies();
			MeshComp->bBlendPhysics = true;
		}
		else
		{
			MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	StartDeathCleanupTimer();
}

void ASFCharacterBase::StartDeathCleanupTimer()
{
	if (DeathCleanupDelay > 0.0f)
	{
		SetLifeSpan(DeathCleanupDelay);
	}
}

// -----------------------------------------------------------------------------
// Movement
// -----------------------------------------------------------------------------

void ASFCharacterBase::PerformDash(const FVector& DashDirection, float DashStrength)
{
	if (bIsDead)
	{
		return;
	}

	FVector FlatDirection = DashDirection;
	FlatDirection.Z = 0.0f;
	FlatDirection = FlatDirection.GetSafeNormal();

	if (FlatDirection.IsNearlyZero())
	{
		return;
	}

	LaunchCharacter(FlatDirection * DashStrength, true, false);
}

void ASFCharacterBase::StartSprint()
{
	if (bIsDead)
	{
		return;
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = SprintSpeed;
	}
}

void ASFCharacterBase::StopSprint()
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = WalkSpeed;
	}
}

void ASFCharacterBase::StartCrouch()
{
	if (bIsDead)
	{
		return;
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		// Optional guards: don’t crouch if falling, etc.
		if (!MoveComp->IsFalling())
		{
			Crouch(); // ACharacter::Crouch (respects bCanCrouch, capsule resizing, etc.)
		}
	}
}

void ASFCharacterBase::StopCrouch()
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		UnCrouch();
		MoveComp->MaxWalkSpeed = WalkSpeed;
	}
}

FGameplayAbilitySpecHandle ASFCharacterBase::GrantCharacterAbility(
	TSubclassOf<UGameplayAbility> AbilityClass, int32 AbilityLevel)
{
	if (!AbilitySystemComponent || !AbilityClass)
	{
		return FGameplayAbilitySpecHandle();
	}

	// Only grant on authority
	if (!AbilitySystemComponent->IsOwnerActorAuthoritative())
	{
		return FGameplayAbilitySpecHandle();
	}

	FGameplayAbilitySpec AbilitySpec(AbilityClass, AbilityLevel);

	FGameplayTag GrantedInputTag;
	if (const USFGameplayAbility* AbilityCDO = AbilityClass->GetDefaultObject<USFGameplayAbility>())
	{
		GrantedInputTag = AbilityCDO->GetInputTag();
		if (GrantedInputTag.IsValid())
		{
			AbilitySpec.GetDynamicSpecSourceTags().AddTag(GrantedInputTag);
		}
	}

	UE_LOG(LogSFCharacter, Warning,
		TEXT("GrantCharacterAbility: class=%s InputTag=%s (CDO valid=%d)"),
		*AbilityClass->GetName(),
		GrantedInputTag.IsValid() ? *GrantedInputTag.ToString() : TEXT("<INVALID/EMPTY>"),
		AbilityClass->GetDefaultObject<USFGameplayAbility>() != nullptr ? 1 : 0);

	return AbilitySystemComponent->GiveAbility(AbilitySpec);
}

void ASFCharacterBase::OnBlockTagChanged(bool bHasBlockTag)
{
	bAnimIsBlocking = bHasBlockTag;

	if (USFAnimInstanceBase* SFAnim = Cast<USFAnimInstanceBase>(GetMesh()->GetAnimInstance()))
	{
		SFAnim->SetIsBlocking(bHasBlockTag);
	}
}

void ASFCharacterBase::HandleBlockTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	const bool bHasBlockTag = (NewCount > 0);
	OnBlockTagChanged(bHasBlockTag);
}

FSFResolvedHit ASFCharacterBase::ResolveIncomingHit(const FSFHitData& HitData)
{
	FSFResolvedHit Result;

	if (IsDead() || !HitData.TargetActor)
	{
		Result.Outcome = ESFHitOutcome::Immune;
		return Result;
	}

	// Ask the attacker what their weapon thinks this swing should do. This lets the weapon asset's
	// BaseDamage (or per-combo-step LightComboDamages / HeavyComboDamages) flow through the resolver
	// path the same way it flows through the no-resolver fallback. Without this, every melee hit
	// against any ASFCharacterBase-derived enemy snapped to the hardcoded 25.f below regardless of
	// what was on the weapon asset.
	float BaseDamageFromAttacker = 0.f;
	if (AActor* SourceActor = HitData.SourceActor.Get())
	{
		if (USFCombatComponent* AttackerCombat = SourceActor->FindComponentByClass<USFCombatComponent>())
		{
			BaseDamageFromAttacker = AttackerCombat->ResolveBaseMeleeDamage(HitData.AttackType);
		}
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	const USFAttributeSetBase* Attr = AttributeSet;
	if (!ASC || !Attr)
	{
		Result.Outcome = ESFHitOutcome::Hit;
		Result.HealthDamage = (BaseDamageFromAttacker > 0.f) ? BaseDamageFromAttacker : 20.f;
		return Result;
	}

	const FSignalForgeGameplayTags& GameplayTags = FSignalForgeGameplayTags::Get();

	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);

	const bool bIsBlocking = OwnedTags.HasTagExact(GameplayTags.State_Blocking);
	const bool bIsBroken = OwnedTags.HasTagExact(GameplayTags.State_Broken);
	const bool bInParryWindow = OwnedTags.HasTagExact(GameplayTags.State_ParryWindow);
	const bool bInvulnerable = OwnedTags.HasTagExact(GameplayTags.State_Invulnerable);

	const float CurrentGuard = Attr->GetGuard();
	const float CurrentPoise = Attr->GetPoise();

	if (bInvulnerable)
	{
		Result.Outcome = ESFHitOutcome::Immune;
		return Result;
	}

	if (bIsBroken)
	{
		Result.Outcome = ESFHitOutcome::Hit;
		// Broken targets take full weapon damage (broken-state bonus could be layered here later).
		Result.HealthDamage = (BaseDamageFromAttacker > 0.f) ? BaseDamageFromAttacker : 25.f;
		Result.PoiseDamageToTarget = 0.f;
		return Result;
	}

	const bool bCanBlockThis = HitData.bIsBlockable && !HitData.bIgnoreGuard;

	if (bCanBlockThis && bInParryWindow)
	{
		Result.Outcome = ESFHitOutcome::PerfectParry;
		Result.HealthDamage = 0.f;
		Result.PoiseDamageToTarget = 40.f * HitData.PoiseDamageScale;
		Result.PoiseDamageToSource = -20.f;
		Result.bCauseStagger = true;
		Result.bTriggerSlowMo = true;
		Result.ResultTagsOnTarget.AddTag(GameplayTags.State_Broken);

		// Close the parry window the moment it lands so a single tap can't parry
		// multiple incoming hits in the same ParryWindowSeconds budget. The block
		// ability's CloseParryWindow timer will still fire normally and start the
		// cooldown; RemoveLooseGameplayTag is a no-op if already absent.
		ASC->RemoveLooseGameplayTag(GameplayTags.State_ParryWindow);

		return Result;
	}

	if (bCanBlockThis && bIsBlocking && CurrentGuard > 0.f)
	{
		Result.Outcome = ESFHitOutcome::Blocked;
		Result.HealthDamage = 0.f;
		Result.PoiseDamageToTarget = 25.f * HitData.PoiseDamageScale;
		Result.PoiseDamageToSource = 10.f;

		if (CurrentGuard <= Result.PoiseDamageToTarget)
		{
			Result.bCauseGuardBreak = true;
			Result.ResultTagsOnTarget.AddTag(GameplayTags.State_Broken);
		}

		return Result;
	}

	Result.Outcome = ESFHitOutcome::Hit;
	Result.HealthDamage = (BaseDamageFromAttacker > 0.f) ? BaseDamageFromAttacker : 25.f;
	Result.PoiseDamageToTarget = 15.f * HitData.PoiseDamageScale;
	Result.PoiseDamageToSource = 0.f;

	if (CurrentPoise <= Result.PoiseDamageToTarget)
	{
		Result.bCauseStagger = true;
		Result.ResultTagsOnTarget.AddTag(GameplayTags.State_Broken);
	}

	return Result;
}

void ASFCharacterBase::ApplyWeaponAnimationFromData(const USFWeaponData* WeaponData)
{
	if (!WeaponData)
	{
		ClearWeaponAnimationProfile();
		return;
	}

	CurrentCombatMode = WeaponData->CombatMode;

	const FSFWeaponAnimationProfile& Profile = WeaponData->GetAnimationProfile();
	CurrentWeaponAnimationProfile = Profile;
	CurrentOverlayMode = Profile.OverlayMode;
	bUseUpperBodyOverlay = Profile.bUseUpperBodyOverlay;

	// Pick the form-specific overlay layer if this character has a form tag,
	// otherwise fall back to the weapon's base overlay layer.
	const FGameplayTag FormTag = GetCharacterFormTag();
	const TSubclassOf<UAnimInstance> NewLayerClass = FormTag.IsValid()
		? WeaponData->GetOverlayLayerForForm(FormTag)
		: WeaponData->GetOverlayLinkedAnimLayerClass();

	SetOverlayLinkedAnimLayer(NewLayerClass);

	// Resolve animation sequences from the weapon's animation set
	if (USFWeaponAnimationSet* AnimationSet = WeaponData->GetWeaponAnimationSet())
	{
		// Synchronously load the animation set content
		AnimationSet->LoadAnimationSetContentSynchronously();

		// Cache the resolved sequences
		CurrentIdleOverlaySequence = AnimationSet->GetIdleOverlaySequence();
		CurrentWalkOverlaySequence = AnimationSet->GetWalkOverlaySequence();
		CurrentSprintOverlaySequence = AnimationSet->GetSprintOverlaySequence();
	}
	else
	{
		// No animation set — clear sequences but keep the profile active
		CurrentIdleOverlaySequence = nullptr;
		CurrentWalkOverlaySequence = nullptr;
		CurrentSprintOverlaySequence = nullptr;
	}

	bHasWeaponAnimationProfile = true;
}

void ASFCharacterBase::ClearWeaponAnimationProfile()
{
	CurrentOverlayMode = ESFOverlayMode::Unarmed;
	CurrentCombatMode = ESFCombatMode::None;
	bUseUpperBodyOverlay = true;
	CurrentWeaponAnimationProfile = FSFWeaponAnimationProfile{};
	SetOverlayLinkedAnimLayer(nullptr);
	bHasWeaponAnimationProfile = false;

	// Clear cached sequences
	CurrentIdleOverlaySequence = nullptr;
	CurrentWalkOverlaySequence = nullptr;
	CurrentSprintOverlaySequence = nullptr;
}

void ASFCharacterBase::OnWeaponEquipped_Implementation(const USFWeaponData* WeaponData)
{
	ApplyWeaponAnimationFromData(WeaponData);
}

// -----------------------------------------------------------------------------
// Overlay linked anim layer (Lyra-style "Attach Object to Hand" anim half).
// The data half (mesh / socket / weapon BP) lives on USFWeaponData and is
// applied by the equipment component / weapon actor; this function handles
// the LinkAnimClassLayers side so that any ASFCharacterBase (player,
// companion, enemy NPC) gets the right overlay automatically based on the
// equipped weapon's data asset.
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Locomotion intent flags (aim, sprint)
// -----------------------------------------------------------------------------
// These are pure intent bits: "does the player/AI want to aim / sprint right
// now?" — not gameplay-state ("is the aim ability currently active?"). Input
// handlers and gameplay abilities both write through these setters; the
// AnimGraph reads them via Property Access on either the character or the
// AnimInstance mirror, and delegates fire on transition so abilities can
// react to intent changes.

// Order inside every setter is the same:
//   1. Early-return on no-change so callers can spam every frame.
//   2. Update the bool first so any code triggered by the broadcasts (or by
//      OnLocomotionIntentChanged) sees the new value via GetWantsToX().
//   3. Broadcast the per-flag delegate for listeners that care about exactly
//      one transition (abilities, UI, audio).
//   4. Fire OnLocomotionIntentChanged() last. This is the chokepoint BP
//      subclasses override to mirror the four flags into their BP
//      "Character Input State" struct so the existing GASP BP functions
//      (UpdateRotation_PreCMC, GetDesiredGait, CanSprint, CalculateMaxSpeed)
//      keep reading a single source of truth.

void ASFCharacterBase::SetWantsToAim(bool bInWantsToAim)
{
	if (bWantsToAim == bInWantsToAim)
	{
		return;
	}

	bWantsToAim = bInWantsToAim;
	OnWantsToAimChanged.Broadcast(bWantsToAim);
	OnLocomotionIntentChanged();
}

void ASFCharacterBase::SetWantsToSprint(bool bInWantsToSprint)
{
	if (bWantsToSprint == bInWantsToSprint)
	{
		return;
	}

	bWantsToSprint = bInWantsToSprint;
	OnWantsToSprintChanged.Broadcast(bWantsToSprint);
	OnLocomotionIntentChanged();
}

void ASFCharacterBase::SetWantsToWalk(bool bInWantsToWalk)
{
	if (bWantsToWalk == bInWantsToWalk)
	{
		return;
	}

	bWantsToWalk = bInWantsToWalk;
	OnWantsToWalkChanged.Broadcast(bWantsToWalk);
	OnLocomotionIntentChanged();
}

void ASFCharacterBase::SetWantsToStrafe(bool bInWantsToStrafe)
{
	if (bWantsToStrafe == bInWantsToStrafe)
	{
		return;
	}

	bWantsToStrafe = bInWantsToStrafe;
	OnWantsToStrafeChanged.Broadcast(bWantsToStrafe);
	OnLocomotionIntentChanged();
}

void ASFCharacterBase::SetOverlayLinkedAnimLayer(TSubclassOf<UAnimInstance> NewLayerClass)
{
	const TSubclassOf<UAnimInstance> PreviousLayerClass = CurrentOverlayLinkedAnimLayerClass;

	if (PreviousLayerClass == NewLayerClass)
	{
		return;
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (PreviousLayerClass)
		{
			MeshComp->UnlinkAnimClassLayers(PreviousLayerClass);
		}
		if (NewLayerClass)
		{
			MeshComp->LinkAnimClassLayers(NewLayerClass);
		}
	}

	CurrentOverlayLinkedAnimLayerClass = NewLayerClass;
	OnOverlayLinkedAnimLayerChanged.Broadcast(NewLayerClass, PreviousLayerClass);
}

void ASFCharacterBase::RefreshOverlayLinkedAnimLayer()
{
	if (!EquipmentComponent)
	{
		SetOverlayLinkedAnimLayer(nullptr);
		return;
	}

	const USFWeaponData* WeaponData = EquipmentComponent->GetCurrentWeaponData();
	if (!WeaponData)
	{
		SetOverlayLinkedAnimLayer(nullptr);
		return;
	}

	const FGameplayTag FormTag = GetCharacterFormTag();
	const TSubclassOf<UAnimInstance> NewLayerClass = FormTag.IsValid()
		? WeaponData->GetOverlayLayerForForm(FormTag)
		: WeaponData->GetOverlayLinkedAnimLayerClass();

	SetOverlayLinkedAnimLayer(NewLayerClass);
}

FGameplayTag ASFCharacterBase::GetCharacterFormTag_Implementation() const
{
	// Default: no form tag, use weapon's base overlay layer.
	return FGameplayTag();
}

// =============================================================================
// Gait (agnostic locomotion surface)
//
// Lifted out of CBP_SandboxCharacter so ABP_Biped can drive locomotion through
// USFAnimInstanceBase::Gait without casting to any specific BP class. NPCs use
// the defaults below with zero per-class wiring; the player BP overrides
// UpdateMovement_PreCMC to keep its GASP curve graph.
// =============================================================================

void ASFCharacterBase::SetCurrentGait(ESFGait NewGait)
{
	if (CurrentGait == NewGait)
	{
		return;
	}

	const ESFGait PreviousGait = CurrentGait;
	CurrentGait = NewGait;
	OnGaitChanged.Broadcast(CurrentGait, PreviousGait);
}

float ASFCharacterBase::GetMaxSpeedForGait(ESFGait Gait) const
{
	switch (Gait)
	{
	case ESFGait::Walk:		return GaitProfile.WalkSpeed;
	case ESFGait::Sprint:	return GaitProfile.SprintSpeed;
	case ESFGait::Run:
	default:				return GaitProfile.RunSpeed;
	}
}

ESFGait ASFCharacterBase::GetDesiredGait_Implementation() const
{
	// Walk intent always wins -- player explicitly toggled walk.
	if (bWantsToWalk)
	{
		return ESFGait::Walk;
	}

	// Sprint intent gated by CanSprint() (dead / falling / crouched lockouts).
	if (bWantsToSprint && CanSprint())
	{
		return ESFGait::Sprint;
	}

	return ESFGait::Run;
}

bool ASFCharacterBase::CanSprint_Implementation() const
{
	if (bIsDead || bIsCrouched)
	{
		return false;
	}

	if (const UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		if (MoveComp->IsFalling())
		{
			return false;
		}
	}

	return true;
}

void ASFCharacterBase::UpdateMovement_PreCMC_Implementation(float /*DeltaTime*/)
{
	// 1. Resolve desired gait from intent + gates and publish it. SetCurrentGait
	//    only broadcasts on transition, so this is cheap to call every frame.
	SetCurrentGait(GetDesiredGait());

	// 2. Push the active gait's tuning into the CharacterMovementComponent so
	//    the same-frame movement update uses the right speed / accel / friction.
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	MoveComp->MaxWalkSpeed = GetMaxSpeedForGait(CurrentGait);
	MoveComp->MaxWalkSpeedCrouched = GaitProfile.CrouchSpeed;
	MoveComp->MaxAcceleration = GaitProfile.MaxAcceleration;
	MoveComp->BrakingDecelerationWalking = GaitProfile.BrakingDeceleration;
	MoveComp->GroundFriction = GaitProfile.GroundFriction;
}