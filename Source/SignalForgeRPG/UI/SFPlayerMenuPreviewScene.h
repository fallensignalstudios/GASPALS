#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SFPlayerMenuPreviewScene.generated.h"

class UCameraComponent;
class USceneComponent;
class ASFMenuPreviewCharacter;

UCLASS()
class SIGNALFORGERPG_API ASFPlayerMenuPreviewScene : public AActor
{
	GENERATED_BODY()

public:
	ASFPlayerMenuPreviewScene();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preview")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preview")
	TObjectPtr<USceneComponent> PreviewCharacterAnchor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preview")
	TObjectPtr<UCameraComponent> PreviewCamera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Preview")
	TSubclassOf<ASFMenuPreviewCharacter> PreviewCharacterClass;

	UPROPERTY()
	TObjectPtr<ASFMenuPreviewCharacter> SpawnedPreviewCharacter;

public:
	UFUNCTION(BlueprintCallable, Category = "Preview")
	ASFMenuPreviewCharacter* GetOrSpawnPreviewCharacter();

	UFUNCTION(BlueprintPure, Category = "Preview")
	UCameraComponent* GetPreviewCamera() const
	{
		return PreviewCamera;
	}

	UFUNCTION(BlueprintPure, Category = "Preview")
	AActor* GetViewTargetActor() const
	{
		return const_cast<ASFPlayerMenuPreviewScene*>(this);
	}
};