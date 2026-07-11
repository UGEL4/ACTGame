// Copyright Epic Games, Inc. All Rights Reserved.

#include "TestEditorModule.h"
#include "TestEditorEditorModeCommands.h"

#define LOCTEXT_NAMESPACE "TestEditorModule"

void FTestEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FTestEditorEditorModeCommands::Register();
}

void FTestEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	FTestEditorEditorModeCommands::Unregister();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTestEditorModule, TestEditor)