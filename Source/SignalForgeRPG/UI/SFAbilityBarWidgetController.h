// SFAbilityBarWidgetController.h

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "UI/SFAbilitySlotUIData.h"
#include "SFAbilityBarWidgetController.generated.h"

class ASFCharacterBase;
class USFAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnAbilitySlotUpdated,
	FGameplayTag,
	InputTag,
	FSFAbilitySlotUIData,
	SlotData);

UCLASS(BlueprintType, Blueprintable)
class SIGNALFORGERPG_API USFAbilityBarWidgetController : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UI")
	void Initialize(ASFCharacterBase* InPlayerCharacter);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void RefreshAbilitySlots();

	UFUNCTION(BlueprintPure, Category = "UI")
	FSFAbilitySlotUIData GetSlotData(FGameplayTag InputTag) const;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnAbilitySlotUpdated OnAbilitySlotUpdated;

protected:
	void HandleAbilitiesChanged();
	void HandleCooldownTagChanged(FGameplayTag ChangedTag, int32 NewCount);

	void RebuildFromASC();
	void BroadcastAllSlots();
	void BroadcastSlot(const FGameplayTag& InputTag);

	void UnbindCooldownDelegates();
	void RebuildCooldownListeners();

protected:
	UPROPERTY()
	TObjectPtr<ASFCharacterBase> PlayerCharacter = nullptr;

	UPROPERTY()
	TObjectPtr<USFAbilitySystemComponent> AbilitySystemComponent = nullptr;

	UPROPERTY()
	TMap<FGameplayTag, FSFAbilitySlotUIData> SlotDataMap;

	UPROPERTY()
	TMap<FGameplayTag, FGameplayTag> CooldownTagByInputTag;

	TMap<FGameplayTag, FDelegateHandle> CooldownDelegateHandles;
};