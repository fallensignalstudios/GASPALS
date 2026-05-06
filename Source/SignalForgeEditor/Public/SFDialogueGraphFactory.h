#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "SFDialogueGraphFactory.generated.h"

UCLASS()
class SIGNALFORGEEDITOR_API USFDialogueGraphFactory : public UFactory
{
	GENERATED_BODY()

public:
	USFDialogueGraphFactory();

	virtual UObject* FactoryCreateNew(
		UClass* Class,
		UObject* InParent,
		FName Name,
		EObjectFlags Flags,
		UObject* Context,
		FFeedbackContext* Warn) override;
};