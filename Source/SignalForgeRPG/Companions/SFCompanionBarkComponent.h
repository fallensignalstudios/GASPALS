#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Companions/SFCompanionTypes.h"
#include "GameplayTagContainer.h"
#include "SFCompanionBarkComponent.generated.h"

class ASFCompanionCharacter;
class USFCompanionTacticsComponent;
class USFNarrativeComponent;
class ASFPlayerState;

/**
 * One bark line. Designers fill these as data; the runtime picks one
 * matching a fired BarkTag and respects per-tag cooldowns + approval
 * gates so silent / friendly / loyal barks can layer on the same
 * trigger without conflict.
 */
USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFCompanionBarkLine
{
	GENERATED_BODY()

	/** Trigger this line answers to (e.g. Bark.Combat.LowHealth.Player). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bark", meta = (Categories = "Bark"))
	FGameplayTag BarkTag;

	/** Subtitle / debug text. The actual VO asset is referenced via VoiceCue. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bark", meta = (MultiLine = true))
	FText Line;

	/** Optional dialogue cue / wave / metasound asset. Stays a soft ref so we don't pull audio chunks at load. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bark")
	TSoftObjectPtr<UObject> VoiceCue;

	/** Minimum approval required for this line to be eligible. Negative = unconditional / hostile lines. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bark")
	int32 MinApproval = -1000;

	/** Selection weight when multiple lines tie; higher = picked more often. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bark", meta = (ClampMin = "0.01"))
	float Weight = 1.f;

	/** Optional facts gating: all of these must be true on the player narrative component. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bark", meta = (Categories = "Fact"))
	FGameplayTagContainer RequiredFactTags;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FSFCompanionBarkPlayed,
	FGameplayTag, BarkTag,
	const FSFCompanionBarkLine&, Line);

/**
 * USFCompanionBarkComponent
 *
 * Lives on the companion pawn. Listens to:
 *   - Tactics: stance changes, order issued, threshold crossings
 *   - PlayerState narrative: world fact changes, faction shifts, ending
 *   - Pawn: death (handled by NPC pipeline; bark fires before broadcast)
 *
 * On each event, derives a BarkTag, picks a matching FSFCompanionBarkLine
 * from BarkLines (filtered by approval + facts), respects per-tag
 * cooldowns, and broadcasts OnBarkPlayed for UI / VO playback to consume.
 *
 * Server-authoritative selection so all peers hear the same line; the
 * delegate broadcasts on owning client too once the chosen line replicates
 * via a multicast helper if desired (Layer 3).
 */
UCLASS(ClassGroup = (Companion), meta = (BlueprintSpawnableComponent))
class SIGNALFORGERPG_API USFCompanionBarkComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFCompanionBarkComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Authority-only entry point. Picks a matching line and broadcasts. Safe to call from any system. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Companion|Bark")
	bool TryPlayBark(FGameplayTag BarkTag);

	UPROPERTY(BlueprintAssignable, Category = "Companion|Bark")
	FSFCompanionBarkPlayed OnBarkPlayed;

	/** Designer-authored line table. Multiple lines may share a BarkTag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Bark")
	TArray<FSFCompanionBarkLine> BarkLines;

	/** Per-tag cooldown floor in seconds. Prevents the same trigger spamming. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Bark", meta = (ClampMin = "0.0"))
	float DefaultBarkCooldown = 8.0f;

protected:
	UPROPERTY(Transient)
	TObjectPtr<ASFCompanionCharacter> CompanionOwner = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USFCompanionTacticsComponent> Tactics = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ASFPlayerState> CachedPlayerState = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USFNarrativeComponent> CachedPlayerNarrative = nullptr;

	/** Map of BarkTag -> last fired game-time. */
	TMap<FGameplayTag, double> LastFiredByTag;

	void BindTacticsHooks();
	void UnbindTacticsHooks();
	void BindNarrativeHooks();
	void UnbindNarrativeHooks();
	void TryResolvePlayerNarrative();

	UFUNCTION()
	void HandleStanceChanged(ESFCompanionStance OldStance, ESFCompanionStance NewStance);

	UFUNCTION()
	void HandleOrderIssued(const FSFCompanionOrder& NewOrder);

	UFUNCTION()
	void HandleThresholdCrossed(FGameplayTag ThresholdTag, bool bNowActive);

	UFUNCTION()
	void HandleWorldFactChanged(FGameplayTag FactTag, FName ContextId, const struct FSFWorldFactValue& NewValue);

	UFUNCTION()
	void HandleFactionStandingChanged(FGameplayTag FactionTag, float OldValue, float NewValue);

	bool LineEligible(const FSFCompanionBarkLine& Line) const;
};
