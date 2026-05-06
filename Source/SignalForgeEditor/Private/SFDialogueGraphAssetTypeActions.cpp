#include "SFDialogueGraphAssetTypeActions.h"
#include "Dialogue/DialogueGraph/SFDialogueGraph.h"
#include "SFDialogueGraphEditorToolkit.h"

#define LOCTEXT_NAMESPACE "SFDialogueGraphAssetTypeActions"

FText FSFDialogueGraphAssetTypeActions::GetName() const
{
	return LOCTEXT("SFDialogueGraphAssetName", "Dialogue Graph");
}

FColor FSFDialogueGraphAssetTypeActions::GetTypeColor() const
{
	return FColor(80, 160, 255);
}

UClass* FSFDialogueGraphAssetTypeActions::GetSupportedClass() const
{
	return USFDialogueGraph::StaticClass();
}

uint32 FSFDialogueGraphAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Misc;
}

void FSFDialogueGraphAssetTypeActions::OpenAssetEditor(
	const TArray<UObject*>& InObjects,
	TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	for (UObject* Object : InObjects)
	{
		if (USFDialogueGraph* DialogueGraph = Cast<USFDialogueGraph>(Object))
		{
			TSharedRef<FSFDialogueGraphEditorToolkit> EditorToolkit = MakeShared<FSFDialogueGraphEditorToolkit>();
			EditorToolkit->Initialize(DialogueGraph, EditWithinLevelEditor);
		}
	}
}

#undef LOCTEXT_NAMESPACE