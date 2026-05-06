#pragma once

#include "CoreMinimal.h"
#include "SFWeaponStatBlock.generated.h"

USTRUCT(BlueprintType)
struct SIGNALFORGERPG_API FSFWeaponStatBlock
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Damage = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Range = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Stability = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Handling = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float ReloadSpeed = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float MagazineSize = 30.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Accuracy = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float RecoilControl = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float AimAssist = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float GuardEfficiency = 0.0f;

	static FSFWeaponStatBlock Zero()
	{
		FSFWeaponStatBlock Result;
		Result.Damage = 0.0f;
		Result.Range = 0.0f;
		Result.Stability = 0.0f;
		Result.Handling = 0.0f;
		Result.ReloadSpeed = 0.0f;
		Result.MagazineSize = 0.0f;
		Result.Accuracy = 0.0f;
		Result.RecoilControl = 0.0f;
		Result.AimAssist = 0.0f;
		Result.GuardEfficiency = 0.0f;
		return Result;
	}

	void Sanitize()
	{
		Damage = FMath::Max(0.0f, Damage);
		Range = FMath::Max(0.0f, Range);
		Stability = FMath::Max(0.0f, Stability);
		Handling = FMath::Max(0.0f, Handling);
		ReloadSpeed = FMath::Max(0.0f, ReloadSpeed);
		MagazineSize = FMath::Max(0.0f, MagazineSize);
		Accuracy = FMath::Max(0.0f, Accuracy);
		RecoilControl = FMath::Max(0.0f, RecoilControl);
		AimAssist = FMath::Max(0.0f, AimAssist);
		GuardEfficiency = FMath::Max(0.0f, GuardEfficiency);
	}

	void ClampTo(const FSFWeaponStatBlock& MinValues, const FSFWeaponStatBlock& MaxValues)
	{
		Damage = FMath::Clamp(Damage, MinValues.Damage, MaxValues.Damage);
		Range = FMath::Clamp(Range, MinValues.Range, MaxValues.Range);
		Stability = FMath::Clamp(Stability, MinValues.Stability, MaxValues.Stability);
		Handling = FMath::Clamp(Handling, MinValues.Handling, MaxValues.Handling);
		ReloadSpeed = FMath::Clamp(ReloadSpeed, MinValues.ReloadSpeed, MaxValues.ReloadSpeed);
		MagazineSize = FMath::Clamp(MagazineSize, MinValues.MagazineSize, MaxValues.MagazineSize);
		Accuracy = FMath::Clamp(Accuracy, MinValues.Accuracy, MaxValues.Accuracy);
		RecoilControl = FMath::Clamp(RecoilControl, MinValues.RecoilControl, MaxValues.RecoilControl);
		AimAssist = FMath::Clamp(AimAssist, MinValues.AimAssist, MaxValues.AimAssist);
		GuardEfficiency = FMath::Clamp(GuardEfficiency, MinValues.GuardEfficiency, MaxValues.GuardEfficiency);
	}

	FSFWeaponStatBlock GetClamped(const FSFWeaponStatBlock& MinValues, const FSFWeaponStatBlock& MaxValues) const
	{
		FSFWeaponStatBlock Result = *this;
		Result.ClampTo(MinValues, MaxValues);
		return Result;
	}

	FSFWeaponStatBlock operator+(const FSFWeaponStatBlock& Other) const
	{
		FSFWeaponStatBlock Result = *this;
		Result.Damage += Other.Damage;
		Result.Range += Other.Range;
		Result.Stability += Other.Stability;
		Result.Handling += Other.Handling;
		Result.ReloadSpeed += Other.ReloadSpeed;
		Result.MagazineSize += Other.MagazineSize;
		Result.Accuracy += Other.Accuracy;
		Result.RecoilControl += Other.RecoilControl;
		Result.AimAssist += Other.AimAssist;
		Result.GuardEfficiency += Other.GuardEfficiency;
		return Result;
	}

	FSFWeaponStatBlock& operator+=(const FSFWeaponStatBlock& Other)
	{
		*this = *this + Other;
		return *this;
	}

	FSFWeaponStatBlock operator-(const FSFWeaponStatBlock& Other) const
	{
		FSFWeaponStatBlock Result = *this;
		Result.Damage -= Other.Damage;
		Result.Range -= Other.Range;
		Result.Stability -= Other.Stability;
		Result.Handling -= Other.Handling;
		Result.ReloadSpeed -= Other.ReloadSpeed;
		Result.MagazineSize -= Other.MagazineSize;
		Result.Accuracy -= Other.Accuracy;
		Result.RecoilControl -= Other.RecoilControl;
		Result.AimAssist -= Other.AimAssist;
		Result.GuardEfficiency -= Other.GuardEfficiency;
		return Result;
	}

	FSFWeaponStatBlock operator*(float Scalar) const
	{
		FSFWeaponStatBlock Result = *this;
		Result.Damage *= Scalar;
		Result.Range *= Scalar;
		Result.Stability *= Scalar;
		Result.Handling *= Scalar;
		Result.ReloadSpeed *= Scalar;
		Result.MagazineSize *= Scalar;
		Result.Accuracy *= Scalar;
		Result.RecoilControl *= Scalar;
		Result.AimAssist *= Scalar;
		Result.GuardEfficiency *= Scalar;
		return Result;
	}
};