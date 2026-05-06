#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SFMenuPreviewCharacter.generated.h"

class USFEquipmentComponent;
class USkeletalMeshComponent;
class ACharacter;

UCLASS()
class SIGNALFORGERPG_API ASFMenuPreviewCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASFMenuPreviewCharacter();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preview")
	TObjectPtr<USFEquipmentComponent> PreviewEquipmentComponent;

public:
	UFUNCTION(BlueprintCallable, Category = "Preview")
	void SyncFromSourceCharacter(ACharacter* SourceCharacter);

	UFUNCTION(BlueprintCallable, Category = "Preview")
	void SyncEquipmentFromSourceCharacter(ACharacter* SourceCharacter);

	UFUNCTION(BlueprintPure, Category = "Preview")
	USFEquipmentComponent* GetPreviewEquipmentComponent() const
	{
		return PreviewEquipmentComponent;
	}

	UFUNCTION(BlueprintImplementableEvent, Category = "Preview")
	void BP_OnPreviewSynced();

	UFUNCTION(BlueprintCallable, Category = "Preview")
	void SetPreviewRotation(float YawDegrees);
};