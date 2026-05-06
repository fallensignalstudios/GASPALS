#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "TickableEditorObject.h"

class IDetailsView;
class SGraphEditor;
template<typename ItemType>
class SListView;
class USFDialogueGraph;
class UEdGraphNode;
class FUICommandList;

struct FSFDialogueCompileResultEntry
{
	FString Message;
	bool bIsError = false;
	TWeakObjectPtr<UEdGraphNode> SourceNode;

	FSFDialogueCompileResultEntry() = default;

	FSFDialogueCompileResultEntry(const FString& InMessage, bool bInIsError, UEdGraphNode* InNode = nullptr)
		: Message(InMessage)
		, bIsError(bInIsError)
		, SourceNode(InNode)
	{
	}
};

class SIGNALFORGEEDITOR_API FSFDialogueGraphEditorToolkit
	: public FAssetEditorToolkit
	, public FTickableEditorObject
{
public:
	void Initialize(USFDialogueGraph* InDialogueGraph, TSharedPtr<class IToolkitHost> InitToolkitHost);

	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;

	virtual void Tick(float DeltaTime) override {}
	virtual TStatId GetStatId() const override;

	void OnGraphSelectionChanged(const TSet<UObject*>& NewSelection);

protected:
	TSharedRef<class SDockTab> SpawnGraphTab(const class FSpawnTabArgs& Args);
	TSharedRef<class SDockTab> SpawnDetailsTab(const class FSpawnTabArgs& Args);
	TSharedRef<class SDockTab> SpawnCompileResultsTab(const class FSpawnTabArgs& Args);

	void CreateInternalWidgets();
	void CompileGraph();
	void AddToolbarExtender();
	void RefreshCompileResults();

	void BindGraphEditorCommands();

	void SelectAllNodes();
	void DeleteSelectedNodes();
	void DeleteSelectedDuplicableNodes();
	void CopySelectedNodes();
	void CutSelectedNodes();
	void DuplicateSelectedNodes();
	bool CanDeleteSelectedNodes() const;
	bool CanCopySelectedNodes() const;
	bool CanDuplicateSelectedNodes() const;

	TSharedRef<class ITableRow> HandleGenerateCompileResultRow(
		TSharedPtr<FSFDialogueCompileResultEntry> InItem,
		const TSharedRef<class STableViewBase>& OwnerTable);

protected:
	TObjectPtr<USFDialogueGraph> DialogueGraph = nullptr;

	TSharedPtr<FUICommandList> GraphEditorCommands;
	TSharedPtr<SGraphEditor> GraphEditorWidget;
	TSharedPtr<IDetailsView> DetailsViewWidget;

	TArray<TSharedPtr<FSFDialogueCompileResultEntry>> CompileResults;
	TSharedPtr<SListView<TSharedPtr<FSFDialogueCompileResultEntry>>> CompileResultsListView;

	static const FName GraphTabId;
	static const FName DetailsTabId;
	static const FName CompileResultsTabId;
};