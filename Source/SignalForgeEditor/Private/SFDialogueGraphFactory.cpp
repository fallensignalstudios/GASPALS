#include "SFDialogueGraphFactory.h"
#include "Dialogue/DialogueGraph/SFDialogueGraph.h"

USFDialogueGraphFactory::USFDialogueGraphFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = USFDialogueGraph::StaticClass();
}

UObject* USFDialogueGraphFactory::FactoryCreateNew(
	UClass* Class,
	UObject* InParent,
	FName Name,
	EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn)
{
	USFDialogueGraph* NewGraph = NewObject<USFDialogueGraph>(InParent, Class, Name, Flags | RF_Transactional);

	if (NewGraph)
	{
		NewGraph->EnsureGraphInitialized();
	}

	return NewGraph;
}