#include "AI/BehaviorTree/SFBTDecorator_NeedsReload.h"

#include "AI/BehaviorTree/SFBTHelpers.h"
#include "AIController.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Characters/SFCharacterBase.h"
#include "Combat/SFWeaponData.h"
#include "Combat/SFWeaponInstanceTypes.h"
#include "Components/SFEquipmentComponent.h"
#include "Core/SignalForgeGameplayTags.h"

USFBTDecorator_NeedsReload::USFBTDecorator_NeedsReload()
{
	NodeName = TEXT("SF Needs Reload");
}

bool USFBTDecorator_NeedsReload::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* /*NodeMemory*/) const
{
	ASFCharacterBase* Self = SFBTHelpers::GetControlledCharacter(OwnerComp.GetAIOwner());
	if (!Self)
	{
		return false;
	}

	const USFEquipmentComponent* Equipment = Self->GetEquipmentComponent();
	if (!Equipment)
	{
		return false;
	}

	const USFWeaponData* WeaponData = Equipment->GetCurrentWeaponData();
	if (!WeaponData)
	{
		return false;
	}

	// Only weapons with a real clip can need reloading. Beams / melee / casters
	// authored with ClipSize <= 0 are unlimited from the ammo system's view.
	const int32 ClipSize = WeaponData->AmmoConfig.ClipSize;
	if (ClipSize <= 0)
	{
		return false;
	}

	const FSFWeaponInstanceData Current = Equipment->GetCurrentWeaponInstance();

	// Already reloading: report false so the BT doesn't re-enter the reload
	// branch and stomp the running ability. SFBTTask_Reload handles the
	// "already reloading" case on its own when explicitly invoked.
	if (USFAbilitySystemComponent* ASC = Cast<USFAbilitySystemComponent>(Self->GetAbilitySystemComponent()))
	{
		if (ASC->HasMatchingGameplayTag(FSignalForgeGameplayTags::Get().State_Weapon_Reloading))
		{
			return false;
		}
	}

	return Current.AmmoInClip <= AmmoThreshold;
}

FString USFBTDecorator_NeedsReload::GetStaticDescription() const
{
	return FString::Printf(TEXT("Needs Reload (AmmoInClip <= %d)"), AmmoThreshold);
}
