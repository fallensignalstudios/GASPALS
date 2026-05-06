#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Misc/DataValidation.h"
#include "Combat/SFWeaponStatBlock.h"
#include "SFWeaponPerkDefinition.generated.h"

class USFWeaponData;

UENUM(BlueprintType)
enum class ESFWeaponPerkSlot : uint8
{
	None,
	Barrel,
	Magazine,
	TraitColumn1,
	TraitColumn2,
	OriginTrait,
	Masterwork
};

UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFWeaponPerkDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perk|Identity")
	FName PerkId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perk|Identity")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perk|Identity", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perk|Categorization")
	ESFWeaponPerkSlot PerkSlot = ESFWeaponPerkSlot::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perk|Categorization")
	FGameplayTag PerkTypeTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perk|Categorization")
	FGameplayTagContainer PerkTags;

	// Weapon must contain all of these tags to be eligible for this perk.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perk|Compatibility")
	FGameplayTagContainer RequiredWeaponTags;

	// Weapon must contain none of these tags to be eligible for this perk.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perk|Compatibility")
	FGameplayTagContainer BlockedWeaponTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perk|Gameplay")
	FSFWeaponStatBlock StatModifiers;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Perk|Gameplay")
	bool bSelectableAfterUnlock = false;

	UFUNCTION(BlueprintPure, Category = "Perk")
	bool IsDefinitionValid() const
	{
		return PerkId != NAME_None
			&& !DisplayName.IsEmpty()
			&& PerkSlot != ESFWeaponPerkSlot::None;
	}

	UFUNCTION(BlueprintPure, Category = "Perk")
	bool CanApplyToWeapon(const USFWeaponData* WeaponData) const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("WeaponPerkDefinition"), GetFName());
	}

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override
	{
		EDataValidationResult Result = Super::IsDataValid(Context);
		bool bHasErrors = (Result == EDataValidationResult::Invalid);
		bool bHasWarnings = false;

		if (PerkId == NAME_None)
		{
			Context.AddError(NSLOCTEXT("SFWeaponPerkDefinition", "PerkIdRequired", "PerkId must be set."));
			bHasErrors = true;
		}

		if (DisplayName.IsEmpty())
		{
			Context.AddError(NSLOCTEXT("SFWeaponPerkDefinition", "DisplayNameRequired", "DisplayName must not be empty."));
			bHasErrors = true;
		}

		if (PerkSlot == ESFWeaponPerkSlot::None)
		{
			Context.AddError(NSLOCTEXT("SFWeaponPerkDefinition", "PerkSlotRequired", "PerkSlot must be assigned."));
			bHasErrors = true;
		}

		if (!PerkTypeTag.IsValid())
		{
			Context.AddWarning(NSLOCTEXT(
				"SFWeaponPerkDefinition",
				"PerkTypeTagMissing",
				"PerkTypeTag is not set. Consider assigning a tag for filtering/querying."
			));
			bHasWarnings = true;
		}

		if (RequiredWeaponTags.HasAny(BlockedWeaponTags))
		{
			Context.AddError(NSLOCTEXT(
				"SFWeaponPerkDefinition",
				"ConflictingCompatibilityTags",
				"RequiredWeaponTags and BlockedWeaponTags overlap."
			));
			bHasErrors = true;
		}

		if (bHasErrors)
		{
			return EDataValidationResult::Invalid;
		}

		return Result == EDataValidationResult::Valid || bHasWarnings
			? EDataValidationResult::Valid
			: Result;
	}
#endif
};