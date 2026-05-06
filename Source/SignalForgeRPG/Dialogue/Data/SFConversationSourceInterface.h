#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SFConversationSourceInterface.generated.h"

class USFConversationDataAsset;

UINTERFACE(BlueprintType)
class SIGNALFORGERPG_API USFConversationSourceInterface : public UInterface
{
	GENERATED_BODY()
};

class SIGNALFORGERPG_API ISFConversationSourceInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue")
	USFConversationDataAsset* GetConversationAsset() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue")
	FName GetDialogueSpeakerId() const;
};