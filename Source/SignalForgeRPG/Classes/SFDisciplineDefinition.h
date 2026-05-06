#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayEffect.h"
#include "Abilities/GameplayAbility.h"
#include "SFDisciplineDefinition.generated.h"

// -----------------------------------------------------------------------------
// Skill tree tier structure
// Tier 1: Abilities 1-2 (available at discipline unlock)
// Tier 2: Abilities 3-5 (require Tier 1 completion)
// Tier 3: Abilities 6-8 (require Tier 2 completion, includes ultimate)
// -----------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ESFDisciplineTier : uint8
{
	Tier1	UMETA(DisplayName = "Tier 1 - Foundation"),
	Tier2	UMETA(DisplayName = "Tier 2 - Advanced"),
	Tier3	UMETA(DisplayName = "Tier 3 - Mastery"),
};

// -----------------------------------------------------------------------------
// A single node in the discipline skill tree.
// Each node represents one ability the player can unlock.
// -----------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FSFDisciplineSkillNode
{
	GENERATED_BODY()

	/** Display name shown in the skill tree UI. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node")
	FText NodeName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node")
	FText NodeDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node")
	TObjectPtr<UTexture2D> NodeIcon = nullptr;

	/** Which tier this node belongs to. Controls unlock requirements. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node")
	ESFDisciplineTier Tier = ESFDisciplineTier::Tier1;

	/** The ability granted when this node is purchased. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node")
	TSubclassOf<UGameplayAbility> GrantedAbility;

	/**
	 * If set, this ability is removed from the character when this node
	 * is purchased. Used to substitute base class abilities with discipline
	 * variants. Optional — player configures which base ability to replace.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node")
	TSubclassOf<UGameplayAbility> ReplacesAbility;

	/** Cost in Discipline Points to unlock this node. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node",
		Meta = (ClampMin = "1"))
	int32 DisciplinePointCost = 1;

	/** Unique identifier for this node within its discipline. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Node")
	FName NodeID;
};

// -----------------------------------------------------------------------------
// USFDisciplineDefinition
// One data asset per discipline (sub-specialization).
// Assigned to a class definition. Players unlock these at level 10.
// -----------------------------------------------------------------------------

UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFDisciplineDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// -------------------------------------------------------------------------
	// Identity
	// -------------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FName DisciplineName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FText DisciplineDisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FText DisciplineDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	TObjectPtr<UTexture2D> DisciplineIcon = nullptr;

	/** Accent color used in UI to differentiate disciplines within a class. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FLinearColor DisciplineColor = FLinearColor::White;

	// -------------------------------------------------------------------------
	// Skill Tree
	//
	// All nodes across all three tiers live here.
	// Tier is set per-node. UI reads tier to determine positioning.
	// Tier 1: unlocked when discipline is selected
	// Tier 2: requires all Tier 1 nodes purchased
	// Tier 3: requires all Tier 2 nodes purchased
	// -------------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill Tree")
	TArray<FSFDisciplineSkillNode> SkillNodes;

	// -------------------------------------------------------------------------
	// Stat Modifier
	//
	// Applied when this discipline is active.
	// Removed when switching disciplines.
	// -------------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	TSubclassOf<UGameplayEffect> DisciplineAttributeModifierEffect;

	// -------------------------------------------------------------------------
	// Passive
	//
	// Each discipline has one persistent passive node (the tier 2 passive
	// listed in the design doc). Granted automatically when discipline unlocks
	// rather than requiring a separate purchase.
	// -------------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TSubclassOf<UGameplayAbility> SignaturePassiveAbility;

	// -------------------------------------------------------------------------
	// Helpers
	// -------------------------------------------------------------------------

	/** Returns all nodes belonging to a specific tier. */
	UFUNCTION(BlueprintPure, Category = "Discipline")
	TArray<FSFDisciplineSkillNode> GetNodesForTier(ESFDisciplineTier Tier) const;

	/** Returns true if all nodes in the given tier are purchased. */
	UFUNCTION(BlueprintPure, Category = "Discipline")
	bool AreTier1NodesComplete(const TArray<FName>& PurchasedNodeIDs) const;

	UFUNCTION(BlueprintPure, Category = "Discipline")
	bool AreTier2NodesComplete(const TArray<FName>& PurchasedNodeIDs) const;

	/** Returns the total Discipline Points required to complete this discipline. */
	UFUNCTION(BlueprintPure, Category = "Discipline")
	int32 GetTotalPointCost() const;

	// -------------------------------------------------------------------------
	// UPrimaryDataAsset
	// -------------------------------------------------------------------------

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("SFDisciplineDefinition", GetFName());
	}

	// -------------------------------------------------------------------------
	// Data Validation
	// -------------------------------------------------------------------------

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};