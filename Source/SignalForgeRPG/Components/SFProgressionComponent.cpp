#include "Components/SFProgressionComponent.h"

#include "Characters/SFCharacterBase.h"
#include "Core/SFAttributeSetBase.h"
#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "Engine/DataTable.h"

USFProgressionComponent::USFProgressionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USFProgressionComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ASFCharacterBase>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}

	CurrentLevel = FMath::Clamp(CurrentLevel, 1, MaxLevel);
	CurrentXP = FMath::Max(CurrentXP, 0);

	BroadcastXPState();
	OnLevelChanged.Broadcast(CurrentLevel, false);
}

void USFProgressionComponent::AddXP(int32 XPAmount)
{
	if (XPAmount <= 0 || IsAtMaxLevel())
	{
		return;
	}

	CurrentXP = FMath::Max(CurrentXP + XPAmount, 0);
	TryProcessLevelUps();
	BroadcastXPState();
}

void USFProgressionComponent::SetLevel(int32 NewLevel, bool bApplyStats, bool bFullRefill)
{
	const int32 ClampedLevel = FMath::Clamp(NewLevel, 1, MaxLevel);
	const bool bLeveledUp = ClampedLevel > CurrentLevel;

	CurrentLevel = ClampedLevel;

	if (bApplyStats)
	{
		ApplyLevelStats(bFullRefill);
	}

	OnLevelChanged.Broadcast(CurrentLevel, bLeveledUp);
	BroadcastXPState();
}

void USFProgressionComponent::ApplyLevelStats(bool bFullRefill)
{
	if (!OwnerCharacter)
	{
		return;
	}

	const FSFLevelDefinition* LevelDef = GetLevelDefinition(CurrentLevel);
	if (!LevelDef)
	{
		return;
	}

	UAbilitySystemComponent* ASC = OwnerCharacter->GetAbilitySystemComponent();
	const USFAttributeSetBase* Attributes = OwnerCharacter->GetAttributeSet();

	if (!ASC || !Attributes)
	{
		return;
	}

	ASC->SetNumericAttributeBase(USFAttributeSetBase::GetMaxHealthAttribute(), LevelDef->MaxHealth);
	ASC->SetNumericAttributeBase(USFAttributeSetBase::GetMaxEchoAttribute(), LevelDef->MaxEcho);
	ASC->SetNumericAttributeBase(USFAttributeSetBase::GetMaxShieldsAttribute(), LevelDef->MaxShields);
	ASC->SetNumericAttributeBase(USFAttributeSetBase::GetMaxStaminaAttribute(), LevelDef->MaxStamina);

	if (bFullRefill)
	{
		ASC->SetNumericAttributeBase(USFAttributeSetBase::GetHealthAttribute(), LevelDef->MaxHealth);
		ASC->SetNumericAttributeBase(USFAttributeSetBase::GetEchoAttribute(), LevelDef->MaxEcho);
		ASC->SetNumericAttributeBase(USFAttributeSetBase::GetShieldsAttribute(), LevelDef->MaxShields);
		ASC->SetNumericAttributeBase(USFAttributeSetBase::GetStaminaAttribute(), LevelDef->MaxStamina);
	}
	else
	{
		const float CurrentHealth = FMath::Min(Attributes->GetHealth(), LevelDef->MaxHealth);
		const float CurrentEcho = FMath::Min(Attributes->GetEcho(), LevelDef->MaxEcho);
		const float CurrentShields = FMath::Min(Attributes->GetShields(), LevelDef->MaxShields);
		const float CurrentStamina = FMath::Min(Attributes->GetStamina(), LevelDef->MaxStamina);

		ASC->SetNumericAttributeBase(USFAttributeSetBase::GetHealthAttribute(), CurrentHealth);
		ASC->SetNumericAttributeBase(USFAttributeSetBase::GetEchoAttribute(), CurrentEcho);
		ASC->SetNumericAttributeBase(USFAttributeSetBase::GetShieldsAttribute(), CurrentShields);
		ASC->SetNumericAttributeBase(USFAttributeSetBase::GetStaminaAttribute(), CurrentStamina);
	}
}

int32 USFProgressionComponent::GetXPToNextLevel() const
{
	if (IsAtMaxLevel())
	{
		return 0;
	}

	const FSFLevelDefinition* NextLevelDef = GetLevelDefinition(CurrentLevel + 1);
	if (!NextLevelDef)
	{
		return 0;
	}

	return FMath::Max(NextLevelDef->XPRequiredToReachThisLevel - CurrentXP, 0);
}

const FSFLevelDefinition* USFProgressionComponent::GetLevelDefinition(int32 Level) const
{
	if (!LevelProgressionTable)
	{
		return nullptr;
	}

	const FString RowNameString = FString::Printf(TEXT("%d"), Level);
	const FName RowName(*RowNameString);

	return LevelProgressionTable->FindRow<FSFLevelDefinition>(RowName, TEXT("GetLevelDefinition"));
}

void USFProgressionComponent::TryProcessLevelUps()
{

	while (!IsAtMaxLevel())
	{
		const FSFLevelDefinition* NextLevelDef = GetLevelDefinition(CurrentLevel + 1);
		if (!NextLevelDef)
		{
			break;
		}

		if (CurrentXP < NextLevelDef->XPRequiredToReachThisLevel)
		{
			break;
		}

		CurrentLevel++;

		ApplyLevelStats(true);
		OnLevelChanged.Broadcast(CurrentLevel, true);
	}

	if (IsAtMaxLevel())
	{
		const FSFLevelDefinition* MaxLevelDef = GetLevelDefinition(MaxLevel);
		if (MaxLevelDef)
		{
			CurrentXP = FMath::Min(CurrentXP, MaxLevelDef->XPRequiredToReachThisLevel);
		}
	}
}

void USFProgressionComponent::BroadcastXPState()
{
	OnXPChanged.Broadcast(CurrentXP, CurrentLevel, GetXPToNextLevel());
}

int32 USFProgressionComponent::GetEnemyXPRewardForCurrentLevel() const
{
	const FSFLevelDefinition* LevelDef = GetLevelDefinition(CurrentLevel);
	return LevelDef ? LevelDef->EnemyXPReward : 0;
}