#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Dialogue/SFDialogueCameraTypes.h"
#include "SFDialogueCameraComponent.generated.h"

class AActor;
class APlayerController;
class USceneComponent;
class UCameraComponent;
class USFDialogueComponent;
class ULevelSequencePlayer;
class ALevelSequenceActor;

UENUM()
enum class ESFActiveDialogueCameraMode : uint8
{
	None,
	ProceduralCamera,
	CameraActor,
	LevelSequence
};

UCLASS(ClassGroup = (SignalForge), meta = (BlueprintSpawnableComponent))
class SIGNALFORGERPG_API USFDialogueCameraComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USFDialogueCameraComponent();

	UFUNCTION(BlueprintCallable, Category = "Dialogue Camera")
	void InitializeFromDialogueComponent(USFDialogueComponent* InDialogueComponent);

	UFUNCTION(BlueprintPure, Category = "Dialogue Camera")
	UCameraComponent* GetDialogueCamera() const { return DialogueCamera; }

	UFUNCTION(BlueprintPure, Category = "Dialogue Camera")
	bool IsUsingDialogueCamera() const { return bUsingDialogueCamera; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(Transient)
	TObjectPtr<USFDialogueComponent> DialogueComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> DialogueCameraRoot = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> DialogueCamera = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> CachedPlayerController = nullptr;

	UPROPERTY(Transient)
	ESFActiveDialogueCameraMode ActiveCameraMode = ESFActiveDialogueCameraMode::None;

	UPROPERTY(Transient)
	bool bUsingDialogueCamera = false;

	UPROPERTY(Transient)
	TObjectPtr<ULevelSequencePlayer> ActiveSequencePlayer = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ALevelSequenceActor> ActiveSequenceActor = nullptr;

protected:
	UFUNCTION()
	void HandleDialogueCameraShotChanged(const FSFDialogueCameraShot& CameraShot);

	UFUNCTION()
	void HandleConversationEnded();

	UFUNCTION()
	void HandleSequenceFinished();

	AActor* ResolveShotTarget(const FSFDialogueCameraShot& CameraShot) const;
	bool ResolveSocketTransform(AActor* TargetActor, const FName SocketName, FTransform& OutTransform) const;

	void ApplyShot(const FSFDialogueCameraShot& CameraShot);
	void ApplyProceduralShot(const FSFDialogueCameraShot& CameraShot);
	void ApplyCameraActorShot(const FSFDialogueCameraShot& CameraShot);
	void ApplyLevelSequenceShot(const FSFDialogueCameraShot& CameraShot);

	void RestoreGameplayCamera();
	void CachePlayerController();
	void StopActiveSequence();
};