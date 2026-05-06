#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Companions/SFCompanionTypes.h"
#include "GameplayTagContainer.h"
#include "SFCompanionComponent.generated.h"

class ASFCompanionCharacter;

/**
 * Approval entry — one per recruited companion. Replicated as part of
 * the FSFCompanionRosterEntry list.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFCompanionRosterEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Companion")
	FName CompanionId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Companion")
	int32 Approval = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Companion")
	bool bRecruited = false;

	UPROPERTY(BlueprintReadOnly, Category = "Companion")
	bool bAvailable = true; // false when wounded/locked out

	/** Personal-quest asset to unlock at threshold. Mirrored from the pawn at recruit time. */
	UPROPERTY(BlueprintReadOnly, Category = "Companion")
	FPrimaryAssetId PersonalQuestAssetId;

	/** Approval threshold for the loyalty unlock. Mirrored from the pawn at recruit time. */
	UPROPERTY(BlueprintReadOnly, Category = "Companion")
	int32 PersonalQuestApprovalThreshold = 50;

	/** Whether to auto-start the personal quest on threshold cross. Mirrored from the pawn at recruit time. */
	UPROPERTY(BlueprintReadOnly, Category = "Companion")
	bool bAutoStartPersonalQuest = false;

	/** True once the loyalty fact has been latched so we don't re-fire on minor approval wobble. */
	UPROPERTY(BlueprintReadOnly, Category = "Companion")
	bool bLoyaltyUnlocked = false;

	bool operator==(const FSFCompanionRosterEntry& Other) const
	{
		return CompanionId == Other.CompanionId;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FSFActiveCompanionChanged,
	ASFCompanionCharacter*, OldActive,
	ASFCompanionCharacter*, NewActive);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FSFCompanionApprovalChanged,
	FName, CompanionId,
	int32, OldApproval,
	int32, NewApproval);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSFCompanionRosterChanged);

/**
 * USFCompanionComponent
 *
 * Lives on ASFPlayerState. Owns the player's roster of recruited
 * companions, the currently active one (single-active by design), and
 * the per-companion approval map.
 *
 * Player input (radial wheel, hotkeys) calls into this component on the
 * server (or directly on listen server / SP). The component then:
 *   - Looks up the active companion pawn
 *   - Forwards stance / order changes to that pawn's
 *     USFCompanionTacticsComponent
 *
 * Approval is stored here, not on the companion pawn, because pawns can
 * be unspawned when they're not active and we still need to remember
 * how the player feels about them.
 */
UCLASS(ClassGroup = (Companion), meta = (BlueprintSpawnableComponent))
class SIGNALFORGERPG_API USFCompanionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFCompanionComponent();

	//~ Begin UActorComponent
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End

	// -------------------------------------------------------------------------
	// Roster / recruitment
	// -------------------------------------------------------------------------

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Companion|Roster")
	void RecruitCompanion(FName CompanionId);

	/**
	 * Authority-only. Recruits and mirrors narrative metadata
	 * (PersonalQuestAssetId, threshold, auto-start) from the pawn so the
	 * roster entry can drive loyalty unlocks even when the pawn is not
	 * spawned later.
	 */
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Companion|Roster")
	void RecruitCompanionFromPawn(ASFCompanionCharacter* CompanionPawn);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Companion|Roster")
	void DismissCompanion(FName CompanionId);

	UFUNCTION(BlueprintPure, Category = "Companion|Roster")
	bool IsRecruited(FName CompanionId) const;

	UFUNCTION(BlueprintPure, Category = "Companion|Roster")
	const TArray<FSFCompanionRosterEntry>& GetRoster() const { return Roster; }

	// -------------------------------------------------------------------------
	// Active companion (single-active design)
	// -------------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Companion|Active")
	ASFCompanionCharacter* GetActiveCompanion() const { return ActiveCompanion; }

	/** Sets the live pawn that the player's commands should target. Authority-only. */
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Companion|Active")
	void SetActiveCompanion(ASFCompanionCharacter* NewActive);

	// -------------------------------------------------------------------------
	// Player command surface — these are what the radial wheel calls.
	// All forward to the active companion's USFCompanionTacticsComponent.
	// -------------------------------------------------------------------------

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Companion|Command")
	void CommandSetStance(ESFCompanionStance NewStance);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Companion|Command")
	void CommandSetAggression(ESFCompanionAggression NewAggression);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Companion|Command")
	void CommandFollow();

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Companion|Command")
	void CommandHoldPosition();

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Companion|Command")
	void CommandMoveTo(FVector Location);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Companion|Command")
	void CommandAttackTarget(AActor* Target);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Companion|Command")
	void CommandFocusFire(AActor* Target);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Companion|Command")
	void CommandUseAbility(FGameplayTag AbilityTag, AActor* OptionalTarget);

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Companion|Command")
	void CommandRetreatToPlayer();

	// -------------------------------------------------------------------------
	// Radial wheel UX — slow-mo enter/exit
	// -------------------------------------------------------------------------

	/** Enters command mode: applies global time dilation. Local-only effect by design (singleplayer). */
	UFUNCTION(BlueprintCallable, Category = "Companion|Radial")
	void BeginCommandMode();

	UFUNCTION(BlueprintCallable, Category = "Companion|Radial")
	void EndCommandMode();

	UFUNCTION(BlueprintPure, Category = "Companion|Radial")
	bool IsInCommandMode() const { return bInCommandMode; }

	// -------------------------------------------------------------------------
	// Approval
	// -------------------------------------------------------------------------

	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Companion|Approval")
	void ChangeApproval(FName CompanionId, int32 Delta);

	UFUNCTION(BlueprintPure, Category = "Companion|Approval")
	int32 GetApproval(FName CompanionId) const;

	UFUNCTION(BlueprintPure, Category = "Companion|Approval")
	bool IsLoyaltyUnlocked(FName CompanionId) const;

	// -------------------------------------------------------------------------
	// Save / load via narrative facts
	//
	// Roster + approval state piggybacks on the narrative system's existing
	// fact pipeline so save/load works without adding a new SaveGame type.
	// Facts written per companion (ContextId = CompanionId):
	//   Fact.Companion.Recruited        (Bool)
	//   Fact.Companion.Approval         (Int)
	//   Fact.Companion.LoyaltyUnlocked  (Bool)
	// -------------------------------------------------------------------------

	/** Authority-only. Writes every roster entry to the player's narrative facts. */
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Companion|Save")
	void PersistRosterToNarrative();

	/** Authority-only. Restores roster entries from the player's narrative facts. Called from OnLoadComplete. */
	UFUNCTION(BlueprintAuthorityOnly, BlueprintCallable, Category = "Companion|Save")
	void RestoreRosterFromNarrative();

	// -------------------------------------------------------------------------
	// Delegates
	// -------------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Companion|Active")
	FSFActiveCompanionChanged OnActiveCompanionChanged;

	UPROPERTY(BlueprintAssignable, Category = "Companion|Approval")
	FSFCompanionApprovalChanged OnApprovalChanged;

	UPROPERTY(BlueprintAssignable, Category = "Companion|Roster")
	FSFCompanionRosterChanged OnRosterChanged;

	// -------------------------------------------------------------------------
	// Tunables
	// -------------------------------------------------------------------------

	/** Time dilation while in command mode. 0.2 = 5x slow-mo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Radial", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float CommandModeTimeDilation = 0.2f;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Roster)
	TArray<FSFCompanionRosterEntry> Roster;

	UPROPERTY(ReplicatedUsing = OnRep_ActiveCompanion)
	TObjectPtr<ASFCompanionCharacter> ActiveCompanion;

	bool bInCommandMode = false;

	UFUNCTION()
	void OnRep_Roster();

	UFUNCTION()
	void OnRep_ActiveCompanion(ASFCompanionCharacter* OldActive);

	int32 FindRosterIndex(FName CompanionId) const;
	FSFCompanionRosterEntry* FindRosterEntry(FName CompanionId);

	/** Resolves the player's narrative component (owner = PlayerState). */
	class USFNarrativeComponent* GetPlayerNarrative() const;

	/** Checks the entry's approval against its threshold and unlocks the loyalty fact / starts the quest if crossed. */
	void CheckLoyaltyUnlock(FSFCompanionRosterEntry& Entry);

	/** Writes Fact.Companion.{Recruited,Approval,LoyaltyUnlocked} for one entry. */
	void WriteEntryFacts(const FSFCompanionRosterEntry& Entry);

	UFUNCTION()
	void HandleNarrativeLoadComplete(const FString& SlotName);
};
