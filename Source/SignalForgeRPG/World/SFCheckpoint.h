#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SFCheckpoint.generated.h"

class UBillboardComponent;
class UArrowComponent;
class UCapsuleComponent;

/**
 * A respawn anchor designers drop in the level. The player overlap activates
 * this checkpoint on the game state, and dark-zone deaths respawn the player
 * here. Non-dark-zone deaths respawn near the death location instead, so the
 * checkpoint only matters in the high-risk areas.
 *
 * The arrow component points in the direction the respawned player will face.
 */
UCLASS()
class SIGNALFORGERPG_API ASFCheckpoint : public AActor
{
	GENERATED_BODY()

public:
	ASFCheckpoint();

	/** Where to respawn the pawn (this actor's transform + any forward offset). */
	UFUNCTION(BlueprintCallable, Category = "Checkpoint")
	FTransform GetRespawnTransform() const;

	/** Friendly label surfaced on the death screen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Checkpoint")
	FText CheckpointDisplayName;

	/**
	 * If true, the player auto-activates this checkpoint on overlap. Leave
	 * off and call ActivateCheckpoint() from gameplay code (e.g. quest step
	 * completion) for scripted activation points.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Checkpoint")
	bool bActivateOnPlayerOverlap = true;

	/**
	 * Mark this checkpoint as active. The activating actor is forwarded to
	 * the game state for any downstream policy (currently unused but kept on
	 * the signature for telemetry / future use). Accepts AActor* so either
	 * protagonist (or a scripted trigger) can drive it without coupling to
	 * a concrete pawn class.
	 */
	UFUNCTION(BlueprintCallable, Category = "Checkpoint")
	void ActivateCheckpoint(AActor* ActivatingActor);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	UPROPERTY(VisibleAnywhere, Category = "Checkpoint")
	TObjectPtr<UCapsuleComponent> TriggerCapsule;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "Checkpoint")
	TObjectPtr<UBillboardComponent> Billboard;

	UPROPERTY(VisibleAnywhere, Category = "Checkpoint")
	TObjectPtr<UArrowComponent> SpawnDirection;
#endif
};
