#include "SFDialogueGraphFactory.h"
#include "Dialogue/DialogueGraph/SFDialogueGraph.h"
#include "Dialogue/DialogueGraph/SFDialogueEdGraph.h"
#include "SFDialogueGraphSchema.h"

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

		// USFDialogueGraph lives in the runtime module and cannot reference
		// the editor-only schema class, so the schema is assigned here at
		// asset-creation time. The toolkit performs the same assignment as a
		// safety net for any legacy assets that were saved before the editor
		// module was wired into the build.
		if (NewGraph->EdGraph && NewGraph->EdGraph->Schema == nullptr)
		{
			NewGraph->EdGraph->Schema = USFDialogueGraphSchema::StaticClass();
		}
	}

	return NewGraph;
}
