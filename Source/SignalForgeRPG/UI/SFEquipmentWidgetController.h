#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UI/SFEquipmentDisplayTypes.h"
#include "SFEquipmentWidgetController.generated.h"

class USFEquipmentComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquipmentDisplayDataUpdatedSignature);

UCLASS(BlueprintType, Blueprintable)
class SIGNALFORGERPG_API USFEquipmentWidgetController : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void Initialize(USFEquipmentComponent* InEquipmentComponent);

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void Deinitialize();

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void RefreshEquipmentDisplayData();

	UFUNCTION(BlueprintPure, Category = "Equipment")
	TArray<FSFEquipmentDisplayEntry> GetCurrentDisplayEntries() const
	{
		return CurrentDisplayEntries;
	}

	UFUNCTION(BlueprintPure, Category = "Equipment")
	bool GetDisplayEntryForSlot(ESFEquipmentSlot Slot, FSFEquipmentDisplayEntry& OutEntry) const;

	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FOnEquipmentDisplayDataUpdatedSignature OnEquipmentDisplayDataUpdated;

protected:
	virtual void BeginDestroy() override;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<USFEquipmentComponent> EquipmentComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	TArray<FSFEquipmentDisplayEntry> CurrentDisplayEntries;

	UFUNCTION()
	void HandleEquipmentUpdated();

	void BindToSources();
	void UnbindFromSources();
	void RebuildDisplayEntries();

	void AddEntryForSlot(
		ESFEquipmentSlot Slot,
		ESFEquipmentSlotType DisplaySlotType,
		const FText& SlotDisplayName
	);

	FSFEquipmentDisplayEntry BuildEntryForSlot(
		ESFEquipmentSlot Slot,
		ESFEquipmentSlotType DisplaySlotType,
		const FText& SlotDisplayName
	) const;
};