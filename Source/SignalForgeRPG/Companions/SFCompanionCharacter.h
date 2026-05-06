#pragma once

#include "CoreMinimal.h"
#include "Characters/SFNPCBase.h"
#include "SFCompanionCharacter.generated.h"

class USFCompanionTacticsComponent;

/**
 * ASFCompanionCharacter
 *
 * Player-aligned, controllable AI ally. Inherits the full SFNPCBase
 * stack (GAS, attributes, equipment, narrative identity, dialogue,
 * faction) and layers on the tactics component (stance, aggression,
 * orders) plus a unique CompanionId for roster management.
 *
 * Companions are recruited into the player's USFCompanionComponent
 * (on the PlayerState), where the active companion follows the player
 * and accepts orders via the radial wheel.
 */
UCLASS()
class SIGNALFORGERPG_API ASFCompanionCharacter : public ASFNPCBase
{
	GENERATED_BODY()

public:
	ASFCompanionCharacter();

	//~ ASFCharacterBase
	virtual void HandleDeath() override;
	//~ End

	UFUNCTION(BlueprintPure, Category = "Companion")
	USFCompanionTacticsComponent* GetTactics() const { return Tactics; }

	UFUNCTION(BlueprintPure, Category = "Companion")
	FName GetCompanionId() const { return CompanionId; }

	UFUNCTION(BlueprintPure, Category = "Companion")
	const FText& GetDisplayName() const { return DisplayName; }

protected:
	/** Stable identifier used as the key into the player's roster and approval map. Designer-set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Companion")
	FName CompanionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Companion")
	FText DisplayName;

	/** Optional portrait for radial wheel / party UI. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Companion")
	TSoftObjectPtr<UTexture2D> Portrait;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Companion")
	TObjectPtr<USFCompanionTacticsComponent> Tactics;

	/** Personal quest unlocked at high approval. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Companion|Narrative")
	FPrimaryAssetId PersonalQuestAssetId;

	/** Approval threshold required to unlock the personal quest. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Companion|Narrative")
	int32 PersonalQuestApprovalThreshold = 50;

public:
	UFUNCTION(BlueprintPure, Category = "Companion|Narrative")
	const FPrimaryAssetId& GetPersonalQuestAssetId() const { return PersonalQuestAssetId; }

	UFUNCTION(BlueprintPure, Category = "Companion|Narrative")
	int32 GetPersonalQuestApprovalThreshold() const { return PersonalQuestApprovalThreshold; }
};
