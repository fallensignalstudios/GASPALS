#include "Combat/SFWeaponAnimationSet.h"

void USFWeaponAnimationSet::LoadAnimationSetContentSynchronously()
{
	if (IdleSequenceBaseRaw.IsValid())
	{
		IdleSequenceBase = IdleSequenceBaseRaw.LoadSynchronous();
	}

	if (WalkSequenceBaseRaw.IsValid())
	{
		WalkSequenceBase = WalkSequenceBaseRaw.LoadSynchronous();
	}

	if (SprintSequenceBaseRaw.IsValid())
	{
		SprintSequenceBase = SprintSequenceBaseRaw.LoadSynchronous();
	}

	if (StrafeSequenceBaseRaw.IsValid())
	{
		StrafeSequenceBase = StrafeSequenceBaseRaw.LoadSynchronous();
	}

	if (TurnInPlaceLeftSequenceBaseRaw.IsValid())
	{
		TurnInPlaceLeftSequenceBase = TurnInPlaceLeftSequenceBaseRaw.LoadSynchronous();
	}

	if (TurnInPlaceRightSequenceBaseRaw.IsValid())
	{
		TurnInPlaceRightSequenceBase = TurnInPlaceRightSequenceBaseRaw.LoadSynchronous();
	}

	if (JumpSequenceBaseRaw.IsValid())
	{
		JumpSequenceBase = JumpSequenceBaseRaw.LoadSynchronous();
	}

	if (LandSequenceBaseRaw.IsValid())
	{
		LandSequenceBase = LandSequenceBaseRaw.LoadSynchronous();
	}

	if (DeathForwardSequenceBaseRaw.IsValid())
	{
		DeathForwardSequenceBase = DeathForwardSequenceBaseRaw.LoadSynchronous();
	}

	if (DeathBackwardSequenceBaseRaw.IsValid())
	{
		DeathBackwardSequenceBase = DeathBackwardSequenceBaseRaw.LoadSynchronous();
	}
}