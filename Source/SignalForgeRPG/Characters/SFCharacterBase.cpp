#include "Characters/SFCharacterBase.h"

#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystem/SFGameplayAbility.h"
#include "Abilities/GameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "Animation/SFAnimInstanceBase.h"
#include "Combat/SFCombatComponent.h"
#include "Combat/SFHitResolverInterface.h"
#include "Components/CapsuleComponent.h"
#include "Components/SFEquipmentComponent.h"
#include "Components/SFAmmoReserveComponent.h"
#include "Components/SFInventoryComponent.h"
#include "Components/SFProgressionComponent.h"
#include "Components/SFStatRegenComponent.h"
#include "Faction/SFFactionComponent.h"
#include "Faction/SFFactionStatics.h"
#include "Core/SFAttributeSetBase.h"
#include "Core/SignalForgeGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "Combat/SFWeaponData.h"
#include "Components/SFInteractionComponent.h"
#include "Combat/SFWeaponAnimationSet.h"

DEFINE_LOG_CATEGORY_STATIC(LogSFCharacter, Log, All);

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

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = WalkSpeed;
		MoveComp->MaxWalkSpeedCrouched = CrouchSpeed;
		MoveComp->GetNavAgentPropertiesRef().bCanCrouch = true;
	}
}

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

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
	OnHealthChanged.Broadcast(ChangeData.NewValue, AttributeSet->GetMaxHealth());
}

void ASFCharacterBase::HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	if (!AttributeSet) { return; }
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

	UE_LOG(LogTemp, Warning,
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

FSFResolvedHit ASFCharacterBase::ResolveIncomingHit_Implementation(const FSFHitData& HitData)
{
	FSFResolvedHit Result;

	if (IsDead() || !HitData.TargetActor)
	{
		Result.Outcome = ESFHitOutcome::Immune;
		return Result;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	const USFAttributeSetBase* Attr = AttributeSet;
	if (!ASC || !Attr)
	{
		Result.Outcome = ESFHitOutcome::Hit;
		Result.HealthDamage = 20.f;
		return Result;
	}

	const FSignalForgeGameplayTags& GameplayTags = FSignalForgeGameplayTags::Get();

	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);

	const bool bIsBlocking = OwnedTags.HasTagExact(GameplayTags.State_Blocking);
	const bool bIsBroken = OwnedTags.HasTagExact(GameplayTags.State_Broken);
	const bool bInParryWindow =
		OwnedTags.HasTagExact(FGameplayTag::RequestGameplayTag(TEXT("State.ParryWindow")));
	const bool bInvulnerable =
		OwnedTags.HasTagExact(FGameplayTag::RequestGameplayTag(TEXT("State.Invulnerable")));

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
		Result.HealthDamage = 25.f;
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
	Result.HealthDamage = 25.f;
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

FVector ASFCharacterBase::GetHeadLookAtLocation_Implementation(bool& bIsValid) const
{
	bIsValid = false;
	return GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);
}