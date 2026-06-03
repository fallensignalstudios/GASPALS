// Copyright Fallen Signal Studios 2026.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SFInventoryBroadcastCounter.generated.h"

/**
 * Test-only helper that exposes a UFUNCTION matching FOnInventoryUpdatedSignature
 * so a dynamic multicast delegate can be bound from automation specs.
 * AddLambda is not legal on DECLARE_DYNAMIC_MULTICAST_DELEGATE -- those require
 * a UFUNCTION target, hence this tiny shim. Intentionally inlined for zero
 * .cpp surface area.
 */
UCLASS(NotBlueprintable, Transient)
class USFInventoryBroadcastCounter : public UObject
{
	GENERATED_BODY()

public:
	int32 BroadcastCount = 0;

	UFUNCTION()
	void HandleBroadcast()
	{
		++BroadcastCount;
	}
};
