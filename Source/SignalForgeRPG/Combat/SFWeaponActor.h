#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Combat/SFWeaponInstanceTypes.h"
#include "SFWeaponActor.generated.h"

class UStaticMeshComponent;
class USkeletalMeshComponent;
class USFWeaponData;
class USceneComponent;
class UPrimitiveComponent;

UCLASS()
class SIGNALFORGERPG_API ASFWeaponActor : public AActor
{
	GENERATED_BODY()

public:
	ASFWeaponActor();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USFWeaponData> WeaponData;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon")
	FSFWeaponInstanceData WeaponInstanceData;

public:
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void InitializeFromWeaponData(USFWeaponData* InWeaponData);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void InitializeFromWeaponInstance(const FSFWeaponInstanceData& InInstanceData);

	/** Call AFTER InitializeFromWeaponData on an offhand actor to swap its visuals to the
	 *  OffhandSkeletalWeaponMesh / OffhandStaticWeaponMesh on the supplied data. If both offhand
	 *  mesh slots are null, this is a no-op and the actor keeps the mainhand mesh (symmetric pair). */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ApplyOffhandMeshOverride(USFWeaponData* InWeaponData);

	UFUNCTION(BlueprintPure, Category = "Weapon")
	USFWeaponData* GetWeaponData() const
	{
		return WeaponData;
	}

	UFUNCTION(BlueprintPure, Category = "Weapon")
	const FSFWeaponInstanceData& GetWeaponInstanceData() const
	{
		return WeaponInstanceData;
	}

	UFUNCTION(BlueprintPure, Category = "Weapon")
	UStaticMeshComponent* GetStaticMeshComponent() const
	{
		return StaticMeshComponent;
	}

	UFUNCTION(BlueprintPure, Category = "Weapon")
	USkeletalMeshComponent* GetSkeletalMeshComponent() const
	{
		return SkeletalMeshComponent;
	}

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Trace")
	FName TraceStartSocketName = TEXT("Trace_Start");

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Trace")
	FName TraceEndSocketName = TEXT("Trace_End");

	UFUNCTION(BlueprintPure, Category = "Weapon|Trace")
	bool HasValidTraceSockets() const;

	UFUNCTION(BlueprintPure, Category = "Weapon|Trace")
	FVector GetTraceStartLocation() const;

	UFUNCTION(BlueprintPure, Category = "Weapon|Trace")
	FVector GetTraceEndLocation() const;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Muzzle")
	FName MuzzleSocketName = TEXT("Muzzle");

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Muzzle")
	FName MuzzleFXSocketName = TEXT("MuzzleFX");

	UFUNCTION(BlueprintPure, Category = "Weapon|Muzzle")
	bool HasValidMuzzleSocket() const;

	UFUNCTION(BlueprintPure, Category = "Weapon|Muzzle")
	FVector GetMuzzleLocation() const;

	UFUNCTION(BlueprintPure, Category = "Weapon|Muzzle")
	FRotator GetMuzzleRotation() const;

	UFUNCTION(BlueprintPure, Category = "Weapon|Muzzle")
	bool HasValidMuzzleFXSocket() const;

	UFUNCTION(BlueprintPure, Category = "Weapon|Muzzle")
	FName GetMuzzleFXSocketName() const
	{
		return MuzzleFXSocketName;
	}

	UFUNCTION(BlueprintPure, Category = "Weapon")
	UPrimitiveComponent* GetActiveVisualComponent() const;

protected:
	void ApplyWeaponVisuals();
	void ClearWeaponVisuals();
};