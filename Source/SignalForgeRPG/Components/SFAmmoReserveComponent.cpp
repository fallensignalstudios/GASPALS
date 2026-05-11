// Copyright Fallen Signal Studios LLC.

#include "Components/SFAmmoReserveComponent.h"
#include "Combat/SFAmmoType.h"

USFAmmoReserveComponent::USFAmmoReserveComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

int32 USFAmmoReserveComponent::GetAmmoCount(USFAmmoType* AmmoType) const
{
	if (!AmmoType)
	{
		return 0;
	}
	const int32* Found = Reserves.Find(AmmoType);
	return Found ? *Found : 0;
}

bool USFAmmoReserveComponent::HasRoomFor(USFAmmoType* AmmoType) const
{
	if (!AmmoType)
	{
		return false;
	}
	return GetAmmoCount(AmmoType) < AmmoType->MaxCarried;
}

int32 USFAmmoReserveComponent::AddAmmo(USFAmmoType* AmmoType, int32 Amount)
{
	if (!AmmoType || Amount <= 0)
	{
		return 0;
	}

	const int32 Existing = GetAmmoCount(AmmoType);
	const int32 NewAmount = FMath::Min(Existing + Amount, AmmoType->MaxCarried);
	const int32 Added = NewAmount - Existing;
	if (Added > 0)
	{
		Reserves.Add(AmmoType, NewAmount);
		OnAmmoReserveChanged.Broadcast(AmmoType, NewAmount, AmmoType->MaxCarried);
	}
	return Added;
}

int32 USFAmmoReserveComponent::ConsumeAmmo(USFAmmoType* AmmoType, int32 RequestedAmount)
{
	if (!AmmoType || RequestedAmount <= 0)
	{
		return 0;
	}

	const int32 Existing = GetAmmoCount(AmmoType);
	const int32 Consumed = FMath::Min(Existing, RequestedAmount);
	if (Consumed <= 0)
	{
		return 0;
	}

	const int32 NewAmount = Existing - Consumed;
	Reserves.Add(AmmoType, NewAmount);
	OnAmmoReserveChanged.Broadcast(AmmoType, NewAmount, AmmoType->MaxCarried);
	return Consumed;
}

void USFAmmoReserveComponent::SetAmmoCount(USFAmmoType* AmmoType, int32 Amount)
{
	if (!AmmoType)
	{
		return;
	}
	const int32 Clamped = FMath::Clamp(Amount, 0, AmmoType->MaxCarried);
	Reserves.Add(AmmoType, Clamped);
	OnAmmoReserveChanged.Broadcast(AmmoType, Clamped, AmmoType->MaxCarried);
}

void USFAmmoReserveComponent::EnsureStartingAmmo(USFAmmoType* AmmoType)
{
	if (!AmmoType)
	{
		return;
	}
	const int32 Current = GetAmmoCount(AmmoType);
	if (Current < AmmoType->DefaultStartingAmount)
	{
		SetAmmoCount(AmmoType, AmmoType->DefaultStartingAmount);
	}
}
