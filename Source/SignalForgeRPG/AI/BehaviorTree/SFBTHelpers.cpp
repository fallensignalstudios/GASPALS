#include "AI/BehaviorTree/SFBTHelpers.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/SFCharacterBase.h"
#include "Combat/SFWeaponData.h"
#include "Components/SFEquipmentComponent.h"
#include "Engine/World.h"
#include "Faction/SFFactionStatics.h"

namespace SFBTHelpers
{
	ASFCharacterBase* GetControlledCharacter(AAIController* AIOwner)
	{
		if (!AIOwner)
		{
			return nullptr;
		}
		return Cast<ASFCharacterBase>(AIOwner->GetPawn());
	}

	AActor* GetBBActor(UBlackboardComponent* BB, FName KeyName)
	{
		if (!BB || KeyName.IsNone())
		{
			return nullptr;
		}
		return Cast<AActor>(BB->GetValueAsObject(KeyName));
	}

	bool GetBBVector(UBlackboardComponent* BB, FName KeyName, FVector& OutVec)
	{
		OutVec = FVector::ZeroVector;
		if (!BB || KeyName.IsNone())
		{
			return false;
		}
		OutVec = BB->GetValueAsVector(KeyName);
		return !OutVec.IsNearlyZero();
	}

	bool IsHostile(const AActor* Self, const AActor* Target)
	{
		if (!Self || !Target || Self == Target)
		{
			return false;
		}
		return USFFactionStatics::AreHostile(Self, Target);
	}

	float GetEquippedWeaponRange(const ASFCharacterBase* Character)
	{
		if (!Character)
		{
			return 0.0f;
		}
		const USFEquipmentComponent* Equipment = Character->GetEquipmentComponent();
		if (!Equipment)
		{
			return 0.0f;
		}
		const USFWeaponData* WeaponData = Equipment->GetCurrentWeaponData();
		if (!WeaponData)
		{
			return 0.0f;
		}

		// Prefer ranged hitscan range if the weapon has any ranged config authored.
		const FSFRangedWeaponConfig& Ranged = WeaponData->RangedConfig;
		if (Ranged.HitscanMaxRange > 0.0f)
		{
			return Ranged.HitscanMaxRange;
		}
		// Projectile weapons: use the falloff end as a reasonable engage horizon.
		if (Ranged.FalloffEndDistance > 0.0f)
		{
			return Ranged.FalloffEndDistance;
		}
		// Caster fallback: most casters launch projectiles ~6000-12000cm; use 6000 as a
		// conservative engagement distance unless a designer wants to override.
		if (WeaponData->CasterConfig.ProjectileClass != nullptr)
		{
			return 6000.0f;
		}
		// Melee fallback: small range (the combat component traces ~150-250cm; pick 250).
		return 250.0f;
	}

	bool HasLineOfSight(const ASFCharacterBase* From, const AActor* To)
	{
		if (!From || !To)
		{
			return false;
		}
		UWorld* World = From->GetWorld();
		if (!World)
		{
			return false;
		}

		FVector EyeLoc;
		FRotator EyeRot;
		From->GetActorEyesViewPoint(EyeLoc, EyeRot);

		const FVector TargetLoc = To->GetActorLocation()
			+ FVector(0.0f, 0.0f, 60.0f); // aim roughly at chest height

		FCollisionQueryParams Params(SCENE_QUERY_STAT(SFBTLineOfSight), false, From);
		Params.AddIgnoredActor(To); // ignore the target so its own collision doesn't block

		FHitResult Hit;
		const bool bBlocked = World->LineTraceSingleByChannel(
			Hit, EyeLoc, TargetLoc, ECC_Visibility, Params);
		return !bBlocked;
	}
}
