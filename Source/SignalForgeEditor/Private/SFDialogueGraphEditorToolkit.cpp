#include "SFDialogueGraphEditorToolkit.h"

#include "Dialogue/DialogueGraph/SFDialogueGraph.h"
#include "Dialogue/DialogueGraph/SFDialogueEdGraph.h"
#include "Dialogue/DialogueGraph/SFDialogueGraphCompiler.h"

#include "EdGraph/EdGraphNode.h"
#include "EdGraphUtilities.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "GraphEditor.h"
#include "HAL/PlatformApplicationMisc.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

const FName FSFDialogueGraphEditorToolkit::GraphTabId(TEXT("SFDialogueGraphEditor_Graph"));
const FName FSFDialogueGraphEditorToolkit::DetailsTabId(TEXT("SFDialogueGraphEditor_Details"));
const FName FSFDialogueGraphEditorToolkit::CompileResultsTabId(TEXT("SFDialogueGraphEditor_CompileResults"));

void FSFDialogueGraphEditorToolkit::Initialize(USFDialogueGraph* InDialogueGraph, TSharedPtr<IToolkitHost> InitToolkitHost)
{
	DialogueGraph = InDialogueGraph;

	BindGraphEditorCommands();
	CreateInternalWidgets();

	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("SFDialogueGraphEditorLayout_v2")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewStack()
				->AddTab(GraphTabId, ETabState::OpenedTab)
				->SetHideTabWell(true)
				->SetSizeCoefficient(0.7f)
			)
			->Split
			(
				FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)
				->Split
				(
					FTabManager::NewStack()
					->AddTab(DetailsTabId, ETabState::OpenedTab)
					->SetHideTabWell(true)
					->SetSizeCoefficient(0.55f)
				)
				->Split
				(
					FTabManager::NewStack()
					->AddTab(CompileResultsTabId, ETabState::OpenedTab)
					->SetHideTabWell(true)
					->SetSizeCoefficient(0.45f)
				)
			)
		);

	InitAssetEditor(
		EToolkitMode::Standalone,
		InitToolkitHost,
		FName(TEXT("SFDialogueGraphEditorApp")),
		Layout,
		true,
		true,
		InDialogueGraph
	);

	AddToolbarExtender();
}

FName FSFDialogueGraphEditorToolkit::GetToolkitFName() const
{
	return FName(TEXT("SFDialogueGraphEditor"));
}

FText FSFDialogueGraphEditorToolkit::GetBaseToolkitName() const
{
	return FText::FromString(TEXT("Dialogue Graph Editor"));
}

FString FSFDialogueGraphEditorToolkit::GetWorldCentricTabPrefix() const
{
	return TEXT("DialogueGraph");
}

FLinearColor FSFDialogueGraphEditorToolkit::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.2f, 0.4f, 0.9f);
}

void FSFDialogueGraphEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(GraphTabId, FOnSpawnTab::CreateSP(this, &FSFDialogueGraphEditorToolkit::SpawnGraphTab));
	InTabManager->RegisterTabSpawner(DetailsTabId, FOnSpawnTab::CreateSP(this, &FSFDialogueGraphEditorToolkit::SpawnDetailsTab));
	InTabManager->RegisterTabSpawner(CompileResultsTabId, FOnSpawnTab::CreateSP(this, &FSFDialogueGraphEditorToolkit::SpawnCompileResultsTab));
}

void FSFDialogueGraphEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(GraphTabId);
	InTabManager->UnregisterTabSpawner(DetailsTabId);
	InTabManager->UnregisterTabSpawner(CompileResultsTabId);
}

TStatId FSFDialogueGraphEditorToolkit::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FSFDialogueGraphEditorToolkit, STATGROUP_Tickables);
}

TSharedRef<SDockTab> FSFDialogueGraphEditorToolkit::SpawnGraphTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		[
			GraphEditorWidget.ToSharedRef()
		];
}

TSharedRef<SDockTab> FSFDialogueGraphEditorToolkit::SpawnDetailsTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		[
			DetailsViewWidget.ToSharedRef()
		];
}

void FSFDialogueGraphEditorToolkit::BindGraphEditorCommands()
{
	GraphEditorCommands = MakeShared<FUICommandList>();

	GraphEditorCommands->MapAction(
		FGenericCommands::Get().SelectAll,
		FExecuteAction::CreateSP(this, &FSFDialogueGraphEditorToolkit::SelectAllNodes));

	GraphEditorCommands->MapAction(
		FGenericCommands::Get().Delete,
		FExecuteAction::CreateSP(this, &FSFDialogueGraphEditorToolkit::DeleteSelectedNodes),
		FCanExecuteAction::CreateSP(this, &FSFDialogueGraphEditorToolkit::CanDeleteSelectedNodes));

	GraphEditorCommands->MapAction(
		FGenericCommands::Get().Copy,
		FExecuteAction::CreateSP(this, &FSFDialogueGraphEditorToolkit::CopySelectedNodes),
		FCanExecuteAction::CreateSP(this, &FSFDialogueGraphEditorToolkit::CanCopySelectedNodes));

	GraphEditorCommands->MapAction(
		FGenericCommands::Get().Cut,
		FExecuteAction::CreateSP(this, &FSFDialogueGraphEditorToolkit::CutSelectedNodes),
		FCanExecuteAction::CreateSP(this, &FSFDialogueGraphEditorToolkit::CanDeleteSelectedNodes));

	GraphEditorCommands->MapAction(
		FGenericCommands::Get().Duplicate,
		FExecuteAction::CreateSP(this, &FSFDialogueGraphEditorToolkit::DuplicateSelectedNodes),
		FCanExecuteAction::CreateSP(this, &FSFDialogueGraphEditorToolkit::CanDuplicateSelectedNodes));
}

void FSFDialogueGraphEditorToolkit::CreateInternalWidgets()
{
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = FText::FromString(TEXT("Dialogue Graph"));

	SGraphEditor::FGraphEditorEvents GraphEditorEvents;
	GraphEditorEvents.OnSelectionChanged =
		SGraphEditor::FOnSelectionChanged::CreateSP(
			this,
			&FSFDialogueGraphEditorToolkit::OnGraphSelectionChanged);

	GraphEditorWidget =
		SNew(SGraphEditor)
		.AdditionalCommands(GraphEditorCommands)
		.Appearance(AppearanceInfo)
		.GraphToEdit(DialogueGraph ? DialogueGraph->EdGraph : nullptr)
		.GraphEvents(GraphEditorEvents);

	FPropertyEditorModule& PropertyEditorModule =
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bHideSelectionTip = false;

	DetailsViewWidget = PropertyEditorModule.CreateDetailView(DetailsArgs);
	DetailsViewWidget->SetObject(DialogueGraph);

	CompileResultsListView =
		SNew(SListView<TSharedPtr<FSFDialogueCompileResultEntry>>)
		.ListItemsSource(&CompileResults)
		.OnGenerateRow(this, &FSFDialogueGraphEditorToolkit::HandleGenerateCompileResultRow);
}

void FSFDialogueGraphEditorToolkit::OnGraphSelectionChanged(const TSet<UObject*>& NewSelection)
{
	if (!DetailsViewWidget.IsValid())
	{
		return;
	}

	if (NewSelection.Num() == 0)
	{
		DetailsViewWidget->SetObject(DialogueGraph);
		return;
	}

	TArray<UObject*> SelectedObjects;
	for (UObject* Obj : NewSelection)
	{
		SelectedObjects.Add(Obj);
	}

	DetailsViewWidget->SetObjects(SelectedObjects);
}

namespace
{
	void ShowDialogueCompileNotification(const FString& Message, SNotificationItem::ECompletionState State)
	{
		FNotificationInfo Info(FText::FromString(Message));
		Info.FadeInDuration = 0.15f;
		Info.FadeOutDuration = 0.3f;
		Info.ExpireDuration = 3.5f;
		Info.bUseLargeFont = false;
		Info.bFireAndForget = true;

		TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
		if (Notification.IsValid())
		{
			Notification->SetCompletionState(State);
		}
	}
}

void FSFDialogueGraphEditorToolkit::CompileGraph()
{
	CompileResults.Reset();

	if (!DialogueGraph)
	{
		CompileResults.Add(MakeShared<FSFDialogueCompileResultEntry>(TEXT("No DialogueGraph to compile."), true));
		RefreshCompileResults();
		ShowDialogueCompileNotification(TEXT("No DialogueGraph to compile."), SNotificationItem::CS_Fail);
		return;
	}

	TArray<FString> Errors;
	TArray<FString> Warnings;

	const bool bSuccess = FSFDialogueGraphCompiler::Compile(DialogueGraph, Errors, Warnings);

	for (const FString& ErrorString : Errors)
	{
		UE_LOG(LogTemp, Error, TEXT("Dialogue Compile Error: %s"), *ErrorString);
		CompileResults.Add(MakeShared<FSFDialogueCompileResultEntry>(ErrorString, true));
	}

	for (const FString& WarningString : Warnings)
	{
		UE_LOG(LogTemp, Warning, TEXT("Dialogue Compile Warning: %s"), *WarningString);
		CompileResults.Add(MakeShared<FSFDialogueCompileResultEntry>(WarningString, false));
	}

	RefreshCompileResults();

	if (!bSuccess)
	{
		ShowDialogueCompileNotification(
			FString::Printf(TEXT("Dialogue compile failed. Errors: %d, Warnings: %d"), Errors.Num(), Warnings.Num()),
			SNotificationItem::CS_Fail
		);
		return;
	}

	if (Warnings.Num() > 0)
	{
		ShowDialogueCompileNotification(
			FString::Printf(TEXT("Dialogue compiled with warnings: %d"), Warnings.Num()),
			SNotificationItem::CS_Pending
		);
		return;
	}

	CompileResults.Add(MakeShared<FSFDialogueCompileResultEntry>(TEXT("Dialogue compiled successfully."), false));
	RefreshCompileResults();

	ShowDialogueCompileNotification(TEXT("Dialogue compiled successfully."), SNotificationItem::CS_Success);
}

void FSFDialogueGraphEditorToolkit::AddToolbarExtender()
{
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([this]()
		{
			FToolMenuOwnerScoped OwnerScoped(this);

			UToolMenu* Toolbar = UToolMenus::Get()->ExtendMenu("AssetEditor.DefaultToolBar");
			FToolMenuSection& Section = Toolbar->AddSection("Dialogue");

			Section.AddEntry(FToolMenuEntry::InitToolBarButton(
				"CompileDialogue",
				FUIAction(FExecuteAction::CreateSP(this, &FSFDialogueGraphEditorToolkit::CompileGraph)),
				FText::FromString("Compile"),
				FText::FromString("Compile Dialogue Graph"),
				FSlateIcon()
			));
		}));
}

TSharedRef<SDockTab> FSFDialogueGraphEditorToolkit::SpawnCompileResultsTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		[
			SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(4.0f)
				[
					SNew(STextBlock)
						.Text(FText::FromString(TEXT("Compile Results")))
				]

				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(4.0f)
				[
					CompileResultsListView.ToSharedRef()
				]
		];
}

TSharedRef<ITableRow> FSFDialogueGraphEditorToolkit::HandleGenerateCompileResultRow(
	TSharedPtr<FSFDialogueCompileResultEntry> InItem,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	const FSlateColor RowColor = InItem.IsValid() && InItem->bIsError
		? FSlateColor(FLinearColor(0.9f, 0.25f, 0.25f))
		: FSlateColor(FLinearColor(0.9f, 0.8f, 0.25f));

	return SNew(STableRow<TSharedPtr<FSFDialogueCompileResultEntry>>, OwnerTable)
		[
			SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
				.ContentPadding(2.0f)
				.OnClicked_Lambda([this, InItem]()
					{
						if (InItem.IsValid() && InItem->SourceNode.IsValid() && GraphEditorWidget.IsValid())
						{
							GraphEditorWidget->JumpToNode(InItem->SourceNode.Get(), false);
						}
						return FReply::Handled();
					})
				[
					SNew(STextBlock)
						.ColorAndOpacity(RowColor)
						.Text(FText::FromString(InItem.IsValid() ? InItem->Message : TEXT("Invalid compile result.")))
						.AutoWrapText(true)
				]
		];
}

void FSFDialogueGraphEditorToolkit::RefreshCompileResults()
{
	if (CompileResultsListView.IsValid())
	{
		CompileResultsListView->RequestListRefresh();
	}
}

void FSFDialogueGraphEditorToolkit::SelectAllNodes()
{
	if (GraphEditorWidget.IsValid())
	{
		GraphEditorWidget->SelectAllNodes();
	}
}

bool FSFDialogueGraphEditorToolkit::CanDeleteSelectedNodes() const
{
	if (!GraphEditorWidget.IsValid())
	{
		return false;
	}

	const FGraphPanelSelectionSet SelectedNodes = GraphEditorWidget->GetSelectedNodes();
	for (UObject* SelectedObject : SelectedNodes)
	{
		if (const UEdGraphNode* Node = Cast<UEdGraphNode>(SelectedObject))
		{
			if (Node->CanUserDeleteNode())
			{
				return true;
			}
		}
	}

	return false;
}

bool FSFDialogueGraphEditorToolkit::CanCopySelectedNodes() const
{
	if (!GraphEditorWidget.IsValid())
	{
		return false;
	}

	const FGraphPanelSelectionSet SelectedNodes = GraphEditorWidget->GetSelectedNodes();
	for (UObject* SelectedObject : SelectedNodes)
	{
		if (const UEdGraphNode* Node = Cast<UEdGraphNode>(SelectedObject))
		{
			if (Node->CanDuplicateNode())
			{
				return true;
			}
		}
	}

	return false;
}

bool FSFDialogueGraphEditorToolkit::CanDuplicateSelectedNodes() const
{
	return CanCopySelectedNodes();
}

void FSFDialogueGraphEditorToolkit::DeleteSelectedNodes()
{
	DeleteSelectedDuplicableNodes();
}

void FSFDialogueGraphEditorToolkit::DeleteSelectedDuplicableNodes()
{
	if (!GraphEditorWidget.IsValid() || !DialogueGraph || !DialogueGraph->EdGraph)
	{
		return;
	}

	const FScopedTransaction Transaction(NSLOCTEXT("SFDialogueGraphEditor", "DeleteSelectedNodes", "Delete Dialogue Node"));

	DialogueGraph->EdGraph->Modify();

	const FGraphPanelSelectionSet SelectedNodes = GraphEditorWidget->GetSelectedNodes();
	GraphEditorWidget->ClearSelectionSet();

	for (UObject* SelectedObject : SelectedNodes)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(SelectedObject))
		{
			if (Node->CanUserDeleteNode())
			{
				Node->Modify();
				Node->DestroyNode();
			}
		}
	}
}

void FSFDialogueGraphEditorToolkit::CopySelectedNodes()
{
	if (!GraphEditorWidget.IsValid())
	{
		return;
	}

	FGraphPanelSelectionSet SelectedNodes = GraphEditorWidget->GetSelectedNodes();

	FString ExportedText;
	for (UObject* SelectedObject : SelectedNodes)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(SelectedObject))
		{
			Node->PrepareForCopying();
		}
	}

	FEdGraphUtilities::ExportNodesToText(SelectedNodes, ExportedText);
	FPlatformApplicationMisc::ClipboardCopy(*ExportedText);
}

void FSFDialogueGraphEditorToolkit::CutSelectedNodes()
{
	CopySelectedNodes();
	DeleteSelectedDuplicableNodes();
}

void FSFDialogueGraphEditorToolkit::DuplicateSelectedNodes()
{
	CopySelectedNodes();
	// Paste can come next. For now this keeps the command path in place.
}