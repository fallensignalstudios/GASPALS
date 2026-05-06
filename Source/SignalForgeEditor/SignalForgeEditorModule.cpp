#include "SignalForgeEditorModule.h"

#include "ToolMenus.h"
#include "Misc/MessageDialog.h"
#include "AssetToolsModule.h"
#include "SFDialogueGraphAssetTypeActions.h"

#define LOCTEXT_NAMESPACE "FSignalForgeEditorModule"

void FSignalForgeEditorModule::StartupModule()
{
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FSignalForgeEditorModule::RegisterMenus));

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	DialogueGraphAssetTypeActions = MakeShared<FSFDialogueGraphAssetTypeActions>();
	AssetTools.RegisterAssetTypeActions(DialogueGraphAssetTypeActions.ToSharedRef());
}

void FSignalForgeEditorModule::ShutdownModule()
{
	UnregisterMenus();

	if (DialogueGraphAssetTypeActions.IsValid() &&
		FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		AssetTools.UnregisterAssetTypeActions(DialogueGraphAssetTypeActions.ToSharedRef());
	}

	DialogueGraphAssetTypeActions.Reset();
}

void FSignalForgeEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
	FToolMenuSection& Section = Menu->FindOrAddSection("SignalForge");

	Section.AddMenuEntry(
		"SignalForge_OpenRetargetTool",
		LOCTEXT("SignalForgeOpenRetargetTool_Label", "Signal Forge Retarget"),
		LOCTEXT("SignalForgeOpenRetargetTool_Tooltip", "Open the Signal Forge retarget pipeline tool."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FSignalForgeEditorModule::HandleOpenRetargetTool)));
}

void FSignalForgeEditorModule::UnregisterMenus()
{
	if (UToolMenus::IsToolMenuUIEnabled())
	{
		UToolMenus::UnregisterOwner(this);
	}
}

void FSignalForgeEditorModule::HandleOpenRetargetTool()
{
	FMessageDialog::Open(
		EAppMsgType::Ok,
		LOCTEXT("SignalForgeRetargetPlaceholder",
			"Signal Forge Retarget tool is wired up. Next step: add the retarget pipeline UI/widget."));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSignalForgeEditorModule, SignalForgeEditor)