// SignalForgeGameInstance.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SignalForgeGameInstance.generated.h"

UCLASS()
class SIGNALFORGERPG_API USignalForgeGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
};