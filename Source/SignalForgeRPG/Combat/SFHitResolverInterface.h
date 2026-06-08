#pragma once

#include "UObject/Interface.h"
#include "Combat/SFHitTypes.h"
#include "SFHitResolverInterface.generated.h"

UINTERFACE(MinimalAPI)
class USFHitResolverInterface : public UInterface
{
	GENERATED_BODY()
};

class ISFHitResolverInterface
{
	GENERATED_BODY()

public:
	// Plain C++ pure-virtual, NOT a BlueprintNativeEvent. See SFCombatantInterface.h
	// for the rationale (UHT event stubs assert when called directly from C++).
	virtual FSFResolvedHit ResolveIncomingHit(const FSFHitData& HitData) = 0;
};