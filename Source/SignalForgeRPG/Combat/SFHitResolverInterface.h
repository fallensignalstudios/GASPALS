#pragma once

#include "UObject/Interface.h"
#include "Combat/SFHitTypes.h"
#include "SFHitResolverInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class USFHitResolverInterface : public UInterface
{
	GENERATED_BODY()
};

class ISFHitResolverInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Combat")
	FSFResolvedHit ResolveIncomingHit(const FSFHitData& HitData);
};