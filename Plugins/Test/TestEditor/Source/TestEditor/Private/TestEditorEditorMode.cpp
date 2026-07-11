// Copyright Epic Games, Inc. All Rights Reserved.

#include "TestEditorEditorMode.h"
#include "TestEditorEditorModeToolkit.h"
#include "EdModeInteractiveToolsContext.h"
#include "InteractiveToolManager.h"
#include "TestEditorEditorModeCommands.h"
#include "Modules/ModuleManager.h"


//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
// AddYourTool Step 1 - include the header file for your Tools here
//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////// 
#include "Tools/TestEditorSimpleTool.h"
#include "Tools/TestEditorInteractiveTool.h"

// step 2: register a ToolBuilder in FTestEditorEditorMode::Enter() below


#define LOCTEXT_NAMESPACE "TestEditorEditorMode"

const FEditorModeID UTestEditorEditorMode::EM_TestEditorEditorModeId = TEXT("EM_TestEditorEditorMode");

FString UTestEditorEditorMode::SimpleToolName = TEXT("TestEditor_ActorInfoTool");
FString UTestEditorEditorMode::InteractiveToolName = TEXT("TestEditor_MeasureDistanceTool");


UTestEditorEditorMode::UTestEditorEditorMode()
{
	FModuleManager::Get().LoadModule("EditorStyle");

	// appearance and icon in the editing mode ribbon can be customized here
	Info = FEditorModeInfo(UTestEditorEditorMode::EM_TestEditorEditorModeId,
		LOCTEXT("ModeName", "TestEditor"),
		FSlateIcon(),
		true);
}


UTestEditorEditorMode::~UTestEditorEditorMode()
{
}


void UTestEditorEditorMode::ActorSelectionChangeNotify()
{
}

void UTestEditorEditorMode::Enter()
{
	UEdMode::Enter();

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	// AddYourTool Step 2 - register the ToolBuilders for your Tools here.
	// The string name you pass to the ToolManager is used to select/activate your ToolBuilder later.
	//////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////// 
	const FTestEditorEditorModeCommands& SampleToolCommands = FTestEditorEditorModeCommands::Get();

	RegisterTool(SampleToolCommands.SimpleTool, SimpleToolName, NewObject<UTestEditorSimpleToolBuilder>(this));
	RegisterTool(SampleToolCommands.InteractiveTool, InteractiveToolName, NewObject<UTestEditorInteractiveToolBuilder>(this));

	// active tool type is not relevant here, we just set to default
	GetToolManager()->SelectActiveToolType(EToolSide::Left, SimpleToolName);
}

void UTestEditorEditorMode::CreateToolkit()
{
	Toolkit = MakeShareable(new FTestEditorEditorModeToolkit);
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> UTestEditorEditorMode::GetModeCommands() const
{
	return FTestEditorEditorModeCommands::Get().GetCommands();
}

#undef LOCTEXT_NAMESPACE
