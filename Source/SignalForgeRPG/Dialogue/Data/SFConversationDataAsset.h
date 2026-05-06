#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Dialogue/Data/SFDialogueTypes.h"
#include "Dialogue/Data/SFDialogueStagingData.h"
#include "SFConversationDataAsset.generated.h"

UCLASS(BlueprintType)
class SIGNALFORGERPG_API USFConversationDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Conversation")
	FName ConversationId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Conversation")
	FName EntryNodeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Conversation")
	TArray<FSFDialogueNode> Nodes;

	const FSFDialogueNode* FindNodeById(FName NodeId) const;

	UFUNCTION(BlueprintCallable, Category = "Conversation")
	bool IsValidConversation() const;

	UFUNCTION(BlueprintCallable, Category = "Conversation")
	bool ValidateConversation(TArray<FString>& OutErrors, TArray<FString>& OutWarnings) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Conversation")
	FSFDialogueStagingData StagingData;


protected:
	bool ValidateNodeIds(TArray<FString>& OutErrors) const;
	bool ValidateNodeLinks(TArray<FString>& OutErrors, TArray<FString>& OutWarnings) const;
};