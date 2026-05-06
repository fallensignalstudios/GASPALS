#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Characters/SFNPCBase.h"
#include "SFNPCNarrativeComponent.generated.h"

class USFNPCStateComponent;
class USFNPCGoalComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FSFNarrativeEventAppliedSignature,
	const FSFNarrativeEventPayload&, Payload);

UCLASS(ClassGroup = (SignalForge), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class SIGNALFORGERPG_API USFNPCNarrativeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFNPCNarrativeComponent();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Narrative")
	FGameplayTagContainer KnownNarrativeTags;

public:
	UPROPERTY(BlueprintAssignable, Category = "NPC|Narrative")
	FSFNarrativeEventAppliedSignature OnNarrativeEventApplied;

	UFUNCTION(BlueprintCallable, Category = "NPC|Narrative")
	void ApplyNarrativeEvent(const FSFNarrativeEventPayload& Payload);

	UFUNCTION(BlueprintCallable, Category = "NPC|Narrative")
	bool KnowsNarrativeTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable, Category = "NPC|Narrative")
	const FGameplayTagContainer& GetKnownNarrativeTags() const { return KnownNarrativeTags; }
};