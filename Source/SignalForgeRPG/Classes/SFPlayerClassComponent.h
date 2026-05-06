// SFPlayerClassComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "ActiveGameplayEffectHandle.h"
#include "Classes/SFClassDefinition.h"
#include "Classes/SFDisciplineDefinition.h"
#include "SFPlayerClassComponent.generated.h"

class USFAbilitySystemComponent;
class ASFCharacterBase;

// -----------------------------------------------------------------------------
// Delegates
// -----------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnClassAssignedSignature,
	USFClassDefinition*,
	NewClass
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnDisciplineAssignedSignature,
	USFDisciplineDefinition*,
	NewDiscipline
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnDisciplinePointsChangedSignature,
	int32,
	NewAmount
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnSkillNodePurchasedSignature,
	USFDisciplineDefinition*,
	Discipline,
	FName,
	NodeID
);

// -----------------------------------------------------------------------------
// Saved discipline progress
// -----------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FSFDisciplineProgress
{
	GENERATED_BODY()

	/** Which discipline this progress belongs to. */
	UPROPERTY(BlueprintReadOnly, Category = "Discipline")
	TObjectPtr<USFDisciplineDefinition> Discipline = nullptr;

	/** Node IDs the player has purchased in this discipline. */
	UPROPERTY(BlueprintReadOnly, Category = "Discipline")
	TArray<FName> PurchasedNodeIDs;

	/** Runtime ability handles for granted node abilities. Not intended for save serialization. */
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;
};

// -----------------------------------------------------------------------------
// USFPlayerClassComponent
// -----------------------------------------------------------------------------

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SIGNALFORGERPG_API USFPlayerClassComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFPlayerClassComponent();

	// -------------------------------------------------------------------------
	// Class assignment
	// -------------------------------------------------------------------------

	/**
	 * Assigns a class to the character. Removes the previous class first,
	 * grants base class abilities, applies the class attribute modifier,
	 * and automatically assigns the default discipline.
	 */
	UFUNCTION(BlueprintCallable, Category = "Class")
	void AssignClass(USFClassDefinition* NewClass, bool bIsFirstTimeSetup = false);

	/** Removes the current class, discipline, abilities, and modifiers. */
	UFUNCTION(BlueprintCallable, Category = "Class")
	void RemoveCurrentClass();

	UFUNCTION(BlueprintPure, Category = "Class")
	USFClassDefinition* GetCurrentClass() const
	{
		return CurrentClass;
	}

	UFUNCTION(BlueprintPure, Category = "Class")
	bool HasClass() const
	{
		return CurrentClass != nullptr;
	}

	// -------------------------------------------------------------------------
	// Discipline assignment
	// -------------------------------------------------------------------------

	/**
	 * Attempts to switch to a new discipline.
	 * Requires the player to be out of combat and meet the unlock requirement.
	 */
	UFUNCTION(BlueprintCallable, Category = "Discipline")
	bool TrySwitchDiscipline(USFDisciplineDefinition* NewDiscipline);

	/** Assigns a discipline directly without gameplay gating checks. */
	void AssignDisciplineInternal(USFDisciplineDefinition* NewDiscipline);

	UFUNCTION(BlueprintPure, Category = "Discipline")
	USFDisciplineDefinition* GetCurrentDiscipline() const
	{
		return CurrentDiscipline;
	}

	UFUNCTION(BlueprintPure, Category = "Discipline")
	bool HasDiscipline() const
	{
		return CurrentDiscipline != nullptr;
	}

	UFUNCTION(BlueprintPure, Category = "Discipline")
	bool CanSwitchDiscipline() const;

	// -------------------------------------------------------------------------
	// Skill node purchasing
	// -------------------------------------------------------------------------

	/**
	 * Attempts to purchase a skill node in the current discipline.
	 * Returns true if the purchase succeeds.
	 */
	UFUNCTION(BlueprintCallable, Category = "Discipline")
	bool TryPurchaseSkillNode(FName NodeID);

	UFUNCTION(BlueprintPure, Category = "Discipline")
	bool IsNodePurchased(FName NodeID) const;

	UFUNCTION(BlueprintPure, Category = "Discipline")
	bool IsNodeAvailable(FName NodeID) const;

	UFUNCTION(BlueprintPure, Category = "Discipline")
	TArray<FName> GetPurchasedNodes() const;

	// -------------------------------------------------------------------------
	// Discipline points
	// -------------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Currency")
	int32 GetDisciplinePoints() const
	{
		return DisciplinePoints;
	}

	UFUNCTION(BlueprintCallable, Category = "Currency")
	void AddDisciplinePoints(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Currency")
	bool SpendDisciplinePoints(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Save")
	void SetDisciplinePoints(int32 Amount);

	// -------------------------------------------------------------------------
	// Delegates
	// -------------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Class")
	FOnClassAssignedSignature OnClassAssigned;

	UPROPERTY(BlueprintAssignable, Category = "Discipline")
	FOnDisciplineAssignedSignature OnDisciplineAssigned;

	UPROPERTY(BlueprintAssignable, Category = "Currency")
	FOnDisciplinePointsChangedSignature OnDisciplinePointsChanged;

	UPROPERTY(BlueprintAssignable, Category = "Discipline")
	FOnSkillNodePurchasedSignature OnSkillNodePurchased;

	// -------------------------------------------------------------------------
	// Save / load
	// -------------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Save")
	const TArray<FSFDisciplineProgress>& GetAllDisciplineProgress() const
	{
		return DisciplineProgressList;
	}

	UFUNCTION(BlueprintCallable, Category = "Save")
	void RestoreDisciplineProgress(const TArray<FSFDisciplineProgress>& SavedProgress);

private:
	// -------------------------------------------------------------------------
	// Internal helpers
	// -------------------------------------------------------------------------

	USFAbilitySystemComponent* GetASC() const;
	ASFCharacterBase* GetOwnerCharacter() const;

	void GrantClassAbilities();
	void RemoveClassAbilities();

	void ApplyClassAttributeModifier();
	void RemoveClassAttributeModifier();

	void GrantDisciplinePassive(USFDisciplineDefinition* Discipline);
	void RemoveDisciplinePassive();

	void RemoveDisciplineNodeAbilities(FSFDisciplineProgress& Progress);

	FSFDisciplineProgress* FindOrCreateProgress(USFDisciplineDefinition* Discipline);
	FSFDisciplineProgress* FindProgress(USFDisciplineDefinition* Discipline);
	const FSFDisciplineProgress* FindProgress(USFDisciplineDefinition* Discipline) const;

	bool IsInCombat() const;
	bool MeetsLevelRequirement() const;

	const FSFDisciplineSkillNode* FindNode(
		USFDisciplineDefinition* Discipline,
		FName NodeID
	) const;

private:
	// -------------------------------------------------------------------------
	// State
	// -------------------------------------------------------------------------

	UPROPERTY()
	TObjectPtr<USFClassDefinition> CurrentClass = nullptr;

	UPROPERTY()
	TObjectPtr<USFDisciplineDefinition> CurrentDiscipline = nullptr;

	/** Handles for abilities granted by the current class. */
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> ClassAbilityHandles;

	/** Active gameplay effect handle for the class attribute modifier. */
	FActiveGameplayEffectHandle ClassAttributeModifierHandle;

	/** Handle for the current discipline passive ability. */
	UPROPERTY()
	FGameplayAbilitySpecHandle DisciplinePassiveHandle;

	/** Active gameplay effect handle for the discipline attribute modifier. */
	FActiveGameplayEffectHandle DisciplineAttributeModifierHandle;

	/** Saved progress for each unlocked/used discipline. */
	UPROPERTY()
	TArray<FSFDisciplineProgress> DisciplineProgressList;

	/** Currency used to purchase discipline nodes. */
	UPROPERTY()
	int32 DisciplinePoints = 0;
};