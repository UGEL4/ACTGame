// Copyright Epic Games, Inc. All Rights Reserved.

#include "ActionEditorCommands.h"

#define LOCTEXT_NAMESPACE "FActionEditorModule"

void FActionEditorCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "ActionEditor", "Bring up ActionEditor window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
