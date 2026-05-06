#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SFDialogueGraph.generated.h"

class USFDialogueEdGraph;
class USFConversationDataAsset;
class FObjectPreSaveContext;

UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFDialogueGraph : public UObject
{
	GENERATED_BODY()

public:
	USFDialogueGraph();

	UPROPERTY()
	TObjectPtr<USFDialogueEdGraph> EdGraph = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName ConversationId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<USFConversationDataAsset> CompiledConversation = nullptr;

	virtual void PreSave(FObjectPreSaveContext SaveContext) override;

	void EnsureGraphInitialized();
};