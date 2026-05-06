#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/SFHitTypes.h"                // FSFHitData, FSFResolvedHit, ESFAttackType
#include "SFCombatComponent.generated.h"

class USFEquipmentComponent;
class ASFWeaponActor;
class UGameplayEffect;
class ASFCharacterBase;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SIGNALFORGERPG_API USFCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFCombatComponent();

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TObjectPtr<ASFCharacterBase> OwnerCharacter;

	/** LIGHT ATTACK TRACE SETTINGS */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Light")
	float LightAttackTraceStartOffset = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Light")
	float LightAttackTraceLength = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Light")
	float LightAttackTraceRadius = 50.0f;

	/** HEAVY ATTACK TRACE SETTINGS */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Heavy")
	float HeavyAttackTraceStartOffset = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Heavy")
	float HeavyAttackTraceLength = 125.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Heavy")
	float HeavyAttackTraceRadius = 70.0f;

	/** Actors already hit during the current attack window */
	UPROPERTY()
	TSet<TObjectPtr<AActor>> HitActorsThisAttack;

	/** Helper to get weapon socket trace points, returns false if not available */
	bool GetWeaponTracePoints(FVector& OutStart, FVector& OutEnd) const;

public:
	/** Called from animation notifies / abilities during light attack window */
	void HandleLightAttackHit();

	/** Called from animation notifies / abilities during heavy attack window */
	void HandleHeavyAttackHit();

	/** Called at start of an attack’s active hit window */
	void BeginAttackWindow();

	/** Called at end of an attack’s active hit window */
	void EndAttackWindow();

protected:
	/** Core function to process an attack type (Light/Heavy) */
	void HandleAttackHitInternal(ESFAttackType AttackType);

	/** Multi‑hit sweep trace using pawn channel */
	bool PerformAttackSweep(float StartOffset, float TraceLength, float TraceRadius, TArray<FHitResult>& OutHitResults) const;

	/** Build hit data from a trace result */
	FSFHitData BuildHitDataFromResult(AActor* HitActor, const FHitResult& HitResult, ESFAttackType AttackType) const;

	/** Apply resolved hit via GAS, including health/poise/guard changes and tags */
	void ApplyResolvedHitWithGAS(const FSFHitData& HitData, const FSFResolvedHit& ResolvedHit);

	/** Optional: drive VFX/SFX/camera based on hit outcome */
	void TriggerHitFeedback(const FSFHitData& HitData, const FSFResolvedHit& ResolvedHit);

	/** Utility: get base damage effect for a given attack type from the owner */
	TSubclassOf<UGameplayEffect> GetDamageEffectForAttackType(ESFAttackType AttackType) const;
};