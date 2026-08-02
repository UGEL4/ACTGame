// Copyright Epic Games, Inc. All Rights Reserved.

#include "ActionEditor.h"
#include "ActionEditorStyle.h"
#include "ActionEditorCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"
#include "SActionEditorMenu.h"
#include "SActionEditor_Sequence.h"
#include "../TrackEditor/CancelTag_TrackEditor.h"
#include "../TrackEditor/ActionInfo_TrackEditor.h"
#include "../TrackEditor/ActionCommand/Command_TrackEditor.h"
#include "../TrackEditor/HitBox/HitBox_TrackEditor.h"

static const FName ActionEditorTabName("ActionEditor");

#define LOCTEXT_NAMESPACE "FActionEditorModule"

void FActionEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FActionEditorStyle::Initialize();
	FActionEditorStyle::ReloadTextures();

	FActionEditorCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FActionEditorCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FActionEditorModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FActionEditorModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ActionEditorTabName, FOnSpawnTab::CreateRaw(this, &FActionEditorModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FActionEditorTabTitle", "ActionEditor"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	ISequencerModule& Module     = FModuleManager::LoadModuleChecked<ISequencerModule>("sequencer");
    CancelTag_TrackEditorHandle  = Module.RegisterTrackEditor(FOnCreateTrackEditor::CreateStatic(&FCancelTag_TrackEditor::CreateTrackEditor));
    ActionInfo_TrackEditorHandle = Module.RegisterTrackEditor(FOnCreateTrackEditor::CreateStatic(&FActionInfo_TrackEditor::CreateTrackEditor));
    FHitBox_TrackEditorHandle    = Module.RegisterTrackEditor(FOnCreateTrackEditor::CreateStatic(&FHitBox_TrackEditor::CreateTrackEditor));
	//ActionCommand_TrackEditorHandle = Module.RegisterTrackEditor(FOnCreateTrackEditor::CreateStatic(&FCommand_TrackEditor::CreateTrackEditor));
}

void FActionEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FActionEditorStyle::Shutdown();

	FActionEditorCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ActionEditorTabName);

	ISequencerModule& Module = FModuleManager::LoadModuleChecked<ISequencerModule>("sequencer");
    Module.UnRegisterTrackEditor(CancelTag_TrackEditorHandle);
    Module.UnRegisterTrackEditor(ActionInfo_TrackEditorHandle);
    Module.UnRegisterTrackEditor(FHitBox_TrackEditorHandle);
	//Module.UnRegisterTrackEditor(ActionCommand_TrackEditorHandle);
}

TSharedRef<SDockTab> FActionEditorModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	FText WidgetText = FText::Format(
		LOCTEXT("WindowWidgetText", "Add code to {0} in {1} to override this window's contents"),
		FText::FromString(TEXT("FActionEditorModule::OnSpawnPluginTab")),
		FText::FromString(TEXT("ActionEditor.cpp"))
		);

	TSharedRef<SActionEditorMenu> MenuWidget          = SNew(SActionEditorMenu);
    TSharedRef<SActionEditor_Sequence> SequenceWidget = SNew(SActionEditor_Sequence);
    MenuWidget->SetSequencerEditor(SequenceWidget);

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			// Put your tab content here!
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				MenuWidget
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0)
			[
				SequenceWidget
			]
			// SNew(SBox)
			// .HAlign(HAlign_Center)
			// .VAlign(VAlign_Center)
			// [
			// 	SNew(STextBlock)
			// 	.Text(WidgetText)
			// ]
			//SNew(SSplitter)
			//+ SSplitter::Slot()
			//.Value(0.75)
			//[
			//	// 左侧：时间轴主体区域
			//	SNew(SVerticalBox)
			//	+ SVerticalBox::Slot()
			//	.AutoHeight()
			//	[
			//		// 工具栏
			//		ConstructToolbar()
			//	]
			//	+ SVerticalBox::Slot()
			//	.FillHeight(1.0f)
			//	//[
			//	//	// 轨道列表与时间轴
			//	//	//ConstructTimelineArea()
			//	//]
			//]
			//+ SSplitter::Slot()
			//	// .Value(0.25f) // 占据25%空间
			//	// [
			//	// 	// 右侧：属性面板
			//	// 	ConstructDetailsPanel()
			//	// ]
		];
}

void FActionEditorModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(ActionEditorTabName);
}

void FActionEditorModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FActionEditorCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FActionEditorCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}


TSharedRef<SWidget> FActionEditorModule::ConstructToolbar()
{
	// 1. 绑定命令列表（通常使用 FUICommandList）
	// UICommandList = MakeShareable(new FUICommandList);
	// UICommandList->MapAction(
	// 	MyCommands->AddTrack, // 你自定义的命令
	// 	FExecuteAction::CreateRaw(this, &FMyTimelineEditor::OnAddTrack),
	// 	FCanExecuteAction::CreateRaw(this, &FMyTimelineEditor::CanAddTrack)
	// );

	// 2. 使用 FToolBarBuilder 构建真正的工具栏UI
	FToolBarBuilder ToolbarBuilder(PluginCommands, FMultiBoxCustomization::None);
	//ToolbarBuilder.AddToolBarButton(MyCommands->AddTrack);
	//ToolbarBuilder.AddToolBarButton(MyCommands->PlayAnimation);
	// ... 添加更多按钮

	// 3. 返回构建好的 Widget
	return ToolbarBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FActionEditorModule, ActionEditor)