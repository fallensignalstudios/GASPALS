// SFAbilityBarWidgetController.cpp

#include "UI/SFAbilityBarWidgetController.h"

#include "AbilitySystem/SFAbilitySystemComponent.h"
#include "Characters/SFCharacterBase.h"

void USFAbilityBarWidgetController::Initialize(ASFCharacterBase* InPlayerCharacter)
{
	PlayerCharacter = InPlayerCharacter;
	if (!PlayerCharacter)
	{
		return;
	}

	AbilitySystemComponent = Cast<USFAbilitySystemComponent>(PlayerCharacter->GetAbilitySystemComponent());
	if (!AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->OnAbilitiesChanged().AddUObject(
		this, &USFAbilityBarWidgetController::HandleAbilitiesChanged);

	RefreshAbilitySlots();
}

void USFAbilityBarWidgetController::RefreshAbilitySlots()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	RebuildFromASC();
}

FSFAbilitySlotUIData USFAbilityBarWidgetController::GetSlotData(FGameplayTag InputTag) const
{
	if (const FSFAbilitySlotUIData* Found = SlotDataMap.Find(InputTag))
	{
		return *Found;
	}

	FSFAbilitySlotUIData Empty;
	Empty.InputTag = InputTag;
	return Empty;
}

void USFAbilityBarWidgetController::HandleAbilitiesChanged()
{
	RebuildFromASC();
}

void USFAbilityBarWidgetController::RebuildFromASC()
{
	UnbindCooldownDelegates();

	SlotDataMap.Empty();
	CooldownTagByInputTag.Empty();

	TArray<FSFAbilitySlotUIData> SlotDataArray;
	AbilitySystemComponent->GetAbilityBarSlotData(SlotDataArray);

	for (const FSFAbilitySlotUIData& SlotData : SlotDataArray)
	{
		if (!SlotData.InputTag.IsValid())
		{
			continue;
		}

		SlotDataMap.Add(SlotData.InputTag, SlotData);

		if (SlotData.CooldownTag.IsValid())
		{
			CooldownTagByInputTag.Add(SlotData.InputTag, SlotData.CooldownTag);
		}
	}

	RebuildCooldownListeners();
	BroadcastAllSlots();
}

void USFAbilityBarWidgetController::BroadcastAllSlots()
{
	for (const TPair<FGameplayTag, FSFAbilitySlotUIData>& Pair : SlotDataMap)
	{
		OnAbilitySlotUpdated.Broadcast(Pair.Key, Pair.Value);
	}
}

void USFAbilityBarWidgetController::BroadcastSlot(const FGameplayTag& InputTag)
{
	if (const FSFAbilitySlotUIData* Data = SlotDataMap.Find(InputTag))
	{
		OnAbilitySlotUpdated.Broadcast(InputTag, *Data);
	}
}

void USFAbilityBarWidgetController::UnbindCooldownDelegates()
{
	if (!AbilitySystemComponent)
	{
		CooldownDelegateHandles.Empty();
		return;
	}

	for (const TPair<FGameplayTag, FDelegateHandle>& Pair : CooldownDelegateHandles)
	{
		AbilitySystemComponent
			->RegisterGameplayTagEvent(Pair.Key, EGameplayTagEventType::NewOrRemoved)
			.Remove(Pair.Value);
	}

	CooldownDelegateHandles.Empty();
}

void USFAbilityBarWidgetController::RebuildCooldownListeners()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	TSet<FGameplayTag> UniqueCooldownTags;
	for (const TPair<FGameplayTag, FGameplayTag>& Pair : CooldownTagByInputTag)
	{
		if (Pair.Value.IsValid())
		{
			UniqueCooldownTags.Add(Pair.Value);
		}
	}

	for (const FGameplayTag& CooldownTag : UniqueCooldownTags)
	{
		const FDelegateHandle Handle =
			AbilitySystemComponent
			->RegisterGameplayTagEvent(CooldownTag, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &USFAbilityBarWidgetController::HandleCooldownTagChanged);

		CooldownDelegateHandles.Add(CooldownTag, Handle);
	}
}

void USFAbilityBarWidgetController::HandleCooldownTagChanged(FGameplayTag ChangedTag, int32 NewCount)
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	const bool bIsReady = (NewCount <= 0);

	for (TPair<FGameplayTag, FGameplayTag>& Pair : CooldownTagByInputTag)
	{
		if (Pair.Value != ChangedTag)
		{
			continue;
		}

		if (FSFAbilitySlotUIData* SlotData = SlotDataMap.Find(Pair.Key))
		{
			SlotData->bIsReady = bIsReady;
			BroadcastSlot(Pair.Key);
		}
	}
}