#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Inventory/SFInventoryDisplayTypes.h"
#include "Inventory/SFInventoryWidgetController.h"
#include "SFInventoryFunctionLibrary.generated.h"

class AActor;
class APawn;
class APlayerController;
class APlayerState;
class USFInventoryComponent;
class USFEquipmentComponent;
class USFItemDefinition;

UCLASS()
class SIGNALFORGERPG_API USFInventoryFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Inventory", meta = (DefaultToSelf = "Target"))
	static USFInventoryComponent* GetInventoryComponentFromTarget(AActor* Target);

	UFUNCTION(BlueprintPure, Category = "Inventory", meta = (DefaultToSelf = "Target"))
	static USFEquipmentComponent* GetEquipmentComponentFromTarget(AActor* Target);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	static TArray<FSFInventoryDisplayEntry> SortDisplayEntries(
		const TArray<FSFInventoryDisplayEntry>& InEntries,
		ESFInventoryDisplaySortMode SortMode,
		bool bEquippedFirst = false);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	static bool FindDisplayEntryByGuid(
		const TArray<FSFInventoryDisplayEntry>& InEntries,
		FGuid EntryGuid,
		FSFInventoryDisplayEntry& OutEntry);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	static int32 GetDisplayIndexByGuid(
		const TArray<FSFInventoryDisplayEntry>& InEntries,
		FGuid EntryGuid);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	static bool ContainsDisplayEntryGuid(
		const TArray<FSFInventoryDisplayEntry>& InEntries,
		FGuid EntryGuid);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	static TArray<FSFInventoryDisplayEntry> FilterDisplayEntriesToEquippable(
		const TArray<FSFInventoryDisplayEntry>& InEntries);

private:
	static USFInventoryComponent* FindInventoryComponentOnActor(const AActor* Target);
	static USFEquipmentComponent* FindEquipmentComponentOnActor(const AActor* Target);
	static int32 CompareDisplayEntries(
		const FSFInventoryDisplayEntry& A,
		const FSFInventoryDisplayEntry& B,
		ESFInventoryDisplaySortMode SortMode,
		bool bEquippedFirst);
};