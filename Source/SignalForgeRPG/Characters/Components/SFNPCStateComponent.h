#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "SFNPCStateComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FSFNPCStateTagChangedSignature,
	FGameplayTag, Tag,
	bool, bAdded);

UCLASS(ClassGroup = (SignalForge), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class SIGNALFORGERPG_API USFNPCStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFNPCStateComponent();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|State")
	FGameplayTagContainer PersistentStateTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|State")
	FGameplayTagContainer RuntimeStateTags;

public:
	UPROPERTY(BlueprintAssignable, Category = "NPC|State")
	FSFNPCStateTagChangedSignature OnStateTagChanged;

	UFUNCTION(BlueprintCallable, Category = "NPC|State")
	void AddStateTag(FGameplayTag Tag, bool bPersistent = false);

	UFUNCTION(BlueprintCallable, Category = "NPC|State")
	void RemoveStateTag(FGameplayTag Tag, bool bPersistent = false);

	UFUNCTION(BlueprintCallable, Category = "NPC|State")
	bool HasTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable, Category = "NPC|State")
	bool HasAnyTags(const FGameplayTagContainer& Tags) const;

	UFUNCTION(BlueprintCallable, Category = "NPC|State")
	bool HasAllTags(const FGameplayTagContainer& Tags) const;

	UFUNCTION(BlueprintCallable, Category = "NPC|State")
	FGameplayTagContainer GetCombinedStateTags() const;

	UFUNCTION(BlueprintCallable, Category = "NPC|State")
	void ResetRuntimeState();
};